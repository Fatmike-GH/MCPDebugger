#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpoint;

  /// <summary>
  /// The software breakpoints of one debuggee process and the single-step switch of its threads.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only - every member reads or writes debuggee state.
  /// Ownership: the manager owns its breakpoints; a breakpoint it hands out is borrowed and stays valid as
  /// long as the manager.
  /// </remarks>
  class IBreakpointManager
  {
  public:
    virtual ~IBreakpointManager() = default;

    /// <summary>Enables the breakpoint at the address, creating it when the address has none yet.</summary>
    virtual bool SetBreakpoint(DWORD_PTR address) = 0;

    /// <summary>Disables the breakpoint at the address. False when the address has no breakpoint.</summary>
    virtual bool RemoveBreakpoint(DWORD_PTR address) = 0;

    /// <summary>The breakpoint at the address; null when the address has none.</summary>
    virtual IBreakpoint* GetBreakpoint(DWORD_PTR address) = 0;

    /// <summary>Sets or clears the trap flag of a debuggee thread, so that it stops after one instruction.</summary>
    virtual void SetTrapFlag(HANDLE threadHandle, bool enable) = 0;
  };
}
