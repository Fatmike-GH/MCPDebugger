#include "Mocks/Debugger/Interfaces/MockDebuggerEventHandler.h"
#include "Mocks/Debugger/Interfaces/MockDebuggerSession.h"
#include "Mocks/Debugger/Interfaces/MockDebuggerSessionFactory.h"
#include "Mocks/Debugger/Interfaces/MockDebuggerThread.h"
#include "Mocks/PEFile/Interfaces/MockExecutableValidator.h"
#include "Mocks/ProcessExplorer/Interfaces/MockProcessExplorer.h"
#include "Native/Debugger/Debugger.h"
#include "Native/Debugger/DebugEventHandler/ProcessInfo.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace MCP::Native::Tests::Debugger
{
  using MCP::Native::Debugger::Commands::CommandResult;
  using MCP::Native::Debugger::Commands::DisassembleResult;
  using MCP::Native::Debugger::Commands::ReadMemoryResult;
  using MCP::Native::Debugger::Commands::TryResolveStringResult;
  using MCP::Native::Debugger::DebugEventHandler::ProcessInfo;
  using MCP::Native::Debugger::Interfaces::IDebuggerSession;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockDebuggerEventHandler;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockDebuggerSession;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockDebuggerSessionFactory;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockDebuggerThread;
  using MCP::Native::Tests::Mocks::PEFile::Interfaces::MockExecutableValidator;
  using MCP::Native::Tests::Mocks::ProcessExplorer::Interfaces::MockProcessExplorer;
  using ::testing::_;
  using ::testing::Assign;
  using ::testing::ByMove;
  using ::testing::DoAll;
  using ::testing::Ref;
  using ::testing::Return;
  using ::testing::ReturnPointee;
  using ::testing::ReturnRef;
  using ::testing::Sequence;
  using ::testing::SetArgReferee;
  using ::testing::StrictMock;

  /// <summary>
  /// Tests the debugger facade against doubles of everything it is given: the session factory, the executable
  /// check and - through the session double - the debug thread.
  /// </summary>
  class DebuggerTests : public ::testing::Test
  {
  protected:
    /// <summary>What every operation of the facade returned in one pass.</summary>
    struct OperationResults
    {
      bool IsRunning = true;
      CommandResult Stop{ true };
      CommandResult Continue{ true };
      CommandResult StepInto{ true };
      CommandResult StepOver{ true };
      CommandResult SetBreakpoint{ true };
      CommandResult RemoveBreakpoint{ true };
      ReadMemoryResult ReadMemory{ true };
      CommandResult WriteMemory{ true };
      DisassembleResult Disassemble{ true };
      TryResolveStringResult TryResolveString{ true, "unset" };
      PROCESS_INFORMATION ProcessInformation{};
      DWORD_PTR EntryPoint = 1;
      DWORD_PTR ImageBase = 1;
    };

    DebuggerTests()
    {
      auto sessionFactory = std::make_unique<StrictMock<MockDebuggerSessionFactory>>();
      auto executableValidator = std::make_unique<StrictMock<MockExecutableValidator>>();
      _sessionFactory = sessionFactory.get();
      _executableValidator = executableValidator.get();
      _debugger = std::make_unique<MCP::Native::Debugger::Debugger>(std::move(sessionFactory), std::move(executableValidator), _eventHandler);
    }

    /// <summary>
    /// Creates a session double that hands out the thread and the explorer of this fixture. The debugger under
    /// test becomes its owner; _session points to it for as long as the debugger keeps it.
    /// </summary>
    std::unique_ptr<IDebuggerSession> CreateSession()
    {
      auto session = std::make_unique<StrictMock<MockDebuggerSession>>();
      EXPECT_CALL(*session, GetDebuggerThread()).WillRepeatedly(ReturnRef(_thread));
      EXPECT_CALL(*session, GetProcessExplorer()).WillRepeatedly(ReturnRef(_processExplorer));
      _session = session.get();
      return session;
    }

    /// <summary>
    /// Expects one successful start: the validator accepts the file, the factory hands out a session and the
    /// thread starts. From then on the thread reports running until it is stopped, and it must be stopped
    /// exactly once - by the test or, at the latest, by the destructor of the debugger.
    /// </summary>
    void ExpectSuccessfulStart()
    {
      EXPECT_CALL(*_executableValidator, Validate(Path, _)).WillOnce(Return(true));
      EXPECT_CALL(*_sessionFactory, CreateSession(Ref(_eventHandler))).WillOnce(Return(ByMove(CreateSession())));
      EXPECT_CALL(_thread, Start(Path)).WillOnce(DoAll(Assign(&_threadRunning, true), Return(true)));
      EXPECT_CALL(_thread, IsRunning()).WillRepeatedly(ReturnPointee(&_threadRunning));
      EXPECT_CALL(_thread, Stop()).WillOnce(DoAll(Assign(&_threadRunning, false), Return(CommandResult(true))));
    }

    /// <summary>Brings the debugger under test into a running session.</summary>
    void StartDebugger()
    {
      ExpectSuccessfulStart();
      std::string error;
      ASSERT_TRUE(_debugger->Start(Path, error));
    }

    /// <summary>
    /// Brings the debugger under test into a session whose thread could not be started and reports not
    /// running.
    /// </summary>
    void StartDebuggerWithAThreadThatDoesNotStart()
    {
      EXPECT_CALL(*_executableValidator, Validate(Path, _)).WillOnce(Return(true));
      EXPECT_CALL(*_sessionFactory, CreateSession(Ref(_eventHandler))).WillOnce(Return(ByMove(CreateSession())));
      EXPECT_CALL(_thread, Start(Path)).WillOnce(Return(false));
      EXPECT_CALL(_thread, IsRunning()).WillRepeatedly(Return(false));
      std::string error;
      ASSERT_FALSE(_debugger->Start(Path, error));
    }

    OperationResults CallEveryOperation()
    {
      const BYTE data[] = { 0x90 };

      OperationResults results;
      results.IsRunning = _debugger->IsRunning();
      results.Stop = _debugger->Stop();
      results.Continue = _debugger->Continue();
      results.StepInto = _debugger->StepInto();
      results.StepOver = _debugger->StepOver();
      results.SetBreakpoint = _debugger->SetBreakpoint(Address);
      results.RemoveBreakpoint = _debugger->RemoveBreakpoint(Address);
      results.ReadMemory = _debugger->ReadMemory(Address, 16);
      results.WriteMemory = _debugger->WriteMemory(Address, data, sizeof(data));
      results.Disassemble = _debugger->Disassemble(Address, 16);
      results.TryResolveString = _debugger->TryResolveString(Address);
      results.ProcessInformation = _debugger->GetProcessInformation();
      results.EntryPoint = _debugger->GetEntryPoint(ProcessId);
      results.ImageBase = _debugger->GetImageBase(ProcessId);
      return results;
    }

    static void ExpectEveryOperationFailed(OperationResults& results)
    {
      EXPECT_FALSE(results.IsRunning);
      EXPECT_FALSE(results.Stop.IsSuccessful());
      EXPECT_FALSE(results.Continue.IsSuccessful());
      EXPECT_FALSE(results.StepInto.IsSuccessful());
      EXPECT_FALSE(results.StepOver.IsSuccessful());
      EXPECT_FALSE(results.SetBreakpoint.IsSuccessful());
      EXPECT_FALSE(results.RemoveBreakpoint.IsSuccessful());
      EXPECT_FALSE(results.ReadMemory.IsSuccessful());
      EXPECT_EQ(nullptr, results.ReadMemory.GetData());
      EXPECT_FALSE(results.WriteMemory.IsSuccessful());
      EXPECT_FALSE(results.Disassemble.IsSuccessful());
      EXPECT_EQ(nullptr, results.Disassemble.GetInstructions());
      EXPECT_FALSE(results.TryResolveString.IsSuccessful());
      EXPECT_EQ("", results.TryResolveString.GetResult());
      EXPECT_EQ(nullptr, results.ProcessInformation.hProcess);
      EXPECT_EQ(nullptr, results.ProcessInformation.hThread);
      EXPECT_EQ(0u, results.ProcessInformation.dwProcessId);
      EXPECT_EQ(0u, results.ProcessInformation.dwThreadId);
      EXPECT_EQ(0u, results.EntryPoint);
      EXPECT_EQ(0u, results.ImageBase);
    }

    static ProcessInfo CreateProcessInfo()
    {
      ProcessInfo processInfo{};
      processInfo.ProcessId = ProcessId;
      // The debug API reports both addresses as pointers into the debuggee; the tests only need their values.
      processInfo.info.lpStartAddress = reinterpret_cast<LPTHREAD_START_ROUTINE>(EntryPoint);
      processInfo.info.lpBaseOfImage = reinterpret_cast<LPVOID>(ImageBase);
      return processInfo;
    }

  protected:
    static constexpr DWORD ProcessId = 4711;
    static constexpr DWORD_PTR Address = 0x401000;
    static constexpr DWORD_PTR EntryPoint = 0x401230;
    static constexpr DWORD_PTR ImageBase = 0x400000;
    const std::string Path = "Example.exe";

    // The doubles the debugger borrows are declared before the debugger, so they outlive it: the debugger
    // stops a running thread in its destructor.
    StrictMock<MockDebuggerEventHandler> _eventHandler;
    StrictMock<MockDebuggerThread> _thread;
    StrictMock<MockProcessExplorer> _processExplorer;
    bool _threadRunning = false;

    // Owned by the debugger under test.
    StrictMock<MockDebuggerSessionFactory>* _sessionFactory = nullptr;
    StrictMock<MockExecutableValidator>* _executableValidator = nullptr;
    StrictMock<MockDebuggerSession>* _session = nullptr;

    std::unique_ptr<MCP::Native::Debugger::Debugger> _debugger;
  };

  TEST_F(DebuggerTests, EveryOperationFailsWithoutUsingACollaboratorBeforeTheFirstStart)
  {
    // Act
    OperationResults results = CallEveryOperation();
    auto* processExplorer = _debugger->GetProcessExplorer();

    // Assert: the strict doubles of factory and validator fail the test on any call.
    ExpectEveryOperationFailed(results);
    EXPECT_EQ(nullptr, processExplorer);
  }

  TEST_F(DebuggerTests, EveryOperationFailsAndOnlyAsksTheThreadForItsStateWhenTheThreadIsNotRunning)
  {
    // Arrange
    StartDebuggerWithAThreadThatDoesNotStart();

    // Act
    OperationResults results = CallEveryOperation();

    // Assert: the strict thread double fails the test on any call other than IsRunning.
    ExpectEveryOperationFailed(results);
  }

  TEST_F(DebuggerTests, StartValidatesTheExecutableThenCreatesTheSessionThenStartsTheThread)
  {
    // Arrange
    Sequence startOrder;
    EXPECT_CALL(*_executableValidator, Validate(Path, _)).InSequence(startOrder).WillOnce(Return(true));
    EXPECT_CALL(*_sessionFactory, CreateSession(Ref(_eventHandler))).InSequence(startOrder).WillOnce(Return(ByMove(CreateSession())));
    EXPECT_CALL(_thread, Start(Path)).InSequence(startOrder).WillOnce(Return(true));
    EXPECT_CALL(_thread, IsRunning()).WillRepeatedly(Return(false));
    std::string error = "unchanged";

    // Act
    bool started = _debugger->Start(Path, error);

    // Assert
    EXPECT_TRUE(started);
    EXPECT_EQ("unchanged", error);
  }

  TEST_F(DebuggerTests, StartCreatesNoSessionAndReturnsTheErrorOfTheValidatorWhenTheExecutableIsRejected)
  {
    // Arrange
    EXPECT_CALL(*_executableValidator, Validate(Path, _))
      .WillOnce(DoAll(SetArgReferee<1>(std::string("The file was rejected.")), Return(false)));
    std::string error;

    // Act
    bool started = _debugger->Start(Path, error);

    // Assert: the strict factory double fails the test if a session is asked for.
    EXPECT_FALSE(started);
    EXPECT_EQ("The file was rejected.", error);
    EXPECT_FALSE(_debugger->IsRunning());
    EXPECT_EQ(nullptr, _debugger->GetProcessExplorer());
  }

  TEST_F(DebuggerTests, StartFailsWithItsOwnErrorWhenTheThreadCannotBeStarted)
  {
    // Arrange
    EXPECT_CALL(*_executableValidator, Validate(Path, _)).WillOnce(Return(true));
    EXPECT_CALL(*_sessionFactory, CreateSession(Ref(_eventHandler))).WillOnce(Return(ByMove(CreateSession())));
    EXPECT_CALL(_thread, Start(Path)).WillOnce(Return(false));
    EXPECT_CALL(_thread, IsRunning()).WillRepeatedly(Return(false));
    std::string error;

    // Act
    bool started = _debugger->Start(Path, error);

    // Assert
    EXPECT_FALSE(started);
    EXPECT_EQ("The debugger could not be started.", error);
  }

  TEST_F(DebuggerTests, StartFailsWithoutValidatingAgainWhileASessionIsRunning)
  {
    // Arrange
    StartDebugger();
    std::string error;

    // Act
    bool started = _debugger->Start(Path, error);

    // Assert: validator, factory and thread expect one start only and fail the test on a second one.
    EXPECT_FALSE(started);
    EXPECT_EQ("Debugger is already running.", error);
    EXPECT_TRUE(_debugger->IsRunning());
  }

  TEST_F(DebuggerTests, StartAfterAStopAsksTheFactoryForANewSession)
  {
    // Arrange
    StartDebugger();
    _debugger->Stop();
    ExpectSuccessfulStart();
    std::string error;

    // Act
    bool started = _debugger->Start(Path, error);

    // Assert: the factory was expected twice, once per start.
    EXPECT_TRUE(started);
    EXPECT_TRUE(_debugger->IsRunning());
  }

  TEST_F(DebuggerTests, StopIsPassedToTheRunningThread)
  {
    // Arrange: a started debugger expects exactly one Stop of its thread.
    StartDebugger();

    // Act
    CommandResult result = _debugger->Stop();

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
    EXPECT_FALSE(_debugger->IsRunning());
  }

  TEST_F(DebuggerTests, ContinueIsPassedToTheRunningThread)
  {
    // Arrange
    StartDebugger();
    EXPECT_CALL(_thread, Continue()).WillOnce(Return(CommandResult(true)));

    // Act
    CommandResult result = _debugger->Continue();

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
  }

  TEST_F(DebuggerTests, StepIntoIsPassedToTheRunningThread)
  {
    // Arrange
    StartDebugger();
    EXPECT_CALL(_thread, StepInto()).WillOnce(Return(CommandResult(true)));

    // Act
    CommandResult result = _debugger->StepInto();

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
  }

  TEST_F(DebuggerTests, StepOverIsPassedToTheRunningThread)
  {
    // Arrange
    StartDebugger();
    EXPECT_CALL(_thread, StepOver()).WillOnce(Return(CommandResult(true)));

    // Act
    CommandResult result = _debugger->StepOver();

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
  }

  TEST_F(DebuggerTests, SetBreakpointIsPassedToTheRunningThreadWithItsAddress)
  {
    // Arrange
    StartDebugger();
    EXPECT_CALL(_thread, SetBreakpoint(Address)).WillOnce(Return(CommandResult(true)));

    // Act
    CommandResult result = _debugger->SetBreakpoint(Address);

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
  }

  TEST_F(DebuggerTests, RemoveBreakpointIsPassedToTheRunningThreadWithItsAddress)
  {
    // Arrange
    StartDebugger();
    EXPECT_CALL(_thread, RemoveBreakpoint(Address)).WillOnce(Return(CommandResult(true)));

    // Act
    CommandResult result = _debugger->RemoveBreakpoint(Address);

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
  }

  TEST_F(DebuggerTests, ReadMemoryIsPassedToTheRunningThreadAndReturnsItsBuffer)
  {
    // Arrange
    StartDebugger();
    auto buffer = std::make_shared<std::vector<uint8_t>>(std::vector<uint8_t>{ 0x90, 0xCC });
    EXPECT_CALL(_thread, ReadMemory(Address, 2)).WillOnce(Return(ReadMemoryResult(true, buffer)));

    // Act
    ReadMemoryResult result = _debugger->ReadMemory(Address, 2);

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
    EXPECT_EQ(buffer, result.GetData());
  }

  TEST_F(DebuggerTests, WriteMemoryIsPassedToTheRunningThreadWithTheBufferOfTheCaller)
  {
    // Arrange
    StartDebugger();
    const BYTE data[] = { 0x90, 0xCC };
    EXPECT_CALL(_thread, WriteMemory(Address, data, sizeof(data))).WillOnce(Return(CommandResult(true)));

    // Act
    CommandResult result = _debugger->WriteMemory(Address, data, sizeof(data));

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
  }

  TEST_F(DebuggerTests, DisassembleIsPassedToTheRunningThreadAndReturnsItsInstructions)
  {
    // Arrange
    StartDebugger();
    auto instructions = std::make_shared<std::vector<MCP::Native::Disassembler::Instruction>>(1);
    EXPECT_CALL(_thread, Disassemble(Address, 16)).WillOnce(Return(DisassembleResult(true, instructions)));

    // Act
    DisassembleResult result = _debugger->Disassemble(Address, 16);

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
    EXPECT_EQ(instructions, result.GetInstructions());
  }

  TEST_F(DebuggerTests, TryResolveStringIsPassedToTheRunningThreadAndReturnsItsText)
  {
    // Arrange
    StartDebugger();
    EXPECT_CALL(_thread, TryResolveString(Address)).WillOnce(Return(TryResolveStringResult(true, "Hello")));

    // Act
    TryResolveStringResult result = _debugger->TryResolveString(Address);

    // Assert
    EXPECT_TRUE(result.IsSuccessful());
    EXPECT_EQ("Hello", result.GetResult());
  }

  TEST_F(DebuggerTests, GetProcessInformationReturnsWhatTheRunningThreadReports)
  {
    // Arrange
    StartDebugger();
    PROCESS_INFORMATION expected{};
    expected.dwProcessId = ProcessId;
    expected.dwThreadId = 4712;
    EXPECT_CALL(_thread, GetProcessInformation()).WillOnce(Return(expected));

    // Act
    PROCESS_INFORMATION processInformation = _debugger->GetProcessInformation();

    // Assert
    EXPECT_EQ(expected.dwProcessId, processInformation.dwProcessId);
    EXPECT_EQ(expected.dwThreadId, processInformation.dwThreadId);
  }

  TEST_F(DebuggerTests, GetEntryPointReturnsTheStartAddressTheSessionRecordedForTheProcess)
  {
    // Arrange
    StartDebugger();
    EXPECT_CALL(*_session, GetProcessById(ProcessId, _)).WillOnce(DoAll(SetArgReferee<1>(CreateProcessInfo()), Return(true)));

    // Act
    DWORD_PTR entryPoint = _debugger->GetEntryPoint(ProcessId);

    // Assert
    EXPECT_EQ(EntryPoint, entryPoint);
  }

  TEST_F(DebuggerTests, GetImageBaseReturnsTheImageBaseTheSessionRecordedForTheProcess)
  {
    // Arrange
    StartDebugger();
    EXPECT_CALL(*_session, GetProcessById(ProcessId, _)).WillOnce(DoAll(SetArgReferee<1>(CreateProcessInfo()), Return(true)));

    // Act
    DWORD_PTR imageBase = _debugger->GetImageBase(ProcessId);

    // Assert
    EXPECT_EQ(ImageBase, imageBase);
  }

  TEST_F(DebuggerTests, GetEntryPointAndGetImageBaseReturnZeroForAProcessTheSessionDoesNotKnow)
  {
    // Arrange
    StartDebugger();
    EXPECT_CALL(*_session, GetProcessById(ProcessId, _)).Times(2).WillRepeatedly(Return(false));

    // Act
    DWORD_PTR entryPoint = _debugger->GetEntryPoint(ProcessId);
    DWORD_PTR imageBase = _debugger->GetImageBase(ProcessId);

    // Assert
    EXPECT_EQ(0u, entryPoint);
    EXPECT_EQ(0u, imageBase);
  }

  TEST_F(DebuggerTests, GetProcessExplorerReturnsTheExplorerOfTheRunningSession)
  {
    // Arrange
    StartDebugger();

    // Act
    auto* processExplorer = _debugger->GetProcessExplorer();

    // Assert
    EXPECT_EQ(&_processExplorer, processExplorer);
  }

  TEST_F(DebuggerTests, GetProcessExplorerStillReturnsTheExplorerOfTheSessionAfterAStop)
  {
    // Arrange
    StartDebugger();
    _debugger->Stop();

    // Act
    auto* processExplorer = _debugger->GetProcessExplorer();

    // Assert
    EXPECT_EQ(&_processExplorer, processExplorer);
  }

  TEST_F(DebuggerTests, DestroyingARunningDebuggerStopsTheThread)
  {
    // Arrange: a started debugger expects exactly one Stop of its thread.
    StartDebugger();

    // Act
    _debugger.reset();

    // Assert
    EXPECT_FALSE(_threadRunning);
  }
}
