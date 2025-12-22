#include "DebuggerCommandQueue.h"
#include <utility>
#include <memory>
#include <queue>
#include <future>
#include <algorithm>
#include <Windows.h>
#include "BreakpointManager.h"
#include ".\Commands\Base\CommandType.h"
#include ".\Commands\Command.h"
#include ".\Commands\DisassembleCommand.h"
#include ".\Commands\ReadMemoryCommand.h"
#include ".\Commands\RemoveBreakpointCommand.h"
#include ".\Commands\SetBreakpointCommand.h"
#include ".\Commands\StepIntoCommand.h"
#include ".\Commands\StepOverCommand.h"
#include ".\Commands\StopCommand.h"
#include ".\Commands\TryResolveStringCommand.h"
#include ".\Commands\WriteMemoryCommand.h"
#include ".\DebugEventHandler\DebugEventStorage.h"
#include ".\DebugEventHandler\ProcessInfo.h"
#include ".\DebugEventHandler\ThreadInfo.h"
#include "..\Disassembler\Disassembler.h"
#include "..\Threading\EventWaiter.h"

namespace MCP::Native::Debugger
{
  DebuggerCommandQueue::DebuggerCommandQueue(MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                                             MCP::Native::Debugger::DebugEventHandler::DebugEventStorage* debugEventStorage,
                                             BreakpointManager* breakpointManager)
    : _processExplorer(processExplorer),
      _debugEventStorage(debugEventStorage),
      _breakpointManager(breakpointManager)
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

  bool DebuggerCommandQueue::IsGoCommand(Commands::Base::CommandType type)
  {
    switch (type)
    {
    case Commands::Base::CommandType::CMD_CONTINUE:
    case Commands::Base::CommandType::CMD_STEP_INTO:
    case Commands::Base::CommandType::CMD_STEP_OVER:
      return true;
    default:
      return false;
    }
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
      HANDLE processHandle = GetCurrentDebuggeeProcess();
      DebugBreakProcess(processHandle);
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
    auto command = std::make_unique<Commands::Command>(MCP::Native::Debugger::Commands::Base::CommandType::CMD_CONTINUE);
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

  Commands::Base::CommandType DebuggerCommandQueue::ProcessCommandsSync()
  {
    SetPaused(true);
    Commands::Base::CommandType lastCommand = Commands::Base::CommandType::CMD_CONTINUE;

    while (true)
    {
      _eventWaiter.Reset();

      if (ExecutePendingCommands(lastCommand))
        break;

      _eventWaiter.Wait();
    }

    SetPaused(false);
    return lastCommand;
  }

  void DebuggerCommandQueue::ProcessCommandsAsync()
  {
    SetPaused(false);
    ExecuteAllPendingCommands();
    _eventWaiter.Reset();
  }

  bool DebuggerCommandQueue::ExecutePendingCommands(Commands::Base::CommandType& lastCommand)
  {
    std::lock_guard<std::mutex> lock(_mutex);

    while (!_queue.empty())
    {
      Commands::Base::CommandPtr cmd = std::move(_queue.front());
      _queue.pop();

      lastCommand = cmd->GetType();
      if (HandleCommand(std::move(cmd)))
      {
        return true; // Stop the processing loop
      }
    }
    return false;
  }

  void DebuggerCommandQueue::ExecuteAllPendingCommands()
  {
    std::lock_guard<std::mutex> lock(_mutex);

    while (!_queue.empty())
    {
      Commands::Base::CommandPtr cmd = std::move(_queue.front());
      _queue.pop();

      HandleCommand(std::move(cmd));
    }
  }

  bool DebuggerCommandQueue::HandleCommand(Commands::Base::CommandPtr cmd)
  {
    if (!cmd) return false;

    Commands::Base::CommandType type = cmd->GetType();
    ProcessCommand(std::move(cmd));

    if (IsGoCommand(type) || type == Commands::Base::CommandType::CMD_STOP)
    {
      return true; // Signal stop
    }

    // Standard Commands (continue processing queue)
    return false;
  }

  void DebuggerCommandQueue::ProcessCommand(Commands::Base::CommandPtr cmd)
  {
    if (!cmd) return;
    cmd->Execute(this);
  }

  void DebuggerCommandQueue::ExecuteClearCommandQueue()
  {
    // Executes the logic previously in the CMD_STOP switch case.
    // Assumes the lock is held by ExecutePendingCommands/ExecuteAllPendingCommands.
    std::queue<Commands::Base::CommandPtr>().swap(_queue);
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
    MCP::Native::Disassembler::Disassembler disassembler(_processExplorer);
    disassembler.Disassemble(GetCurrentDebuggeeProcess(), _currentProcessId, address, length, outInstructions);
    return true;
  }

  std::string DebuggerCommandQueue::ExecuteTryResolveString(DWORD_PTR address)
  {
    MCP::Native::Disassembler::SymbolResolver symbolResolver(_processExplorer);
    return symbolResolver.ResolveString(GetCurrentDebuggeeProcess(), address);
  }

  HANDLE DebuggerCommandQueue::GetCurrentDebuggeeProcess()
  {
    MCP::Native::Debugger::DebugEventHandler::ProcessInfo processInfo = { 0 };
    _debugEventStorage->GetProcessById(_currentProcessId, processInfo);
    return processInfo.info.hProcess;
  }

  HANDLE DebuggerCommandQueue::GetCurrentDebuggeeThread()
  {
    MCP::Native::Debugger::DebugEventHandler::ThreadInfo threadInfo = { 0 };
    _debugEventStorage->GetThreadById(_currentThreadId, threadInfo);
    return threadInfo.info.hThread;
  }  
}