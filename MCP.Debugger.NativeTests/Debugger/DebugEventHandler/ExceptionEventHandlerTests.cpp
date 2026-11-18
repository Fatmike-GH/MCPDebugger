#include "Mocks/Debugger/Interfaces/MockBreakpoint.h"
#include "Mocks/Debugger/Interfaces/MockBreakpointFactory.h"
#include "Mocks/Debugger/Interfaces/MockBreakpointManager.h"
#include "Mocks/Debugger/Interfaces/MockDebuggerCommandQueue.h"
#include "Mocks/Debugger/Interfaces/MockDebuggerEventHandler.h"
#include "Mocks/Disassembler/Interfaces/MockDisassembler.h"
#include "Native/Debugger/BreakpointManager.h"
#include "Native/Debugger/Commands/Base/CommandType.h"
#include "Native/Debugger/DebugEventHandler/DebugEventStorage.h"
#include "Native/Debugger/DebugEventHandler/ExceptionEventHandler.h"
#include "Native/Debugger/DebugEventHandler/HandleResult.h"
#include "Native/Threading/EventWaiter.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

// The production names are written out from the root namespace: inside the test namespace "Debugger" and
// "DebugEventHandler" name the test namespaces.
namespace MCP::Native::Tests::Debugger::DebugEventHandler
{
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockBreakpoint;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockBreakpointFactory;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockBreakpointManager;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockDebuggerCommandQueue;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockDebuggerEventHandler;
  using MCP::Native::Tests::Mocks::Disassembler::Interfaces::MockDisassembler;
  using ::testing::_;
  using ::testing::AnyNumber;
  using ::testing::ByMove;
  using ::testing::DoAll;
  using ::testing::InvokeWithoutArgs;
  using ::testing::IsNull;
  using ::testing::NiceMock;
  using ::testing::Return;
  using ::testing::Sequence;
  using ::testing::SetArgReferee;
  using ::testing::StrictMock;

  using BreakpointManager = MCP::Native::Debugger::BreakpointManager;
  using CommandType = MCP::Native::Debugger::Commands::Base::CommandType;
  using DebugEventStorage = MCP::Native::Debugger::DebugEventHandler::DebugEventStorage;
  using EventWaiter = MCP::Native::Threading::EventWaiter;
  using ExceptionEventHandler = MCP::Native::Debugger::DebugEventHandler::ExceptionEventHandler;
  using HandleResult = MCP::Native::Debugger::DebugEventHandler::HandleResult;
  using IBreakpoint = MCP::Native::Debugger::Interfaces::IBreakpoint;

  /// <summary>
  /// Tests what the exception handler does to breakpoint instructions around a go command. No debuggee runs:
  /// the test delivers the debug events itself, and a single-step exception at an address stands for "the
  /// thread executed one instruction and stopped there".
  ///
  /// The breakpoint bookkeeping is the real breakpoint manager, because the behaviour under test is the
  /// interplay of the handler's hold and release with a set that arrives in between. Its breakpoints are
  /// doubles that record whether their breakpoint instruction would be in the debuggee's memory. The handler
  /// reaches the manager through a double that passes every call on except the trap flag, which would need a
  /// thread; the double counts the single steps the handler asks for instead.
  ///
  /// The thread of the events has a null handle. The handler reads and writes the thread context itself; with
  /// a null handle those calls into the system fail at once and change nothing.
  /// </summary>
  class ExceptionEventHandlerTests : public ::testing::Test
  {
  protected:
    static constexpr DWORD ProcessId = 1000;
    static constexpr DWORD ThreadId = 2000;

    /// <summary>Entry point of the process. Stops there are handled differently, so no test stops at it.</summary>
    static constexpr DWORD_PTR EntryPoint = 0x401000;

    /// <summary>The address the debuggee is stopped at when the go command is issued.</summary>
    static constexpr DWORD_PTR StopAddress = 0x401010;

    /// <summary>Where the thread stands once it executed the instruction at the stop address.</summary>
    static constexpr DWORD_PTR NextAddress = 0x401012;

    /// <summary>An address execution does not touch.</summary>
    static constexpr DWORD_PTR OtherAddress = 0x402000;

    /// <summary>
    /// Address of the first breakpoint exception of the debuggee. It lies with the other test addresses: the
    /// handler ignores breakpoint exceptions at the break-in function of the system library, which is far
    /// away from them.
    /// </summary>
    static constexpr DWORD_PTR SystemBreakpointAddress = 0x400500;

    ExceptionEventHandlerTests()
    {
      auto breakpointFactory = std::make_unique<StrictMock<MockBreakpointFactory>>();
      _breakpointFactory = breakpointFactory.get();
      // No test reaches a member that uses the process handle.
      _breakpoints = std::make_unique<BreakpointManager>(nullptr, std::move(breakpointFactory));

      ON_CALL(_breakpointManager, SetBreakpoint(_)).WillByDefault([this](DWORD_PTR address) { return _breakpoints->SetBreakpoint(address); });
      ON_CALL(_breakpointManager, RemoveBreakpoint(_)).WillByDefault([this](DWORD_PTR address) { return _breakpoints->RemoveBreakpoint(address); });
      ON_CALL(_breakpointManager, HasBreakpoint(_)).WillByDefault([this](DWORD_PTR address) { return _breakpoints->HasBreakpoint(address); });
      ON_CALL(_breakpointManager, HoldBreakpointAt(_)).WillByDefault([this](DWORD_PTR address) { _breakpoints->HoldBreakpointAt(address); });
      ON_CALL(_breakpointManager, ReleaseHeldBreakpoint()).WillByDefault([this] { _breakpoints->ReleaseHeldBreakpoint(); });
      // Any other request - another thread, or clearing the flag - fails the test as an unexpected call.
      EXPECT_CALL(_breakpointManager, SetTrapFlag(IsNull(), true))
        .Times(AnyNumber())
        .WillRepeatedly([this] { _singleStepRequests++; });

      EXPECT_CALL(_eventHandler, OnSingleStep(_))
        .Times(AnyNumber())
        .WillRepeatedly([this] { _stepStops.push_back(_armedAddresses); });

      CREATE_PROCESS_DEBUG_INFO processInfo = {};
      // The cast turns the address value into the pointer type of the field; it is never called.
      processInfo.lpStartAddress = reinterpret_cast<LPTHREAD_START_ROUTINE>(EntryPoint);
      _debugEventStorage.AddProcess(ProcessId, processInfo);
      CREATE_THREAD_DEBUG_INFO threadInfo = {}; // the thread handle stays null
      _debugEventStorage.AddThread(ProcessId, ThreadId, threadInfo);
    }

    /// <summary>
    /// Lets the factory hand out a breakpoint double for the address, exactly once; the factory must be asked
    /// in the order of these calls. The double records in the fixture when its breakpoint instruction is
    /// written and when it is taken out again.
    /// </summary>
    void ExpectBreakpointCreatedFor(DWORD_PTR address)
    {
      auto breakpoint = std::make_unique<NiceMock<MockBreakpoint>>();
      ON_CALL(*breakpoint, Enable()).WillByDefault([this, address]
      {
        _armedAddresses.insert(address);
        _armings[address]++;
      });
      ON_CALL(*breakpoint, Disable()).WillByDefault([this, address] { _armedAddresses.erase(address); });
      std::unique_ptr<IBreakpoint> created = std::move(breakpoint);
      EXPECT_CALL(*_breakpointFactory, Create(address)).InSequence(_creations).WillOnce(Return(ByMove(std::move(created))));
    }

    /// <summary>
    /// Expects one more stop that waits for the client and lets the client answer it with the given command;
    /// the stops are answered in the order of these calls. What the client does while the debuggee is stopped
    /// runs before the answer, as the commands of a stop are executed while the handler waits.
    /// </summary>
    void ExpectStopAnsweredWith(CommandType command, std::function<void()> duringTheStop = [] {})
    {
      EXPECT_CALL(_commandQueue, ProcessCommandsSync())
        .InSequence(_answers)
        .WillOnce(DoAll(InvokeWithoutArgs(std::move(duringTheStop)), Return(command)));
    }

    /// <summary>
    /// Delivers the first breakpoint exception of the debuggee, which the handler takes for the breakpoint
    /// the system raises at process start, and lets the client continue from it. The handler then sets a
    /// breakpoint at the entry point.
    /// </summary>
    void PassTheSystemBreakpoint()
    {
      EXPECT_CALL(_eventHandler, OnSystemBreakpoint(_)).Times(1);
      ExpectStopAnsweredWith(CommandType::CMD_CONTINUE);
      ExpectBreakpointCreatedFor(EntryPoint);
      DeliverBreakpointExceptionAt(SystemBreakpointAddress);
    }

    /// <summary>Tells the handler that the thread executed one instruction and now stands at the address.</summary>
    HandleResult DeliverSingleStepAt(DWORD_PTR address)
    {
      DEBUG_EVENT debugEvent = MakeExceptionEvent(EXCEPTION_SINGLE_STEP, address);
      return _handler.HandleEvent(debugEvent);
    }

    /// <summary>Tells the handler that the thread executed a breakpoint instruction at the address.</summary>
    HandleResult DeliverBreakpointExceptionAt(DWORD_PTR address)
    {
      DEBUG_EVENT debugEvent = MakeExceptionEvent(EXCEPTION_BREAKPOINT, address);
      return _handler.HandleEvent(debugEvent);
    }

    /// <summary>True while the breakpoint instruction of the address would be in the debuggee's memory.</summary>
    bool IsArmed(DWORD_PTR address) const
    {
      return _armedAddresses.contains(address);
    }

    /// <summary>How often the breakpoint instruction of the address was written so far.</summary>
    int ArmingsOf(DWORD_PTR address) const
    {
      auto iter = _armings.find(address);
      return iter == _armings.end() ? 0 : iter->second;
    }

    DebugEventStorage _debugEventStorage;
    StrictMock<MockBreakpointFactory>* _breakpointFactory = nullptr; // owned by _breakpoints
    Sequence _creations; // orders the creations, so one address can be given a second double after a removal
    std::unique_ptr<BreakpointManager> _breakpoints; // the real bookkeeping behind _breakpointManager
    NiceMock<MockBreakpointManager> _breakpointManager; // what the handler and the "client" of a test call
    StrictMock<MockDebuggerEventHandler> _eventHandler;
    EventWaiter _startupWaiter;
    StrictMock<MockDebuggerCommandQueue> _commandQueue;
    Sequence _answers; // orders the answers of the client to the stops
    StrictMock<MockDisassembler> _disassembler;

    std::set<DWORD_PTR> _armedAddresses; // addresses whose breakpoint instruction is written
    std::map<DWORD_PTR, int> _armings; // per address, how often its breakpoint instruction was written
    int _singleStepRequests = 0; // how often the handler asked for a single step of the thread
    std::vector<std::set<DWORD_PTR>> _stepStops; // per reported step stop, the armed addresses at that moment

    ExceptionEventHandler _handler{ _debugEventStorage, _breakpointManager, _eventHandler, _startupWaiter, _commandQueue, _disassembler };

  private:
    static DEBUG_EVENT MakeExceptionEvent(DWORD exceptionCode, DWORD_PTR address)
    {
      DEBUG_EVENT debugEvent = {};
      debugEvent.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
      debugEvent.dwProcessId = ProcessId;
      debugEvent.dwThreadId = ThreadId;
      debugEvent.u.Exception.ExceptionRecord.ExceptionCode = exceptionCode;
      // The cast turns the address value into the pointer type of the field; it is never dereferenced.
      debugEvent.u.Exception.ExceptionRecord.ExceptionAddress = reinterpret_cast<PVOID>(address);
      return debugEvent;
    }
  };

  TEST_F(ExceptionEventHandlerTests, StepIntoAtAStepStopKeepsABreakpointSetBehindItUnarmedUntilTheInstructionRan)
  {
    // Arrange
    ExpectStopAnsweredWith(CommandType::CMD_STEP_INTO); // the step stop at the stop address
    ExpectStopAnsweredWith(CommandType::CMD_STOP); // the step stop the step into ends with
    ExpectBreakpointCreatedFor(StopAddress);

    // Act: the debuggee stops at the stop address and the client answers with step into
    HandleResult resumed = DeliverSingleStepAt(StopAddress);

    // Assert: the instruction at the stop address is executed with a single step
    EXPECT_EQ(resumed.ContinueStatus, DBG_CONTINUE);
    EXPECT_EQ(_singleStepRequests, 1);

    // Act: a breakpoint is set at the stop address before the debuggee is resumed
    _breakpointManager.SetBreakpoint(StopAddress);

    // Assert
    EXPECT_FALSE(IsArmed(StopAddress));

    // Act: the instruction at the stop address ran
    DeliverSingleStepAt(NextAddress);

    // Assert: the breakpoint was written once, before the one step stop of the step into was reported
    EXPECT_EQ(ArmingsOf(StopAddress), 1);
    ASSERT_EQ(_stepStops.size(), 2u);
    EXPECT_TRUE(_stepStops[1].contains(StopAddress));
  }

  TEST_F(ExceptionEventHandlerTests, StepOverAtAStepStopKeepsABreakpointSetBehindItUnarmedUntilTheInstructionRan)
  {
    // Arrange
    ExpectStopAnsweredWith(CommandType::CMD_STEP_OVER); // the step stop at the stop address
    ExpectStopAnsweredWith(CommandType::CMD_STOP); // the step stop the step over ends with
    EXPECT_CALL(_disassembler, IsCallInstruction(_, StopAddress, _)).WillOnce(Return(false));
    ExpectBreakpointCreatedFor(StopAddress);

    // Act: the debuggee stops at the stop address and the client answers with step over
    HandleResult resumed = DeliverSingleStepAt(StopAddress);

    // Assert: the instruction at the stop address is executed with a single step
    EXPECT_EQ(resumed.ContinueStatus, DBG_CONTINUE);
    EXPECT_EQ(_singleStepRequests, 1);

    // Act: a breakpoint is set at the stop address before the debuggee is resumed
    _breakpointManager.SetBreakpoint(StopAddress);

    // Assert
    EXPECT_FALSE(IsArmed(StopAddress));

    // Act: the instruction at the stop address ran
    DeliverSingleStepAt(NextAddress);

    // Assert: the breakpoint was written once, before the one step stop of the step over was reported
    EXPECT_EQ(ArmingsOf(StopAddress), 1);
    ASSERT_EQ(_stepStops.size(), 2u);
    EXPECT_TRUE(_stepStops[1].contains(StopAddress));
  }

  TEST_F(ExceptionEventHandlerTests, ContinueAtAStepStopKeepsABreakpointSetBehindItUnarmedUntilTheInstructionRan)
  {
    // Arrange
    ExpectStopAnsweredWith(CommandType::CMD_CONTINUE); // the step stop at the stop address; no stop follows
    ExpectBreakpointCreatedFor(StopAddress);

    // Act: the debuggee stops at the stop address and the client answers with continue
    HandleResult resumed = DeliverSingleStepAt(StopAddress);

    // Assert: the instruction at the stop address is executed with a single step
    EXPECT_EQ(resumed.ContinueStatus, DBG_CONTINUE);
    EXPECT_EQ(_singleStepRequests, 1);

    // Act: a breakpoint is set at the stop address before the debuggee is resumed
    _breakpointManager.SetBreakpoint(StopAddress);

    // Assert
    EXPECT_FALSE(IsArmed(StopAddress));

    // Act: the instruction at the stop address ran
    HandleResult ranOn = DeliverSingleStepAt(NextAddress);

    // Assert: the breakpoint was written once and the debuggee runs on without a stop being reported
    EXPECT_EQ(ArmingsOf(StopAddress), 1);
    EXPECT_TRUE(IsArmed(StopAddress));
    EXPECT_EQ(ranOn.ContinueStatus, DBG_CONTINUE);
    EXPECT_EQ(_stepStops.size(), 1u);
  }

  TEST_F(ExceptionEventHandlerTests, StepOverACallAtAStepStopKeepsABreakpointSetBehindItUnarmedAndRunsOnToTheReturnBreakpoint)
  {
    // Arrange
    const size_t callLength = 5;
    const DWORD_PTR returnAddress = StopAddress + callLength;
    const DWORD_PTR calleeAddress = 0x403000;
    ExpectStopAnsweredWith(CommandType::CMD_STEP_OVER); // the step stop at the call; no stop follows in the callee
    EXPECT_CALL(_disassembler, IsCallInstruction(_, StopAddress, _)).WillOnce(DoAll(SetArgReferee<2>(callLength), Return(true)));
    ExpectBreakpointCreatedFor(returnAddress);
    ExpectBreakpointCreatedFor(StopAddress);

    // Act: the debuggee stops at the call and the client answers with step over
    HandleResult resumed = DeliverSingleStepAt(StopAddress);

    // Assert: the breakpoint behind the call is written at once, and the call is executed with a single step
    EXPECT_EQ(resumed.ContinueStatus, DBG_CONTINUE);
    EXPECT_TRUE(IsArmed(returnAddress));
    EXPECT_EQ(_singleStepRequests, 1);

    // Act: a breakpoint is set at the stop address before the debuggee is resumed
    _breakpointManager.SetBreakpoint(StopAddress);

    // Assert
    EXPECT_FALSE(IsArmed(StopAddress));

    // Act: the call ran, the thread stands at the first instruction of the called function
    HandleResult ranOn = DeliverSingleStepAt(calleeAddress);

    // Assert: the breakpoint was written once and the debuggee runs on to the breakpoint behind the call
    EXPECT_EQ(ArmingsOf(StopAddress), 1);
    EXPECT_TRUE(IsArmed(StopAddress));
    EXPECT_EQ(ranOn.ContinueStatus, DBG_CONTINUE);
    EXPECT_EQ(_stepStops.size(), 1u);
  }

  TEST_F(ExceptionEventHandlerTests, ContinueAtABreakpointStopWhoseBreakpointWasRemovedKeepsABreakpointSetBehindItUnarmedUntilTheInstructionRan)
  {
    // Arrange
    ExpectBreakpointCreatedFor(StopAddress); // the breakpoint the debuggee stops at
    _breakpointManager.SetBreakpoint(StopAddress);
    PassTheSystemBreakpoint();
    EXPECT_CALL(_eventHandler, OnBreakpoint(_)).Times(1);
    // The client removes the breakpoint while the debuggee is stopped at it, then continues.
    ExpectStopAnsweredWith(CommandType::CMD_CONTINUE, [this] { _breakpointManager.RemoveBreakpoint(StopAddress); });
    ExpectBreakpointCreatedFor(StopAddress); // the breakpoint that is set again behind the go command

    // Act: the debuggee hits the breakpoint at the stop address
    HandleResult resumed = DeliverBreakpointExceptionAt(StopAddress);

    // Assert: the instruction at the stop address is executed with a single step
    EXPECT_EQ(resumed.ContinueStatus, DBG_CONTINUE);
    EXPECT_EQ(_singleStepRequests, 1);

    // Act: a breakpoint is set at the stop address again before the debuggee is resumed
    _breakpointManager.SetBreakpoint(StopAddress);

    // Assert
    EXPECT_FALSE(IsArmed(StopAddress));

    // Act: the instruction at the stop address ran
    HandleResult ranOn = DeliverSingleStepAt(NextAddress);

    // Assert: the breakpoint is written and the debuggee runs on without a stop being reported
    EXPECT_TRUE(IsArmed(StopAddress));
    EXPECT_EQ(ranOn.ContinueStatus, DBG_CONTINUE);
    EXPECT_TRUE(_stepStops.empty());
  }

  TEST_F(ExceptionEventHandlerTests, ABreakpointSetBehindTheGoCommandAndRemovedAgainBeforeTheInstructionRanWritesNothing)
  {
    // Arrange
    ExpectStopAnsweredWith(CommandType::CMD_CONTINUE);
    // The strict factory fails the test when the end of the single step asks it for a breakpoint.
    ExpectBreakpointCreatedFor(StopAddress);
    DeliverSingleStepAt(StopAddress);
    _breakpointManager.SetBreakpoint(StopAddress);
    _breakpointManager.RemoveBreakpoint(StopAddress);

    // Act: the instruction at the stop address ran
    DeliverSingleStepAt(NextAddress);

    // Assert
    EXPECT_EQ(ArmingsOf(StopAddress), 0);
    EXPECT_FALSE(_breakpointManager.HasBreakpoint(StopAddress));
  }

  TEST_F(ExceptionEventHandlerTests, ABreakpointSetAtTheStopAddressAfterTheInstructionRanIsArmedAtOnce)
  {
    // Arrange
    ExpectStopAnsweredWith(CommandType::CMD_CONTINUE);
    ExpectBreakpointCreatedFor(StopAddress);
    DeliverSingleStepAt(StopAddress);
    DeliverSingleStepAt(NextAddress);

    // Act
    _breakpointManager.SetBreakpoint(StopAddress);

    // Assert
    EXPECT_TRUE(IsArmed(StopAddress));
  }

  TEST_F(ExceptionEventHandlerTests, ABreakpointSetAtAnotherAddressBehindTheGoCommandIsArmedAtOnce)
  {
    // Arrange
    ExpectStopAnsweredWith(CommandType::CMD_CONTINUE);
    ExpectBreakpointCreatedFor(OtherAddress);
    DeliverSingleStepAt(StopAddress);

    // Act
    _breakpointManager.SetBreakpoint(OtherAddress);

    // Assert
    EXPECT_TRUE(IsArmed(OtherAddress));
  }
}
