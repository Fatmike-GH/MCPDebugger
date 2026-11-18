#pragma once
#include <Windows.h>
#include <memory>
#include "IBreakpoint.h"

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// Creates the breakpoints of one debuggee process. The breakpoint manager asks for one per address, so
  /// its bookkeeping does not depend on how a breakpoint reaches the debuggee's memory.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only, like the breakpoints it creates.
  /// Ownership: the caller owns the returned breakpoint.
  /// </remarks>
  class IBreakpointFactory
  {
  public:
    virtual ~IBreakpointFactory() = default;

    /// <summary>Returns a new breakpoint for the address that is not enabled yet; never null.</summary>
    virtual std::unique_ptr<IBreakpoint> Create(DWORD_PTR va) = 0;
  };
}
