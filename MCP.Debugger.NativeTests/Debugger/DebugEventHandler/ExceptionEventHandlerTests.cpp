#include "Mocks/Debugger/Interfaces/MockBreakpoint.h"
#include "Mocks/Debugger/Interfaces/MockBreakpointFactory.h"
#include "Mocks/Debugger/Interfaces/MockBreakpointManager.h"
#include "Mocks/Debugger/Interfaces/MockDebuggerCommandQueue.h"
#include "Mocks/Debugger/Interfaces/MockDebuggerEventHandler.h"
#include "Mocks/Debugger/Interfaces/MockResumePlan.h"
#include "Mocks/Debugger/Interfaces/MockStopSiteFactory.h"
#include "Mocks/Disassembler/Interfaces/MockDisassembler.h"
#include "Native/Debugger/BreakpointManager.h"
#include "Native/Debugger/Commands/ContinuePlan.h"
#include "Native/Debugger/Commands/StepIntoPlan.h"
#include "Native/Debugger/Commands/StepOverPlan.h"
#include "Native/Debugger/DebugEventHandler/DebugEventStorage.h"
#include "Native/Debugger/DebugEventHandler/ExceptionEventHandler.h"
#include "Native/Debugger/DebugEventHandler/HandleResult.h"
#include "Native/Debugger/Interfaces/IResumePlan.h"
#include "Native/Debugger/Interfaces/IStopSite.h"
#include "Native/Debugger/Interfaces/IStopSiteFactory.h"
#include "Native/Debugger/Interfaces/StopOutcome.h"
#include "Native/Debugger/StopSite.h"
#include "Native/Threading/EventWaiter.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
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
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockResumePlan;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockStopSiteFactory;
  using MCP::Native::Tests::Mocks::Disassembler::Interfaces::MockDisassembler;
  using ::testing::_;
  using ::testing::AnyNumber;
  using ::testing::ByMove;
  using ::testing::DoAll;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;
  using ::testing::IsNull;
  using ::testing::NiceMock;
  using ::testing::Pair;
  using ::testing::Return;
  using ::testing::Sequence;
  using ::testing::SetArgReferee;
  using ::testing::StrictMock;

  using BreakpointManager = MCP::Native::Debugger::BreakpointManager;
  using ContinuePlan = MCP::Native::Debugger::Commands::ContinuePlan;
  using DebugEventStorage = MCP::Native::Debugger::DebugEventHandler::DebugEventStorage;
  using EventWaiter = MCP::Native::Threading::EventWaiter;
  using ExceptionEventHandler = MCP::Native::Debugger::DebugEventHandler::ExceptionEventHandler;
  using HandleResult = MCP::Native::Debugger::DebugEventHandler::HandleResult;
  using IBreakpoint = MCP::Native::Debugger::Interfaces::IBreakpoint;
  using IResumePlan = MCP::Native::Debugger::Interfaces::IResumePlan;
  using IStopSite = MCP::Native::Debugger::Interfaces::IStopSite;
  using IStopSiteFactory = MCP::Native::Debugger::Interfaces::IStopSiteFactory;
  using StepIntoPlan = MCP::Native::Debugger::Commands::StepIntoPlan;
  using StepOverPlan = MCP::Native::Debugger::Commands::StepOverPlan;
  using StopOutcome = MCP::Native::Debugger::Interfaces::StopOutcome;
  using StopSite = MCP::Native::Debugger::StopSite;

  /// <summary>
  /// Tests what the exception handler does at the stops of the debuggee: to breakpoint instructions around
  /// the command that resumes it, and to the plan that command leaves. No debuggee runs: the test delivers
  /// the debug events itself, and a single-step exception at an address stands for "the thread executed one
  /// instruction and stopped there". The client's answer to a stop is the plan of a resuming command - the
  /// real plans, or a double where the test is about what the handler asks a plan - or the end of the session.
  ///
  /// The breakpoint bookkeeping is the real breakpoint manager, because the behaviour under test is the
  /// interplay of the handler's hold and release with a set that arrives in between. Its breakpoints are
  /// doubles that record whether their breakpoint instruction would be in the debuggee's memory. The handler
  /// reaches the manager through a double that passes every call on except the trap flag, which would need a
  /// thread; the double counts the single steps the handler asks for instead. The stop sites are the real
  /// ones too, over that manager double and the disassembler double: the factory double the handler owns
  /// creates them and records for which process and address it was asked.
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

    /// <summary>
    /// A value that stands for the handle of the debuggee process. It is only passed on and compared, never
    /// used: the disassembler that would read the process with it is a double.
    /// </summary>
    static HANDLE FakeProcessHandle()
    {
      // The cast builds a handle value from a number; no test hands it to the system.
      return reinterpret_cast<HANDLE>(static_cast<DWORD_PTR>(0x1234));
    }

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
      processInfo.hProcess = FakeProcessHandle();
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
    /// Expects one more stop that waits for the client and lets the client answer it with a command that
    /// resumes the debuggee with the given plan; the stops are answered in the order of these calls. What the
    /// client does while the debuggee is stopped runs before the answer, as the commands of a stop are
    /// executed while the handler waits.
    /// </summary>
    void ExpectStopAnsweredWith(std::unique_ptr<IResumePlan> plan, std::function<void()> duringTheStop = [] {})
    {
      EXPECT_CALL(_commandQueue, ProcessCommandsSync())
        .InSequence(_answers)
        .WillOnce([plan = std::move(plan), duringTheStop = std::move(duringTheStop)]() mutable
        {
          duringTheStop();
          return StopOutcome::Resumed(std::move(plan));
        });
    }

    /// <summary>
    /// Expects one more stop that waits for the client and lets the client answer it by ending the session.
    /// </summary>
    void ExpectStopAnsweredWithTheEndOfTheSession()
    {
      EXPECT_CALL(_commandQueue, ProcessCommandsSync())
        .InSequence(_answers)
        .WillOnce([] { return StopOutcome::SessionEnded(); });
    }

    /// <summary>
    /// Delivers the first breakpoint exception of the debuggee, which the handler takes for the breakpoint
    /// the system raises at process start, and lets the client continue from it. The handler then sets a
    /// breakpoint at the entry point.
    /// </summary>
    void PassTheSystemBreakpoint()
    {
      EXPECT_CALL(_eventHandler, OnSystemBreakpoint(_)).Times(1);
      ExpectStopAnsweredWith(std::make_unique<ContinuePlan>());
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
    std::vector<std::pair<HANDLE, DWORD_PTR>> _stopSites; // per created stop site, its process and stop address

    // Declared behind everything the factory of the stop sites uses and records into: creating the handler
    // creates that factory.
    ExceptionEventHandler _handler{ _debugEventStorage, _breakpointManager, _eventHandler, _startupWaiter, _commandQueue, CreateStopSiteFactory() };

  private:
    /// <summary>
    /// Creates the factory the handler owns. It hands out real stop sites that act on the breakpoint manager
    /// double and ask the disassembler double, so that what a plan does at a stop reaches the same doubles as
    /// what the handler does itself.
    /// </summary>
    std::unique_ptr<IStopSiteFactory> CreateStopSiteFactory()
    {
      auto stopSiteFactory = std::make_unique<NiceMock<MockStopSiteFactory>>();
      ON_CALL(*stopSiteFactory, Create(_, _)).WillByDefault([this](HANDLE processHandle, DWORD_PTR stopAddress)
      {
        _stopSites.emplace_back(processHandle, stopAddress);
        std::unique_ptr<IStopSite> stopSite = std::make_unique<StopSite>(processHandle, stopAddress, _breakpointManager, _disassembler);
        return stopSite;
      });
      return stopSiteFactory;
    }

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
    ExpectStopAnsweredWith(std::make_unique<StepIntoPlan>()); // the step stop at the stop address
    ExpectStopAnsweredWithTheEndOfTheSession(); // the step stop the step into ends with
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
    ExpectStopAnsweredWith(std::make_unique<StepOverPlan>()); // the step stop at the stop address
    ExpectStopAnsweredWithTheEndOfTheSession(); // the step stop the step over ends with
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
    ExpectStopAnsweredWith(std::make_unique<ContinuePlan>()); // the step stop at the stop address; no stop follows
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
    ExpectStopAnsweredWith(std::make_unique<StepOverPlan>()); // the step stop at the call; no stop follows in the callee
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
    ExpectStopAnsweredWith(std::make_unique<ContinuePlan>(), [this] { _breakpointManager.RemoveBreakpoint(StopAddress); });
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
    ExpectStopAnsweredWith(std::make_unique<ContinuePlan>());
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
    ExpectStopAnsweredWith(std::make_unique<ContinuePlan>());
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
    ExpectStopAnsweredWith(std::make_unique<ContinuePlan>());
    ExpectBreakpointCreatedFor(OtherAddress);
    DeliverSingleStepAt(StopAddress);

    // Act
    _breakpointManager.SetBreakpoint(OtherAddress);

    // Assert
    EXPECT_TRUE(IsArmed(OtherAddress));
  }

  /// <summary>One command that resumes the debuggee: its name and the plan it leaves.</summary>
  struct ResumingAnswerCase
  {
    const char* Name;
    std::unique_ptr<IResumePlan> (*CreatePlan)();
  };

  /// <summary>Prints the case by its name; without it Google Test would dump the bytes of the struct.</summary>
  void PrintTo(const ResumingAnswerCase& resumingAnswerCase, std::ostream* output)
  {
    *output << resumingAnswerCase.Name;
  }

  /// <summary>
  /// The commands that resume the debuggee, each as the answer of the client to the stop at the system
  /// breakpoint.
  /// </summary>
  class ExceptionEventHandlerSystemBreakpointTests : public ExceptionEventHandlerTests,
                                                     public ::testing::WithParamInterface<ResumingAnswerCase>
  {
  };

  TEST_P(ExceptionEventHandlerSystemBreakpointTests, AGoCommandAtTheSystemBreakpointSetsTheEntryPointBreakpointWithoutASingleStep)
  {
    // Arrange
    EXPECT_CALL(_eventHandler, OnSystemBreakpoint(_)).Times(1);
    ExpectStopAnsweredWith(GetParam().CreatePlan());
    ExpectBreakpointCreatedFor(EntryPoint);

    // Act
    HandleResult resumed = DeliverBreakpointExceptionAt(SystemBreakpointAddress);

    // Assert
    EXPECT_EQ(resumed.ContinueStatus, static_cast<DWORD>(DBG_EXCEPTION_NOT_HANDLED));
    EXPECT_TRUE(IsArmed(EntryPoint));
    EXPECT_EQ(ArmingsOf(EntryPoint), 1);
    EXPECT_EQ(_singleStepRequests, 0);
  }

  INSTANTIATE_TEST_SUITE_P(
    GoCommands,
    ExceptionEventHandlerSystemBreakpointTests,
    ::testing::Values(
      ResumingAnswerCase{ "Continue", []() -> std::unique_ptr<IResumePlan> { return std::make_unique<ContinuePlan>(); } },
      ResumingAnswerCase{ "StepInto", []() -> std::unique_ptr<IResumePlan> { return std::make_unique<StepIntoPlan>(); } },
      ResumingAnswerCase{ "StepOver", []() -> std::unique_ptr<IResumePlan> { return std::make_unique<StepOverPlan>(); } }),
    [](const ::testing::TestParamInfo<ResumingAnswerCase>& info) { return std::string(info.param.Name); });

  TEST_F(ExceptionEventHandlerTests, StopAtTheSystemBreakpointSetsNoBreakpoint)
  {
    // Arrange
    EXPECT_CALL(_eventHandler, OnSystemBreakpoint(_)).Times(1);
    ExpectStopAnsweredWithTheEndOfTheSession();

    // Act
    HandleResult ended = DeliverBreakpointExceptionAt(SystemBreakpointAddress);

    // Assert: the strict factory fails the test when a breakpoint is created
    EXPECT_EQ(ended.ContinueStatus, static_cast<DWORD>(DBG_EXCEPTION_NOT_HANDLED));
    EXPECT_FALSE(_breakpointManager.HasBreakpoint(EntryPoint));
    EXPECT_EQ(_singleStepRequests, 0);
  }

  TEST_F(ExceptionEventHandlerTests, TheHitOfTheBreakpointBehindASteppedOverCallRemovesItAndReportsOneStepStop)
  {
    // Arrange
    const size_t callLength = 5;
    const DWORD_PTR returnAddress = StopAddress + callLength;
    const DWORD_PTR calleeAddress = 0x403000;
    PassTheSystemBreakpoint();
    ExpectStopAnsweredWith(std::make_unique<StepOverPlan>()); // the step stop at the call
    ExpectStopAnsweredWithTheEndOfTheSession(); // the step stop behind the call
    EXPECT_CALL(_disassembler, IsCallInstruction(_, StopAddress, _)).WillOnce(DoAll(SetArgReferee<2>(callLength), Return(true)));
    ExpectBreakpointCreatedFor(returnAddress);
    DeliverSingleStepAt(StopAddress);
    DeliverSingleStepAt(calleeAddress); // the call ran; the debuggee runs on inside the called function

    // Act: the called function returned
    HandleResult stopped = DeliverBreakpointExceptionAt(returnAddress);

    // Assert
    EXPECT_EQ(stopped.ContinueStatus, static_cast<DWORD>(DBG_CONTINUE));
    EXPECT_FALSE(_breakpointManager.HasBreakpoint(returnAddress));
    EXPECT_FALSE(IsArmed(returnAddress));
    EXPECT_EQ(_stepStops.size(), 2u);
  }

  TEST_F(ExceptionEventHandlerTests, StopAtAStepStopRequestsNoSingleStep)
  {
    // Arrange
    ExpectStopAnsweredWithTheEndOfTheSession();

    // Act
    HandleResult ended = DeliverSingleStepAt(StopAddress);

    // Assert
    EXPECT_EQ(ended.ContinueStatus, static_cast<DWORD>(DBG_CONTINUE));
    EXPECT_EQ(_singleStepRequests, 0);
    EXPECT_EQ(_stepStops.size(), 1u);
  }

  TEST_F(ExceptionEventHandlerTests, AStepOverInterruptedByABreakpointInTheCalledFunctionLeavesNoBreakpointBehindTheCall)
  {
    // Arrange
    const size_t callLength = 5;
    const DWORD_PTR returnAddress = StopAddress + callLength;
    const DWORD_PTR calleeAddress = 0x403000;
    const DWORD_PTR breakpointInCallee = 0x403004;
    ExpectBreakpointCreatedFor(breakpointInCallee);
    _breakpointManager.SetBreakpoint(breakpointInCallee);
    PassTheSystemBreakpoint();
    ExpectStopAnsweredWith(std::make_unique<StepOverPlan>()); // the step stop at the call
    EXPECT_CALL(_disassembler, IsCallInstruction(_, StopAddress, _)).WillOnce(DoAll(SetArgReferee<2>(callLength), Return(true)));
    ExpectBreakpointCreatedFor(returnAddress);
    bool setWhenTheStopWasReported = false;
    bool armedWhenTheStopWasReported = false;
    EXPECT_CALL(_eventHandler, OnBreakpoint(_)).WillOnce([&]
    {
      setWhenTheStopWasReported = _breakpointManager.HasBreakpoint(returnAddress);
      armedWhenTheStopWasReported = IsArmed(returnAddress);
    });
    ExpectStopAnsweredWith(std::make_unique<ContinuePlan>()); // the breakpoint stop inside the called function
    DeliverSingleStepAt(StopAddress);
    DeliverSingleStepAt(calleeAddress); // the call ran; the debuggee runs on inside the called function

    // Act: the breakpoint inside the called function is hit and the client continues
    DeliverBreakpointExceptionAt(breakpointInCallee);

    // Assert
    EXPECT_FALSE(setWhenTheStopWasReported);
    EXPECT_FALSE(armedWhenTheStopWasReported);
    EXPECT_FALSE(_breakpointManager.HasBreakpoint(returnAddress));
  }

  // The cases below answer a stop with a plan the handler cannot know - a strict double: every question the
  // test does not expect fails the test, and so does a plan that is destroyed with an expected call missing.

  TEST_F(ExceptionEventHandlerTests, APlanIsBegunOnceAtTheStopBeforeTheSingleStepIsRequested)
  {
    // Arrange
    DWORD_PTR stopAddressSeenByThePlan = 0;
    int singleStepRequestsWhenThePlanBegan = -1;
    auto plan = std::make_unique<StrictMock<MockResumePlan>>();
    EXPECT_CALL(*plan, Begin(_)).WillOnce([&](IStopSite& stopSite)
    {
      stopAddressSeenByThePlan = stopSite.GetStopAddress();
      singleStepRequestsWhenThePlanBegan = _singleStepRequests;
    });
    ExpectStopAnsweredWith(std::move(plan));

    // Act
    HandleResult resumed = DeliverSingleStepAt(StopAddress);

    // Assert
    EXPECT_EQ(stopAddressSeenByThePlan, StopAddress);
    EXPECT_EQ(singleStepRequestsWhenThePlanBegan, 0);
    EXPECT_EQ(_singleStepRequests, 1);
    EXPECT_EQ(resumed.ContinueStatus, static_cast<DWORD>(DBG_CONTINUE));
  }

  TEST_F(ExceptionEventHandlerTests, APlanThatHaltsAfterTheFirstInstructionGetsAStepStopWhenTheInstructionRan)
  {
    // Arrange
    auto plan = std::make_unique<StrictMock<MockResumePlan>>();
    EXPECT_CALL(*plan, Begin(_)).Times(1);
    EXPECT_CALL(*plan, HaltsAfterFirstInstruction()).WillOnce(Return(true));
    EXPECT_CALL(*plan, End(_)).Times(1);
    ExpectStopAnsweredWith(std::move(plan)); // the step stop at the stop address
    ExpectStopAnsweredWithTheEndOfTheSession(); // the step stop the plan asks for
    DeliverSingleStepAt(StopAddress);

    // Act: the instruction at the stop address ran
    DeliverSingleStepAt(NextAddress);

    // Assert
    EXPECT_EQ(_stepStops.size(), 2u);
  }

  TEST_F(ExceptionEventHandlerTests, APlanThatDoesNotHaltLetsTheDebuggeeRunOnWhenTheInstructionRan)
  {
    // Arrange
    auto plan = std::make_unique<StrictMock<MockResumePlan>>();
    EXPECT_CALL(*plan, Begin(_)).Times(1);
    EXPECT_CALL(*plan, HaltsAfterFirstInstruction()).WillOnce(Return(false));
    ExpectStopAnsweredWith(std::move(plan)); // the step stop at the stop address; no stop follows
    DeliverSingleStepAt(StopAddress);

    // Act: the instruction at the stop address ran
    HandleResult ranOn = DeliverSingleStepAt(NextAddress);

    // Assert: the strict queue fails the test when the handler waits for the client again
    EXPECT_EQ(ranOn.ContinueStatus, static_cast<DWORD>(DBG_CONTINUE));
    EXPECT_EQ(_stepStops.size(), 1u);
  }

  TEST_F(ExceptionEventHandlerTests, ABreakpointExceptionAtAnAddressThePlanOwnsEndsThePlanAndIsReportedAsAStepStop)
  {
    // Arrange
    const DWORD_PTR ownedAddress = 0x401020;
    PassTheSystemBreakpoint();
    auto plan = std::make_unique<StrictMock<MockResumePlan>>();
    EXPECT_CALL(*plan, Begin(_)).Times(1);
    EXPECT_CALL(*plan, HaltsAfterFirstInstruction()).WillOnce(Return(false));
    EXPECT_CALL(*plan, OwnsBreakpointAt(ownedAddress)).WillOnce(Return(true));
    EXPECT_CALL(*plan, End(_)).Times(1);
    ExpectStopAnsweredWith(std::move(plan)); // the step stop at the stop address
    ExpectStopAnsweredWithTheEndOfTheSession(); // the step stop at the address the plan owns
    DeliverSingleStepAt(StopAddress);
    DeliverSingleStepAt(NextAddress); // the instruction at the stop address ran; the debuggee runs on

    // Act: the debuggee executes a breakpoint instruction at the address the plan claims
    HandleResult stopped = DeliverBreakpointExceptionAt(ownedAddress);

    // Assert: a step stop and not a breakpoint stop - the strict event handler expects no breakpoint event,
    // and the manager, which has no breakpoint there, was not what decided
    EXPECT_EQ(stopped.ContinueStatus, static_cast<DWORD>(DBG_CONTINUE));
    EXPECT_EQ(_stepStops.size(), 2u);
    EXPECT_THAT(_stopSites, ElementsAre(Pair(FakeProcessHandle(), StopAddress), Pair(FakeProcessHandle(), ownedAddress)));
  }

  TEST_F(ExceptionEventHandlerTests, APlanIsEndedOnceAtTheStepStopThatFollowsItBeforeThatStopIsReported)
  {
    // Arrange
    size_t stepStopsReportedWhenThePlanEnded = 0;
    DWORD_PTR stopAddressOfTheEndingStop = 0;
    auto plan = std::make_unique<StrictMock<MockResumePlan>>();
    EXPECT_CALL(*plan, Begin(_)).Times(1);
    EXPECT_CALL(*plan, HaltsAfterFirstInstruction()).WillOnce(Return(true));
    EXPECT_CALL(*plan, End(_)).WillOnce([&](IStopSite& stopSite)
    {
      stepStopsReportedWhenThePlanEnded = _stepStops.size();
      stopAddressOfTheEndingStop = stopSite.GetStopAddress();
    });
    ExpectStopAnsweredWith(std::move(plan)); // the step stop at the stop address
    ExpectStopAnsweredWithTheEndOfTheSession(); // the step stop that ends the plan
    DeliverSingleStepAt(StopAddress);

    // Act: the instruction at the stop address ran
    DeliverSingleStepAt(NextAddress);

    // Assert: when the plan ended, only the stop it was begun at had been reported
    EXPECT_EQ(stepStopsReportedWhenThePlanEnded, 1u);
    EXPECT_EQ(stopAddressOfTheEndingStop, NextAddress);
    EXPECT_EQ(_stepStops.size(), 2u);
  }

  TEST_F(ExceptionEventHandlerTests, APlanIsEndedOnceAtTheBreakpointStopThatFollowsItBeforeThatStopIsReported)
  {
    // Arrange
    ExpectBreakpointCreatedFor(OtherAddress);
    _breakpointManager.SetBreakpoint(OtherAddress);
    PassTheSystemBreakpoint();
    bool planEnded = false;
    bool planEndedWhenTheStopWasReported = false;
    auto plan = std::make_unique<StrictMock<MockResumePlan>>();
    EXPECT_CALL(*plan, Begin(_)).Times(1);
    EXPECT_CALL(*plan, HaltsAfterFirstInstruction()).WillOnce(Return(false));
    EXPECT_CALL(*plan, OwnsBreakpointAt(OtherAddress)).WillOnce(Return(false));
    EXPECT_CALL(*plan, End(_)).WillOnce([&] { planEnded = true; });
    ExpectStopAnsweredWith(std::move(plan)); // the step stop at the stop address
    EXPECT_CALL(_eventHandler, OnBreakpoint(_)).WillOnce([&] { planEndedWhenTheStopWasReported = planEnded; });
    ExpectStopAnsweredWithTheEndOfTheSession(); // the breakpoint stop that ends the plan
    DeliverSingleStepAt(StopAddress);
    DeliverSingleStepAt(NextAddress); // the instruction at the stop address ran; the debuggee runs on

    // Act: the debuggee hits the breakpoint of the client
    DeliverBreakpointExceptionAt(OtherAddress);

    // Assert
    EXPECT_TRUE(planEndedWhenTheStopWasReported);
  }

  TEST_F(ExceptionEventHandlerTests, TheEndOfTheSingleStepLetsTheDebuggeeRunOnWhenAnotherStopEndedThePlanMeanwhile)
  {
    // Arrange
    ExpectBreakpointCreatedFor(OtherAddress);
    _breakpointManager.SetBreakpoint(OtherAddress);
    PassTheSystemBreakpoint();
    auto plan = std::make_unique<StrictMock<MockResumePlan>>();
    EXPECT_CALL(*plan, Begin(_)).Times(1);
    EXPECT_CALL(*plan, OwnsBreakpointAt(OtherAddress)).WillOnce(Return(false));
    EXPECT_CALL(*plan, End(_)).Times(1);
    ExpectStopAnsweredWith(std::move(plan)); // the step stop at the stop address
    EXPECT_CALL(_eventHandler, OnBreakpoint(_)).Times(1);
    ExpectStopAnsweredWithTheEndOfTheSession(); // the breakpoint stop in between
    DeliverSingleStepAt(StopAddress);
    // Another thread hits a breakpoint before the single step of the resumed thread has ended.
    DeliverBreakpointExceptionAt(OtherAddress);

    // Act: the single step ends
    HandleResult ranOn = DeliverSingleStepAt(NextAddress);

    // Assert: the plan that could have asked for a halt is gone, so no stop is reported
    EXPECT_EQ(ranOn.ContinueStatus, static_cast<DWORD>(DBG_CONTINUE));
    EXPECT_EQ(_stepStops.size(), 1u);
  }

  TEST_F(ExceptionEventHandlerTests, APlanThatAnswersTheSystemBreakpointIsDroppedWithoutBeingAsked)
  {
    // Arrange
    EXPECT_CALL(_eventHandler, OnSystemBreakpoint(_)).Times(1);
    ExpectStopAnsweredWith(std::make_unique<StrictMock<MockResumePlan>>());
    ExpectBreakpointCreatedFor(EntryPoint);
    DeliverBreakpointExceptionAt(SystemBreakpointAddress);

    // Act: a breakpoint instruction of the program itself; a plan that were still current would be asked
    // whether the breakpoint is its own
    HandleResult unknown = DeliverBreakpointExceptionAt(OtherAddress);

    // Assert
    EXPECT_EQ(unknown.ContinueStatus, static_cast<DWORD>(DBG_EXCEPTION_NOT_HANDLED));
    EXPECT_THAT(_stopSites, IsEmpty());
    EXPECT_EQ(_singleStepRequests, 0);
  }

  TEST_F(ExceptionEventHandlerTests, EveryReportedStopGetsOneStopSiteForTheProcessAndTheAddressOfTheStop)
  {
    // Arrange
    ExpectBreakpointCreatedFor(OtherAddress);
    _breakpointManager.SetBreakpoint(OtherAddress);
    PassTheSystemBreakpoint(); // reported, but no stop a plan could be begun at: it gets no stop site
    ExpectStopAnsweredWith(std::make_unique<ContinuePlan>()); // the step stop at the stop address
    EXPECT_CALL(_eventHandler, OnBreakpoint(_)).Times(1);
    ExpectStopAnsweredWithTheEndOfTheSession(); // the breakpoint stop

    // Act
    DeliverSingleStepAt(StopAddress);
    DeliverSingleStepAt(NextAddress); // the end of the single step is no stop
    DeliverBreakpointExceptionAt(OtherAddress);

    // Assert
    EXPECT_THAT(_stopSites, ElementsAre(Pair(FakeProcessHandle(), StopAddress), Pair(FakeProcessHandle(), OtherAddress)));
  }
}
