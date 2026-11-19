#include "DebuggerCommandQueue.h"
#include <utility>
#include <memory>
#include <queue>
#include <future>
#include <algorithm>
#include <Windows.h>
#include ".\Interfaces\IBreakpointManager.h"
#include ".\Interfaces\IDebuggeeBreakIn.h"
#include ".\Interfaces\IResumePlan.h"
#include ".\Commands\ContinueCommand.h"
#include ".\Commands\DisassembleCommand.h"
#include ".\Commands\ReadMemoryCommand.h"
#include ".\Commands\RemoveBreakpointCommand.h"
#include ".\Commands\SetBreakpointCommand.h"
#include ".\Commands\StepIntoCommand.h"
#include ".\Commands\StepOverCommand.h"
#include ".\Commands\StopCommand.h"
#include ".\Commands\TryResolveStringCommand.h"
#include ".\Commands\WriteMemoryCommand.h"
#include ".\DebugEventHandler\Interfaces\IDebugEventStorage.h"
#include ".\DebugEventHandler\ProcessInfo.h"
#include ".\DebugEventHandler\ThreadInfo.h"
#include "..\Disassembler\Interfaces\IDisassembler.h"
#include "..\Disassembler\Interfaces\ISymbolResolver.h"
#include "..\Threading\EventWaiter.h"

namespace MCP::Native::Debugger
{
  DebuggerCommandQueue::DebuggerCommandQueue(MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorage& debugEventStorage,
                                             Interfaces::IBreakpointManager& breakpointManager,
                                             MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler,
                                             MCP::Native::Disassembler::Interfaces::ISymbolResolver& symbolResolver,
                                             Interfaces::IDebuggeeBreakIn& debuggeeBreakIn)
    : _debugEventStorage(debugEventStorage),
      _breakpointManager(breakpointManager),
      _disassembler(disassembler),
      _symbolResolver(symbolResolver),
      _debuggeeBreakIn(debuggeeBreakIn)
  {
    _currentProcessId = 0;
    _currentThreadId = 0;
  }

  void DebuggerCommandQueue::SetCurrentContext(DWORD processId, DWORD threadId)
  {
    _currentProcessId = processId;
    _currentThreadId = threadId;
  }

  void DebuggerCommandQueue::SetPaused(bool value)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _paused = value;
  }

  // Called by main thread
  void DebuggerCommandQueue::PostCommand(Commands::Base::CommandPtr command)
  {
    if (!command) return;
    const bool wasPaused = EnqueueCommand(std::move(command));
    NotifyDebuggerThread(wasPaused);
  }

  bool DebuggerCommandQueue::EnqueueCommand(Commands::Base::CommandPtr cmd)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.push(std::move(cmd));
    return _paused;
  }

  void DebuggerCommandQueue::NotifyDebuggerThread(bool isPaused)
  {
    if (isPaused)
    {
      _eventWaiter.Wakeup(true);
    }
    else
    {
      _debuggeeBreakIn.RequestBreakIn(GetCurrentDebuggeeProcess());
    }
  }

  Commands::CommandResult DebuggerCommandQueue::Stop()
  {
    auto command = std::make_unique<Commands::StopCommand>();
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::CommandResult* result = dynamic_cast<Commands::CommandResult*>(commandResult.get());

    return std::move(*result);
  }

  Commands::CommandResult DebuggerCommandQueue::Continue()
  {
    auto command = std::make_unique<Commands::ContinueCommand>();
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::CommandResult* result = dynamic_cast<Commands::CommandResult*>(commandResult.get());

    return std::move(*result);
  }

  Commands::CommandResult DebuggerCommandQueue::StepInto()
  {
    auto command = std::make_unique<Commands::StepIntoCommand>();
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::CommandResult* result = dynamic_cast<Commands::CommandResult*>(commandResult.get());

    return std::move(*result);
  }

  Commands::CommandResult DebuggerCommandQueue::StepOver()
  {
    auto command = std::make_unique<Commands::StepOverCommand>();
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::CommandResult* result = dynamic_cast<Commands::CommandResult*>(commandResult.get());

    return std::move(*result);
  }

  Commands::CommandResult DebuggerCommandQueue::SetBreakpoint(DWORD_PTR va)
  {
    auto command = std::make_unique<Commands::SetBreakpointCommand>(va);
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::CommandResult* result = dynamic_cast<Commands::CommandResult*>(commandResult.get());

    return std::move(*result);
  }

  Commands::CommandResult DebuggerCommandQueue::RemoveBreakpoint(DWORD_PTR va)
  {
    auto command = std::make_unique<Commands::RemoveBreakpointCommand>(va);
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::CommandResult* result = dynamic_cast<Commands::CommandResult*>(commandResult.get());

    return std::move(*result);
  }

  Commands::ReadMemoryResult DebuggerCommandQueue::ReadMemory(DWORD_PTR va, size_t size)
  {
    auto command = std::make_unique<Commands::ReadMemoryCommand>(va, size);
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::ReadMemoryResult* result = dynamic_cast<Commands::ReadMemoryResult*>(commandResult.get());

    if (result)
    {
      return std::move(*result);
    }

    return Commands::ReadMemoryResult(false);
  }

  Commands::CommandResult DebuggerCommandQueue::WriteMemory(DWORD_PTR va, const BYTE* data, size_t size)
  {
    auto command = std::make_unique<Commands::WriteMemoryCommand>(va, data, size);
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::CommandResult* result = dynamic_cast<Commands::CommandResult*>(commandResult.get());

    return std::move(*result);
  }

  Commands::DisassembleResult DebuggerCommandQueue::Disassemble(DWORD_PTR va, size_t length)
  {
    auto command = std::make_unique<Commands::DisassembleCommand>(va, length);
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::DisassembleResult* result = dynamic_cast<Commands::DisassembleResult*>(commandResult.get());

    if (result)
    {
      return std::move(*result);
    }

    return Commands::DisassembleResult(false);
  }

  Commands::TryResolveStringResult DebuggerCommandQueue::TryResolveString(DWORD_PTR va)
  {
    auto command = std::make_unique<Commands::TryResolveStringCommand>(va);
    auto future = command->GetFuture();

    PostCommand(std::move(command));

    Commands::Base::CommandResultPtr commandResult = future.get();
    Commands::TryResolveStringResult* result = dynamic_cast<Commands::TryResolveStringResult*>(commandResult.get());

    if (result)
    {
      return std::move(*result);
    }

    return Commands::TryResolveStringResult(false, std::string());
  }

  Interfaces::StopOutcome DebuggerCommandQueue::ProcessCommandsSync()
  {
    SetPaused(true);
    std::optional<Interfaces::StopOutcome> outcome;

    while (true)
    {
      _eventWaiter.Reset();

      outcome = ExecutePendingCommandsOfTheStop();
      if (outcome)
        break;

      _eventWaiter.Wait();
    }

    SetPaused(false);
    return std::move(*outcome);
  }

  void DebuggerCommandQueue::ProcessCommandsAsync()
  {
    SetPaused(false);
    ExecuteAllPendingCommands();
    _eventWaiter.Reset();
  }

  std::optional<Interfaces::StopOutcome> DebuggerCommandQueue::ExecutePendingCommandsOfTheStop()
  {
    std::lock_guard<std::mutex> lock(_mutex);

    while (!_queue.empty())
    {
      Commands::Base::CommandPtr cmd = std::move(_queue.front());
      _queue.pop();

      ProcessCommand(std::move(cmd));

      // The command may have resumed the debuggee or ended the session, which also empties the queue: the
      // outcome is looked at before the queue is touched again. What is queued behind a resuming command
      // stays queued for the next processing
      if (_stopOutcome)
      {
        return std::exchange(_stopOutcome, std::nullopt);
      }
    }
    return std::nullopt;
  }

  void DebuggerCommandQueue::ExecuteAllPendingCommands()
  {
    std::lock_guard<std::mutex> lock(_mutex);

    while (!_queue.empty())
    {
      Commands::Base::CommandPtr cmd = std::move(_queue.front());
      _queue.pop();

      ProcessCommand(std::move(cmd));
    }
  }

  void DebuggerCommandQueue::ProcessCommand(Commands::Base::CommandPtr cmd)
  {
    if (!cmd) return;
    cmd->Execute(this);
  }

  bool DebuggerCommandQueue::Resume(std::unique_ptr<Interfaces::IResumePlan> plan)
  {
    // Called by a command while it executes, so this thread already holds the queue mutex: the paused flag
    // and the outcome are used directly, locking again would deadlock. The flag is set for the whole
    // processing of a stop and cleared before commands are executed between two debug events - there the
    // debuggee runs and cannot be resumed. A stop is ended by one command only
    if (!plan || !_paused || _stopOutcome)
    {
      return false;
    }

    _stopOutcome = Interfaces::StopOutcome::Resumed(std::move(plan));
    return true;
  }

  void DebuggerCommandQueue::EndSession()
  {
    // Called by a command while it executes: the queue mutex is held by this thread, see Resume
    std::queue<Commands::Base::CommandPtr>().swap(_queue);

    // Only the processing of a stop waits for an outcome. Between two debug events nothing is processed
    // that could end, and an outcome left there would end the next stop before a client answered it
    if (_paused && !_stopOutcome)
    {
      _stopOutcome = Interfaces::StopOutcome::SessionEnded();
    }
  }

  bool DebuggerCommandQueue::ExecuteReadMemory(DWORD_PTR address, size_t size, std::shared_ptr<std::vector<BYTE>>& outDataBuffer)
  {
    HANDLE processHandle = GetCurrentDebuggeeProcess();
    if (!processHandle || size == 0)
    {
      outDataBuffer.reset();
      return false;
    }

    outDataBuffer = std::make_shared<std::vector<uint8_t>>();
    outDataBuffer->resize(size);

    SIZE_T bytesRead = 0;
    BOOL apiSuccess = ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), outDataBuffer->data(), size, &bytesRead);

    bool success = (apiSuccess && (bytesRead == size));
    if (!success)
    {
      outDataBuffer.reset();
    }
    return success;
  }

  bool DebuggerCommandQueue::ExecuteWriteMemory(DWORD_PTR address, const BYTE* data, size_t size)
  {
    HANDLE processHandle = GetCurrentDebuggeeProcess();

    if (!processHandle || !data || size == 0)
    {
      return false;
    }

    SIZE_T bytesWritten = 0;
    BOOL success = WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), data, size, &bytesWritten);

    bool overallSuccess = (success && (bytesWritten == size));

    if (overallSuccess)
    {
      FlushInstructionCache(processHandle, reinterpret_cast<LPCVOID>(address), size);
    }

    return overallSuccess;
  }

  bool DebuggerCommandQueue::ExecuteDisassemble(DWORD_PTR address, size_t length, std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>>& outInstructions)
  {
    _disassembler.Disassemble(GetCurrentDebuggeeProcess(), _currentProcessId, address, length, outInstructions);
    return true;
  }

  std::string DebuggerCommandQueue::ExecuteTryResolveString(DWORD_PTR address)
  {
    return _symbolResolver.ResolveString(GetCurrentDebuggeeProcess(), address);
  }

  HANDLE DebuggerCommandQueue::GetCurrentDebuggeeProcess()
  {
    MCP::Native::Debugger::DebugEventHandler::ProcessInfo processInfo = { 0 };
    _debugEventStorage.GetProcessById(_currentProcessId, processInfo);
    return processInfo.info.hProcess;
  }

  HANDLE DebuggerCommandQueue::GetCurrentDebuggeeThread()
  {
    MCP::Native::Debugger::DebugEventHandler::ThreadInfo threadInfo = { 0 };
    _debugEventStorage.GetThreadById(_currentThreadId, threadInfo);
    return threadInfo.info.hThread;
  }  
}