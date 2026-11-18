#pragma once
#include <Windows.h>
#include "..\HandleResult.h"

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  /// <summary>
  /// Routes the debug events of a debuggee: every event goes to the handler that is responsible for its
  /// event code.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only - the handlers use the debug API, which belongs to the thread that
  /// launched the debuggee.
  /// Ownership: the event is borrowed for the duration of the call.
  /// </remarks>
  class IDebugEventDispatcher
  {
  public:
    virtual ~IDebugEventDispatcher() = default;

    /// <summary>
    /// Hands the event to the handler of its event code and returns what that handler decided: how the
    /// debuggee continues and whether it exited. Blocks while the debuggee is stopped at a breakpoint or
    /// step, until an operation resumes it.
    /// </summary>
    virtual MCP::Native::Debugger::DebugEventHandler::HandleResult HandleDebugEvent(DEBUG_EVENT& debugEvent) = 0;
  };
}
