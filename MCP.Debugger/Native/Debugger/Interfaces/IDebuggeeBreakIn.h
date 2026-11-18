#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// Interrupts a running debuggee: the process raises a breakpoint exception, so the debug thread receives a
  /// debug event and gets the chance to execute what was queued for it.
  /// </summary>
  /// <remarks>
  /// Threading: may be called from any thread.
  /// Failures are not reported: a request for a process that cannot be interrupted has no effect.
  /// </remarks>
  class IDebuggeeBreakIn
  {
  public:
    virtual ~IDebuggeeBreakIn() = default;

    /// <summary>Asks the process behind the handle to break in. The handle is borrowed for the call.</summary>
    virtual void RequestBreakIn(HANDLE processHandle) = 0;
  };
}
