#pragma once
#include <Windows.h>
#include <memory>
#include "IDebugThreadServices.h"

namespace MCP::Native::Threading
{
  class EventWaiter;
}

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// Creates what a debug thread needs to serve the debuggee it just launched. The debug thread asks once per
  /// launch, so nothing of an earlier debuggee survives.
  /// </summary>
  /// <remarks>
  /// Threading: called on the debug thread, after the debuggee was launched - the created objects use the
  /// debug API, which belongs to the thread that launched the process.
  /// Ownership: the caller owns the result. The process handle and the waiter are borrowed; both must stay
  /// valid as long as the result.
  /// </remarks>
  class IDebugThreadServicesFactory
  {
  public:
    virtual ~IDebugThreadServicesFactory() = default;

    /// <summary>
    /// Returns the services for the debuggee behind the process handle; never null. The waiter is released
    /// when the debuggee reached its first stop.
    /// </summary>
    virtual std::unique_ptr<IDebugThreadServices> Create(HANDLE processHandle, MCP::Native::Threading::EventWaiter& startupWaiter) = 0;
  };
}
