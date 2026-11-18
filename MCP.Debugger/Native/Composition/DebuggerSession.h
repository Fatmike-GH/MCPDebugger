#pragma once
#include <windows.h>
#include <memory>
#include "..\Debugger\Interfaces\IDebuggerSession.h"

namespace MCP::Native::Debugger
{
  class DebuggerThread;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
}

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer;
}

namespace MCP::Native::Composition
{
  /// <summary>
  /// Owns the objects of one debugging session: the module bookkeeping, the debug event bookkeeping and the
  /// debug thread that fills both.
  /// </summary>
  class DebuggerSession : public MCP::Native::Debugger::Interfaces::IDebuggerSession
  {
  public:
    /// <summary>
    /// Takes ownership of the three parts. The thread must have been created with a factory that borrows this
    /// explorer and this storage: what the thread creates through it uses them until the thread is destroyed.
    /// </summary>
    DebuggerSession(std::unique_ptr<MCP::Native::ProcessExplorer::ProcessExplorer> processExplorer,
                    std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::DebugEventStorage> debugEventStorage,
                    std::unique_ptr<MCP::Native::Debugger::DebuggerThread> debuggerThread);
    ~DebuggerSession() override;

    DebuggerSession(const DebuggerSession&) = delete;
    DebuggerSession& operator=(const DebuggerSession&) = delete;
    DebuggerSession(DebuggerSession&&) = delete;
    DebuggerSession& operator=(DebuggerSession&&) = delete;

    MCP::Native::Debugger::Interfaces::IDebuggerThread& GetDebuggerThread() override;
    MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer& GetProcessExplorer() override;
    bool GetProcessById(DWORD processId, MCP::Native::Debugger::DebugEventHandler::ProcessInfo& outInfo) override;

  private:
    // Declaration order is destruction order reversed: the thread is destroyed first, which stops the
    // debuggee and joins the debug thread while the explorer and the storage it writes to still exist.
    std::unique_ptr<MCP::Native::ProcessExplorer::ProcessExplorer> _processExplorer;
    std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::DebugEventStorage> _debugEventStorage;
    std::unique_ptr<MCP::Native::Debugger::DebuggerThread> _debuggerThread;
  };
}
