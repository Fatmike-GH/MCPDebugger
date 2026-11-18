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
#include ".\Interfaces\IDebuggerCommandQueue.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpointManager;
  class IDebuggeeBreakIn;
}

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventStorage;
}

namespace MCP::Native::Disassembler::Interfaces
{
  class IDisassembler;
  class ISymbolResolver;
}

namespace MCP::Native::Debugger
{
  class DebuggerCommandQueue : public Interfaces::IDebuggerCommandQueue,
                               public MCP::Native::Debugger::Commands::Base::ICommandContext
  {
  public:
    /// <summary>All collaborators are borrowed and must outlive the queue.</summary>
    DebuggerCommandQueue(MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorage& debugEventStorage,
                         Interfaces::IBreakpointManager& breakpointManager,
                         MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler,
                         MCP::Native::Disassembler::Interfaces::ISymbolResolver& symbolResolver,
                         Interfaces::IDebuggeeBreakIn& debuggeeBreakIn);
    ~DebuggerCommandQueue() override = default;

    void SetCurrentContext(DWORD processId, DWORD threadId) override;

    // Public synchronous memory wrappers (create command, post, wait, unwrap result)
    Commands::CommandResult Stop() override;
    Commands::CommandResult Continue() override;
    Commands::CommandResult StepInto() override;
    Commands::CommandResult StepOver() override;
    Commands::CommandResult SetBreakpoint(DWORD_PTR va) override;
    Commands::CommandResult RemoveBreakpoint(DWORD_PTR va) override;
    Commands::ReadMemoryResult ReadMemory(DWORD_PTR va, size_t size) override;
    Commands::CommandResult WriteMemory(DWORD_PTR va, const BYTE* data, size_t size) override;
    Commands::DisassembleResult Disassemble(DWORD_PTR va, size_t length) override;
    Commands::TryResolveStringResult TryResolveString(DWORD_PTR va) override;

    // Debugger thread control flow
    Commands::Base::CommandType ProcessCommandsSync() override;
    void ProcessCommandsAsync() override;

    // ICommandContext Implementation
    Interfaces::IBreakpointManager* GetBreakpointManager() override { return &_breakpointManager; }
    bool IsDebuggeeStopped() override;
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
    MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorage& _debugEventStorage;
    Interfaces::IBreakpointManager& _breakpointManager;
    MCP::Native::Disassembler::Interfaces::IDisassembler& _disassembler;
    MCP::Native::Disassembler::Interfaces::ISymbolResolver& _symbolResolver;
    Interfaces::IDebuggeeBreakIn& _debuggeeBreakIn;

    std::queue<Commands::Base::CommandPtr> _queue;
    std::mutex _mutex;
    MCP::Native::Threading::EventWaiter _eventWaiter;
    bool _paused = false;

    DWORD _currentProcessId;
    DWORD _currentThreadId;
  };
}