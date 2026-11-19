#pragma once
#include <queue>
#include <mutex>
#include <memory>
#include <optional>
#include ".\Commands\Base\ICommand.h"
#include ".\Commands\Base\ICommandContext.h"
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
    Interfaces::StopOutcome ProcessCommandsSync() override;
    void ProcessCommandsAsync() override;

    // ICommandContext Implementation
    Interfaces::IBreakpointManager* GetBreakpointManager() override { return &_breakpointManager; }
    [[nodiscard]] bool Resume(std::unique_ptr<Interfaces::IResumePlan> plan) override;
    void EndSession() override;
    bool ExecuteReadMemory(DWORD_PTR va, size_t size, std::shared_ptr<std::vector<BYTE>>& outDataBuffer) override;
    bool ExecuteWriteMemory(DWORD_PTR va, const BYTE* data, size_t size) override;
    bool ExecuteDisassemble(DWORD_PTR address, size_t length, std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>>& outInstructions) override;
    std::string ExecuteTryResolveString(DWORD_PTR address) override;

  private:
    void PostCommand(Commands::Base::CommandPtr command);
    bool EnqueueCommand(Commands::Base::CommandPtr cmd);
    void NotifyDebuggerThread(bool isPaused);

    /// <summary>
    /// Executes the queued commands in order until one of them ends the processing of the stop, and returns
    /// the outcome that command left. Empty when the queue ran empty first: the stop goes on.
    /// </summary>
    std::optional<Interfaces::StopOutcome> ExecutePendingCommandsOfTheStop();
    void ExecuteAllPendingCommands();

    void ProcessCommand(Commands::Base::CommandPtr cmd);

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
    bool _paused = false; // True while the debug thread processes a stop

    // How the processing of the current stop ends, left by the command that ends it while it executes and
    // taken by the processing right behind that command. Empty at any other time. Guarded by _mutex
    std::optional<Interfaces::StopOutcome> _stopOutcome;

    DWORD _currentProcessId;
    DWORD _currentThreadId;
  };
}