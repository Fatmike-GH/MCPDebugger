#pragma once
#include <queue>
#include <mutex>
#include <memory>
#include ".\Commands\Base\ICommand.h"
#include ".\Commands\Base\ICommandContext.h"
#include ".\Commands\Base\CommandType.h"
#include ".\Commands\CommandResult.h"
#include ".\Commands\ReadMemoryResult.h"
#include ".\Commands\DisassembleResult.h"
#include ".\Commands\TryResolveStringResult.h"
#include "..\Threading\EventWaiter.h"

namespace MCP::Native::Debugger
{
  class BreakpointManager;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
}

namespace MCP::Native::Disassembler
{
  class Disassembler;
}

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer;
}

namespace MCP::Native::Debugger
{
  class DebuggerCommandQueue : public MCP::Native::Debugger::Commands::Base::ICommandContext
  {
  public:
    DebuggerCommandQueue(MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                         MCP::Native::Debugger::DebugEventHandler::DebugEventStorage* debugEventStorage,
                         BreakpointManager* breakpointManager);
    ~DebuggerCommandQueue() = default;

    void SetCurrentContext(DWORD processId, DWORD threadId);

    // Public synchronous memory wrappers (create command, post, wait, unwrap result)
    Commands::CommandResult Stop();
    Commands::CommandResult Continue();
    Commands::CommandResult StepInto();
    Commands::CommandResult StepOver();
    Commands::CommandResult SetBreakpoint(DWORD_PTR va);
    Commands::CommandResult RemoveBreakpoint(DWORD_PTR va);
    Commands::ReadMemoryResult ReadMemory(DWORD_PTR va, size_t size);
    Commands::CommandResult WriteMemory(DWORD_PTR va, const BYTE* data, size_t size);
    Commands::DisassembleResult Disassemble(DWORD_PTR va, size_t length);
    Commands::TryResolveStringResult TryResolveString(DWORD_PTR va);

    // Debugger thread control flow
    Commands::Base::CommandType ProcessCommandsSync();
    void ProcessCommandsAsync();

    // ICommandContext Implementation
    BreakpointManager* GetBreakpointManager() override { return _breakpointManager; }
    void ExecuteClearCommandQueue() override;
    bool ExecuteReadMemory(DWORD_PTR va, size_t size, std::shared_ptr<std::vector<BYTE>>& outDataBuffer) override;
    bool ExecuteWriteMemory(DWORD_PTR va, const BYTE* data, size_t size) override;
    bool ExecuteDisassemble(DWORD_PTR address, size_t length, std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>>& outInstructions) override;
    std::string ExecuteTryResolveString(DWORD_PTR address) override;

  private:
    void PostCommand(Commands::Base::CommandPtr command);
    bool EnqueueCommand(Commands::Base::CommandPtr cmd);
    void NotifyDebuggerThread(bool isPaused);
    bool ExecutePendingCommands(Commands::Base::CommandType& lastCommand);
    void ExecuteAllPendingCommands();

    bool HandleCommand(Commands::Base::CommandPtr cmd);
    void ProcessCommand(Commands::Base::CommandPtr cmd);

    bool IsGoCommand(Commands::Base::CommandType type);
    void SetPaused(bool value);

    HANDLE GetCurrentDebuggeeProcess();
    HANDLE GetCurrentDebuggeeThread();    

  private:
    MCP::Native::ProcessExplorer::ProcessExplorer* _processExplorer = nullptr;
    MCP::Native::Debugger::DebugEventHandler::DebugEventStorage* _debugEventStorage = nullptr;
    BreakpointManager* _breakpointManager = nullptr;

    std::queue<Commands::Base::CommandPtr> _queue;
    std::mutex _mutex;
    MCP::Native::Threading::EventWaiter _eventWaiter;
    bool _paused = false;

    DWORD _currentProcessId;
    DWORD _currentThreadId;
  };
}