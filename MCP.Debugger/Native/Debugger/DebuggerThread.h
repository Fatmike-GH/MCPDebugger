#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include "BreakpointManager.h"
#include ".\Commands\Base\ICommand.h"
#include ".\Commands\CommandResult.h"
#include ".\Commands\DisassembleResult.h"
#include ".\Commands\ReadMemoryResult.h"
#include ".\Commands\TryResolveStringResult.h"
#include ".\DebugEventHandler\HandleResult.h"
#include "..\Threading\EventWaiter.h"

namespace MCP::Native::Debugger
{
  class DebuggerCommandQueue;
}

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
  class DebugEventHandler;
}

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer;
}

namespace MCP::Native::Debugger
{
  class DebuggerThread
  {
  public:
    DebuggerThread(MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                   MCP::Native::Debugger::DebugEventHandler::DebugEventStorage* debugEventStorage,
                   Interfaces::IDebuggerEventHandler* eventHandler);
    ~DebuggerThread();

    bool Start(const std::string& path);
    
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

    PROCESS_INFORMATION GetProcessInformation();
    bool IsRunning() const { return Is(State::Running); }  

  private:
    enum class State
    {
      Stopped,
      Starting,
      Running,
      Stopping
    };

    void ResetMembers();
    bool LaunchThread(const std::string& path);
    bool WaitForInitialization();
    void WaitForDebuggerThread();

    void ThreadMain(std::string path);
    bool CreateDebugSession(const std::string& path);
    bool Launch(const std::string& path);
    void RunDebugLoop();
    void CleanupDebugSession();

    void SetState(State s) { _state.store(s, std::memory_order_release); }
    State GetState() const { return _state.load(std::memory_order_acquire); }
    bool Is(State s) const { return GetState() == s; }

  private:
    std::atomic<State> _state{ State::Stopped };

    std::thread _thread;
    MCP::Native::Threading::EventWaiter _startupWaiter;

    MCP::Native::ProcessExplorer::ProcessExplorer* _processExplorer;
    MCP::Native::Debugger::DebugEventHandler::DebugEventStorage* _debugEventStorage;
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* _eventHandler;
    PROCESS_INFORMATION _processInfo;
    std::unique_ptr<DebuggerCommandQueue> _commandQueue;
    std::unique_ptr<BreakpointManager> _breakpointManager;
    std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::DebugEventHandler> _debugEventHandler;
  };
}