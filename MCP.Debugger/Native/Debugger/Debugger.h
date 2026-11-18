#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include ".\Interfaces\IDebugger.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;
  class IDebuggerSession;
  class IDebuggerSessionFactory;
}

namespace MCP::Native::PEFile::Interfaces
{
  class IExecutableValidator;
}

namespace MCP::Native::Debugger
{
  /// <summary>
  /// The debugger facade: checks the executable, asks the session factory for a new session on every start
  /// and passes each operation to the session's debug thread while that thread is running.
  /// </summary>
  class Debugger : public Interfaces::IDebugger
  {
  public:
    /// <summary>
    /// Takes ownership of the session factory and the executable validator. The event handler is borrowed and
    /// must outlive the debugger; it is handed to every session.
    /// </summary>
    Debugger(std::unique_ptr<Interfaces::IDebuggerSessionFactory> sessionFactory,
             std::unique_ptr<MCP::Native::PEFile::Interfaces::IExecutableValidator> executableValidator,
             Interfaces::IDebuggerEventHandler& eventHandler);

    /// <summary>Stops a running session and waits for its debug thread.</summary>
    ~Debugger() override;

    Debugger(const Debugger&) = delete;
    Debugger& operator=(const Debugger&) = delete;
    Debugger(Debugger&&) = delete;
    Debugger& operator=(Debugger&&) = delete;

    bool IsRunning() const override;
    bool Start(const std::string& path, std::string& error) override;

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

    PROCESS_INFORMATION GetProcessInformation() const override;
    DWORD_PTR GetEntryPoint(DWORD processId) const override;
    DWORD_PTR GetImageBase(DWORD processId) const override;
    MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer* GetProcessExplorer() const override;

  private:
    std::unique_ptr<Interfaces::IDebuggerSessionFactory> _sessionFactory;
    std::unique_ptr<MCP::Native::PEFile::Interfaces::IExecutableValidator> _executableValidator;
    Interfaces::IDebuggerEventHandler& _eventHandler;

    // The current or the last session. It is kept after its thread stopped, so its module bookkeeping stays
    // readable, and is replaced by the next start.
    std::unique_ptr<Interfaces::IDebuggerSession> _session;
  };
}
