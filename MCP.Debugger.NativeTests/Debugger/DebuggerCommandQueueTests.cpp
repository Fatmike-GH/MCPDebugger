#include "Mocks/Debugger/Interfaces/MockBreakpointManager.h"
#include "Mocks/Debugger/Interfaces/MockDebuggeeBreakIn.h"
#include "Mocks/Disassembler/Interfaces/MockDisassembler.h"
#include "Mocks/Disassembler/Interfaces/MockSymbolResolver.h"
#include "Native/Debugger/Commands/CommandResult.h"
#include "Native/Debugger/DebuggerCommandQueue.h"
#include "Native/Debugger/DebugEventHandler/DebugEventStorage.h"
#include "Native/Debugger/Interfaces/IDebuggerCommandQueue.h"
#include "TestHelpers/CrossThread.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace MCP::Native::Tests::Debugger
{
  using MCP::Native::Debugger::Commands::Base::CommandType;
  using MCP::Native::Debugger::DebugEventHandler::DebugEventStorage;
  using MCP::Native::Debugger::DebuggerCommandQueue;
  using MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue;
  using MCP::Native::Disassembler::Instruction;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockBreakpointManager;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockDebuggeeBreakIn;
  using MCP::Native::Tests::Mocks::Disassembler::Interfaces::MockDisassembler;
  using MCP::Native::Tests::Mocks::Disassembler::Interfaces::MockSymbolResolver;
  using MCP::Native::Tests::TestHelpers::RunOnOtherThread;
  using MCP::Native::Tests::TestHelpers::SignalCounter;
  using MCP::Native::Tests::TestHelpers::WaitForResult;
  using ::testing::_;
  using ::testing::DoAll;
  using ::testing::InSequence;
  using ::testing::InvokeWithoutArgs;
  using ::testing::IsNull;
  using ::testing::Matcher;
  using ::testing::Return;
  using ::testing::SetArgReferee;
  using ::testing::StrictMock;

  /// <summary>
  /// Tests the hand-over of commands from client threads to the debug thread, with doubles of everything the
  /// queue is given except the bookkeeping of debug events, which makes no call into the system.
  ///
  /// The tests are deterministic without a sleep because they wait for two points the queue guarantees:
  ///  * a command is in the queue once the break-in was requested - while the debug thread is not paused,
  ///    posting enqueues the command first and requests the break-in afterwards;
  ///  * the debug thread is paused once a command was executed inside the synchronous processing - the
  ///    processing marks the queue as paused before it executes the first command and keeps it so until a
  ///    command ends it.
  /// Every expectation is set before the first other thread starts: a mock must not be called while
  /// expectations are still being added. Commands are executed while the queue is locked, so the actions of
  /// the mocks only signal and never call the queue.
  /// </summary>
  class DebuggerCommandQueueTests : public ::testing::Test
  {
  protected:
    static constexpr DWORD KnownProcessId = 7;
    static constexpr DWORD UnknownProcessId = 99;
    static constexpr DWORD AnyThreadId = 8;

    /// <summary>A value that stands for a process handle. It is only stored and compared, never used.</summary>
    static HANDLE FakeProcessHandle()
    {
      // The cast builds a handle value from a number; no test hands it to the system.
      return reinterpret_cast<HANDLE>(static_cast<DWORD_PTR>(0x1234));
    }

    /// <summary>Records a process with the given handle and makes it the one of the current debug event.</summary>
    void MakeProcessCurrent(DWORD processId, HANDLE processHandle)
    {
      CREATE_PROCESS_DEBUG_INFO info = {};
      info.hProcess = processHandle;
      _debugEventStorage.AddProcess(processId, info);
      _queue.SetCurrentContext(processId, AnyThreadId);
    }

    /// <summary>
    /// Expects exactly this many break-in requests for the given handle and counts them: each one tells that
    /// a posted command is in the queue. Without a current process the queue asks with a null handle.
    /// </summary>
    void ExpectBreakInRequests(int count, const Matcher<HANDLE>& processHandle = IsNull())
    {
      EXPECT_CALL(_debuggeeBreakIn, RequestBreakIn(processHandle))
        .Times(count)
        .WillRepeatedly(InvokeWithoutArgs([this] { _queued.Signal(); }));
    }

    /// <summary>Makes the breakpoint manager accept a breakpoint at the address once and counts the call.</summary>
    void ExpectBreakpointSetAt(DWORD_PTR address)
    {
      EXPECT_CALL(_breakpointManager, SetBreakpoint(address))
        .WillOnce(DoAll(InvokeWithoutArgs([this] { _executed.Signal(); }), Return(true)));
    }

    /// <summary>
    /// Ends a synchronous processing that runs on another thread, as a client does that resumes the debuggee,
    /// and returns the command type the processing ended with.
    /// </summary>
    CommandType ResumeAndWaitForTheEndOfProcessing(std::future<CommandType>& processing)
    {
      auto resuming = RunOnOtherThread([this] { _queue.Continue(); });
      CommandType ended = WaitForResult(processing, "the synchronous processing to end");
      WaitForResult(resuming, "the caller of the resuming command to be released");
      return ended;
    }

    DebugEventStorage _debugEventStorage;
    StrictMock<MockBreakpointManager> _breakpointManager;
    StrictMock<MockDisassembler> _disassembler;
    StrictMock<MockSymbolResolver> _symbolResolver;
    StrictMock<MockDebuggeeBreakIn> _debuggeeBreakIn;
    SignalCounter _queued;   // commands that reached the queue while the debug thread was not paused
    SignalCounter _executed; // commands the breakpoint manager executed
    DebuggerCommandQueue _queue{ _debugEventStorage, _breakpointManager, _disassembler, _symbolResolver, _debuggeeBreakIn };
  };

  TEST_F(DebuggerCommandQueueTests, CommandsPostedWhileTheDebugThreadIsPausedAreExecutedAndTheirResultsReachTheCallers)
  {
    // Arrange
    // Only the command that brings the debug thread to its pause is posted while it is not paused.
    ExpectBreakInRequests(1);
    ExpectBreakpointSetAt(0x10);
    EXPECT_CALL(_breakpointManager, SetBreakpoint(0x20)).WillOnce(Return(true));
    EXPECT_CALL(_breakpointManager, RemoveBreakpoint(0x30)).WillOnce(Return(false));
    auto first = RunOnOtherThread([this] { return _queue.SetBreakpoint(0x10).IsSuccessful(); });
    _queued.WaitForCount(1, "the first command to be queued");
    auto processing = RunOnOtherThread([this] { return _queue.ProcessCommandsSync(); });
    _executed.WaitForCount(1, "the debug thread to execute the queued command");

    // Act
    auto accepted = RunOnOtherThread([this] { return _queue.SetBreakpoint(0x20).IsSuccessful(); });
    bool acceptedResult = WaitForResult(accepted, "the paused debug thread to execute the accepted command");
    auto refused = RunOnOtherThread([this] { return _queue.RemoveBreakpoint(0x30).IsSuccessful(); });
    bool refusedResult = WaitForResult(refused, "the paused debug thread to execute the refused command");

    // Assert
    // That waking the paused debug thread needed no break-in is checked by the expectation of exactly one.
    EXPECT_TRUE(acceptedResult);
    EXPECT_FALSE(refusedResult);
    EXPECT_TRUE(WaitForResult(first, "the first caller to be released"));
    EXPECT_EQ(CommandType::CMD_CONTINUE, ResumeAndWaitForTheEndOfProcessing(processing));
  }

  TEST_F(DebuggerCommandQueueTests, PostingWhileTheDebuggeeRunsRequestsABreakInForTheProcessOfTheCurrentEventAndExecutesNothing)
  {
    // Arrange
    MakeProcessCurrent(KnownProcessId, FakeProcessHandle());
    ExpectBreakInRequests(1, FakeProcessHandle());
    ExpectBreakpointSetAt(0x10);

    // Act
    auto caller = RunOnOtherThread([this] { return _queue.SetBreakpoint(0x10).IsSuccessful(); });
    _queued.WaitForCount(1, "the break-in request");
    int executedBeforeProcessing = _executed.Count();

    // Assert
    EXPECT_EQ(0, executedBeforeProcessing);
    // The command stays queued until the debug thread processes the queue; that also releases the caller.
    _queue.ProcessCommandsAsync();
    EXPECT_TRUE(WaitForResult(caller, "the caller to be released"));
  }

  TEST_F(DebuggerCommandQueueTests, AsynchronousProcessingExecutesTheQueuedCommandsInOrderAndEachCallerGetsItsResult)
  {
    // Arrange
    ExpectBreakInRequests(2);
    {
      InSequence executionOrder;
      EXPECT_CALL(_breakpointManager, SetBreakpoint(0x10)).WillOnce(Return(true));
      EXPECT_CALL(_breakpointManager, RemoveBreakpoint(0x20)).WillOnce(Return(false));
    }
    auto first = RunOnOtherThread([this] { return _queue.SetBreakpoint(0x10).IsSuccessful(); });
    _queued.WaitForCount(1, "the first command to be queued");
    auto second = RunOnOtherThread([this] { return _queue.RemoveBreakpoint(0x20).IsSuccessful(); });
    _queued.WaitForCount(2, "the second command to be queued");

    // Act
    _queue.ProcessCommandsAsync();

    // Assert
    EXPECT_TRUE(WaitForResult(first, "the first caller to be released"));
    EXPECT_FALSE(WaitForResult(second, "the second caller to be released"));
  }

  TEST_F(DebuggerCommandQueueTests, ACommandQueuedBehindAResumingCommandIsLeftForTheNextProcessing)
  {
    // Arrange
    ExpectBreakInRequests(2);
    ExpectBreakpointSetAt(0x10);
    auto resuming = RunOnOtherThread([this] { _queue.Continue(); });
    _queued.WaitForCount(1, "the resuming command to be queued");
    auto behind = RunOnOtherThread([this] { return _queue.SetBreakpoint(0x10).IsSuccessful(); });
    _queued.WaitForCount(2, "the command behind it to be queued");

    // Act: the processing that the resuming command ends
    CommandType ended = _queue.ProcessCommandsSync();
    int executedByThatProcessing = _executed.Count();

    // Assert
    EXPECT_EQ(CommandType::CMD_CONTINUE, ended);
    EXPECT_EQ(0, executedByThatProcessing);
    WaitForResult(resuming, "the caller of the resuming command to be released");

    // Act: the next processing
    _queue.ProcessCommandsAsync();

    // Assert
    EXPECT_TRUE(WaitForResult(behind, "the caller of the command behind it to be released"));
  }

  TEST_F(DebuggerCommandQueueTests, ExecuteDisassembleAsksTheDisassemblerWithTheProcessOfTheCurrentEvent)
  {
    // Arrange
    MakeProcessCurrent(KnownProcessId, FakeProcessHandle());
    auto decoded = std::make_shared<std::vector<Instruction>>(1);
    EXPECT_CALL(_disassembler, Disassemble(FakeProcessHandle(), KnownProcessId, 0x401000, 32, _))
      .WillOnce(SetArgReferee<4>(decoded));
    std::shared_ptr<std::vector<Instruction>> instructions;

    // Act
    bool result = _queue.ExecuteDisassemble(0x401000, 32, instructions);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_EQ(decoded, instructions);
  }

  TEST_F(DebuggerCommandQueueTests, ExecuteTryResolveStringAsksTheSymbolResolverWithTheProcessOfTheCurrentEvent)
  {
    // Arrange
    MakeProcessCurrent(KnownProcessId, FakeProcessHandle());
    EXPECT_CALL(_symbolResolver, ResolveString(FakeProcessHandle(), 0x402000)).WillOnce(Return("resolved text"));

    // Act
    std::string result = _queue.ExecuteTryResolveString(0x402000);

    // Assert
    EXPECT_EQ("resolved text", result);
  }

  TEST_F(DebuggerCommandQueueTests, GetBreakpointManagerHandsOutTheBreakpointManagerTheQueueWasGiven)
  {
    // Act
    auto* result = _queue.GetBreakpointManager();

    // Assert
    EXPECT_EQ(&_breakpointManager, result);
  }

  TEST_F(DebuggerCommandQueueTests, ExecuteReadMemoryFailsWithoutABufferWhenTheProcessOfTheCurrentEventIsUnknown)
  {
    // Arrange
    _queue.SetCurrentContext(UnknownProcessId, AnyThreadId);
    auto buffer = std::make_shared<std::vector<BYTE>>(4);

    // Act
    bool result = _queue.ExecuteReadMemory(0x401000, 4, buffer);

    // Assert
    EXPECT_FALSE(result);
    EXPECT_EQ(nullptr, buffer);
  }

  TEST_F(DebuggerCommandQueueTests, ExecuteWriteMemoryFailsWhenTheProcessOfTheCurrentEventIsUnknown)
  {
    // Arrange
    _queue.SetCurrentContext(UnknownProcessId, AnyThreadId);
    const BYTE data[] = { 0x90 };

    // Act
    bool result = _queue.ExecuteWriteMemory(0x401000, data, sizeof(data));

    // Assert
    EXPECT_FALSE(result);
  }

  /// <summary>One command that ends the synchronous processing: how a client posts it and its type.</summary>
  struct ProcessingEndCase
  {
    const char* Name;
    CommandType Type;
    void (*Post)(IDebuggerCommandQueue& queue);
  };

  /// <summary>Prints the case by its name; without it Google Test would dump the bytes of the struct.</summary>
  void PrintTo(const ProcessingEndCase& processingEndCase, std::ostream* output)
  {
    *output << processingEndCase.Name;
  }

  /// <summary>
  /// The commands that end the synchronous processing: the three that resume the debuggee, and stop.
  /// </summary>
  class DebuggerCommandQueueProcessingEndTests : public DebuggerCommandQueueTests,
                                                 public ::testing::WithParamInterface<ProcessingEndCase>
  {
  };

  TEST_P(DebuggerCommandQueueProcessingEndTests, SynchronousProcessingExecutesTheCommandQueuedBeforeAndEndsWithTheTypeOfTheEndingCommand)
  {
    // Arrange
    ExpectBreakInRequests(2);
    ExpectBreakpointSetAt(0x10);
    auto earlier = RunOnOtherThread([this] { return _queue.SetBreakpoint(0x10).IsSuccessful(); });
    _queued.WaitForCount(1, "the earlier command to be queued");
    auto ending = RunOnOtherThread([this] { GetParam().Post(_queue); });
    _queued.WaitForCount(2, "the ending command to be queued");

    // Act
    CommandType ended = _queue.ProcessCommandsSync();
    int executedByTheProcessing = _executed.Count();

    // Assert
    EXPECT_EQ(GetParam().Type, ended);
    EXPECT_EQ(1, executedByTheProcessing);
    EXPECT_TRUE(WaitForResult(earlier, "the caller of the earlier command to be released"));
    WaitForResult(ending, "the caller of the ending command to be released");
  }

  TEST_P(DebuggerCommandQueueProcessingEndTests, AnEndingCommandPostedWhileTheDebugThreadIsPausedEndsTheProcessingWithItsType)
  {
    // Arrange
    // Only the command that brings the debug thread to its pause is posted while it is not paused.
    ExpectBreakInRequests(1);
    ExpectBreakpointSetAt(0x10);
    auto first = RunOnOtherThread([this] { return _queue.SetBreakpoint(0x10).IsSuccessful(); });
    _queued.WaitForCount(1, "the first command to be queued");
    auto processing = RunOnOtherThread([this] { return _queue.ProcessCommandsSync(); });
    _executed.WaitForCount(1, "the debug thread to execute the queued command");

    // Act
    auto ending = RunOnOtherThread([this] { GetParam().Post(_queue); });
    CommandType ended = WaitForResult(processing, "the ending command to end the processing");

    // Assert
    // That waking the paused debug thread needed no break-in is checked by the expectation of exactly one.
    EXPECT_EQ(GetParam().Type, ended);
    WaitForResult(ending, "the caller of the ending command to be released");
    EXPECT_TRUE(WaitForResult(first, "the first caller to be released"));
  }

  INSTANTIATE_TEST_SUITE_P(
    EndingCommands,
    DebuggerCommandQueueProcessingEndTests,
    ::testing::Values(
      ProcessingEndCase{ "Continue", CommandType::CMD_CONTINUE, [](IDebuggerCommandQueue& queue) { queue.Continue(); } },
      ProcessingEndCase{ "StepInto", CommandType::CMD_STEP_INTO, [](IDebuggerCommandQueue& queue) { queue.StepInto(); } },
      ProcessingEndCase{ "StepOver", CommandType::CMD_STEP_OVER, [](IDebuggerCommandQueue& queue) { queue.StepOver(); } },
      ProcessingEndCase{ "Stop", CommandType::CMD_STOP, [](IDebuggerCommandQueue& queue) { queue.Stop(); } }),
    [](const ::testing::TestParamInfo<ProcessingEndCase>& info) { return std::string(info.param.Name); });

  /// <summary>
  /// One command that resumes the debuggee: its type and how a client posts it and learns whether the debugger
  /// accepted it.
  /// </summary>
  struct ResumingCommandCase
  {
    const char* Name;
    CommandType Type;
    bool (*Post)(IDebuggerCommandQueue& queue);
  };

  /// <summary>Prints the case by its name; without it Google Test would dump the bytes of the struct.</summary>
  void PrintTo(const ResumingCommandCase& resumingCommandCase, std::ostream* output)
  {
    *output << resumingCommandCase.Name;
  }

  /// <summary>
  /// What the three resuming commands report to their callers. A debuggee can only be resumed from a stop, so a
  /// command is accepted when the debug thread executes it while it processes a stop, and refused when it
  /// executes it between two debug events of a running debuggee. What counts is where the command is executed,
  /// not when it was posted.
  /// </summary>
  class DebuggerCommandQueueResumingCommandTests : public DebuggerCommandQueueTests,
                                                   public ::testing::WithParamInterface<ResumingCommandCase>
  {
  };

  TEST_P(DebuggerCommandQueueResumingCommandTests, AResumingCommandExecutedWhileTheDebuggeeRunsIsRefusedAndTheCommandBehindItIsExecuted)
  {
    // Arrange
    ExpectBreakInRequests(2);
    ExpectBreakpointSetAt(0x10);
    auto resuming = RunOnOtherThread([this] { return GetParam().Post(_queue); });
    _queued.WaitForCount(1, "the resuming command to be queued");
    auto behind = RunOnOtherThread([this] { return _queue.SetBreakpoint(0x10).IsSuccessful(); });
    _queued.WaitForCount(2, "the command behind it to be queued");

    // Act
    _queue.ProcessCommandsAsync();

    // Assert
    EXPECT_FALSE(WaitForResult(resuming, "the caller of the resuming command to be released"));
    EXPECT_TRUE(WaitForResult(behind, "the caller of the command behind it to be released"));
    EXPECT_EQ(1, _executed.Count());
  }

  TEST_P(DebuggerCommandQueueResumingCommandTests, AResumingCommandQueuedBeforeTheProcessingOfAStopBeginsIsAccepted)
  {
    // Arrange
    // The client posts while the debug thread has not begun to process the stop yet: to the queue the debuggee
    // still runs, so it asks for a break-in. The command is executed by the processing of the stop all the same.
    ExpectBreakInRequests(1);
    auto resuming = RunOnOtherThread([this] { return GetParam().Post(_queue); });
    _queued.WaitForCount(1, "the resuming command to be queued");

    // Act
    CommandType ended = _queue.ProcessCommandsSync();

    // Assert
    EXPECT_EQ(GetParam().Type, ended);
    EXPECT_TRUE(WaitForResult(resuming, "the caller of the resuming command to be released"));
  }

  TEST_P(DebuggerCommandQueueResumingCommandTests, AResumingCommandPostedWhileTheDebugThreadIsPausedIsAccepted)
  {
    // Arrange
    // Only the command that brings the debug thread to its pause is posted while it is not paused.
    ExpectBreakInRequests(1);
    ExpectBreakpointSetAt(0x10);
    auto first = RunOnOtherThread([this] { return _queue.SetBreakpoint(0x10).IsSuccessful(); });
    _queued.WaitForCount(1, "the first command to be queued");
    auto processing = RunOnOtherThread([this] { return _queue.ProcessCommandsSync(); });
    _executed.WaitForCount(1, "the debug thread to execute the queued command");

    // Act
    auto resuming = RunOnOtherThread([this] { return GetParam().Post(_queue); });
    CommandType ended = WaitForResult(processing, "the resuming command to end the processing");

    // Assert
    EXPECT_EQ(GetParam().Type, ended);
    EXPECT_TRUE(WaitForResult(resuming, "the caller of the resuming command to be released"));
    EXPECT_TRUE(WaitForResult(first, "the first caller to be released"));
  }

  TEST_P(DebuggerCommandQueueResumingCommandTests, ASecondResumingCommandBehindTheOneThatEndedTheStopIsRefused)
  {
    // Arrange
    ExpectBreakInRequests(2);
    auto first = RunOnOtherThread([this] { return GetParam().Post(_queue); });
    _queued.WaitForCount(1, "the first resuming command to be queued");
    auto second = RunOnOtherThread([this] { return GetParam().Post(_queue); });
    _queued.WaitForCount(2, "the second resuming command to be queued");

    // Act: the processing of the stop, which the first command ends
    CommandType ended = _queue.ProcessCommandsSync();

    // Assert
    EXPECT_EQ(GetParam().Type, ended);
    EXPECT_TRUE(WaitForResult(first, "the caller of the first resuming command to be released"));

    // Act: the processing after the debuggee was resumed
    _queue.ProcessCommandsAsync();

    // Assert
    EXPECT_FALSE(WaitForResult(second, "the caller of the second resuming command to be released"));
  }

  INSTANTIATE_TEST_SUITE_P(
    ResumingCommands,
    DebuggerCommandQueueResumingCommandTests,
    ::testing::Values(
      ResumingCommandCase{ "Continue", CommandType::CMD_CONTINUE, [](IDebuggerCommandQueue& queue) { return queue.Continue().IsSuccessful(); } },
      ResumingCommandCase{ "StepInto", CommandType::CMD_STEP_INTO, [](IDebuggerCommandQueue& queue) { return queue.StepInto().IsSuccessful(); } },
      ResumingCommandCase{ "StepOver", CommandType::CMD_STEP_OVER, [](IDebuggerCommandQueue& queue) { return queue.StepOver().IsSuccessful(); } }),
    [](const ::testing::TestParamInfo<ResumingCommandCase>& info) { return std::string(info.param.Name); });
}
