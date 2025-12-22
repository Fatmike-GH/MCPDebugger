#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include ".\Commands\Base\ICommand.h"
#include ".\Commands\CommandResult.h"
#include ".\Commands\DisassembleResult.h"
#include ".\Commands\ReadMemoryResult.h"
#include ".\Commands\TryResolveStringResult.h"

namespace MCP::Native::Debugger
{
  class DebuggerThread;
}

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
}

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer;
}

namespace MCP::Native::Debugger
{
  class Debugger
  {
  public:
    Debugger(Interfaces::IDebuggerEventHandler* eventHandler);
    ~Debugger();

    bool IsRunning() const;
    bool Start(const std::string& path, std::string& error);

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

    PROCESS_INFORMATION GetProcessInformation() const;
    DWORD_PTR GetEntryPoint(DWORD processId) const;
    DWORD_PTR GetImageBase(DWORD processId) const;
    MCP::Native::ProcessExplorer::ProcessExplorer* GetProcessExplorer() const { return _processExplorer.get(); }

  private:
    bool ValidateExecutable(const std::string& path, std::string& error);
    void Initialize();
    bool StartDebuggerThread(const std::string& path);

  private:
    std::unique_ptr<MCP::Native::ProcessExplorer::ProcessExplorer> _processExplorer;
    std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::DebugEventStorage> _debugEventStorage;
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* _eventHandler;

    std::unique_ptr<DebuggerThread> _debuggerThread;    
  };
}