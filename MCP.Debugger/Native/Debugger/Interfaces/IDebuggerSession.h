#pragma once
#include <windows.h>

namespace MCP::Native::Debugger::DebugEventHandler
{
  struct ProcessInfo;
}

namespace MCP::Native::ProcessExplorer::Interfaces
{
  class IProcessExplorer;
}

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerThread;

  /// <summary>
  /// The objects of one debugging session: the debug thread and the bookkeeping it fills. Created not started;
  /// the session is started and stopped through its thread.
  /// </summary>
  /// <remarks>
  /// Ownership: the session owns its parts and hands them out as references that are valid as long as the
  /// session. Destroying the session stops a running debuggee and waits for the debug thread to end.
  /// Threading: the members are called from client threads; the returned objects state their own rules.
  /// </remarks>
  class IDebuggerSession
  {
  public:
    virtual ~IDebuggerSession() = default;

    virtual IDebuggerThread& GetDebuggerThread() = 0;
    virtual MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer& GetProcessExplorer() = 0;

    /// <summary>Copies what the session recorded about a debuggee process. False when the process is unknown.</summary>
    virtual bool GetProcessById(DWORD processId, MCP::Native::Debugger::DebugEventHandler::ProcessInfo& outInfo) = 0;
  };
}
