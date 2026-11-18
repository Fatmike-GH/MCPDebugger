#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// The software breakpoints of one debuggee process and the single-step switch of its threads.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only - every member reads or writes debuggee state.
  /// Ownership: the manager owns its breakpoints and hands none of them out, so removing a breakpoint cannot
  /// invalidate anything a caller holds.
  /// Held address: a breakpoint instruction at the address the debuggee is stopped at would be executed by
  /// the next go command before the instruction it replaced, and a memory read during the stop would show it
  /// instead of the program's byte. The manager is therefore told the stop address (hold) and keeps the
  /// breakpoint instruction out of memory there until it is told that the debuggee has left the address
  /// (release). A breakpoint at any other address is in memory from the moment it is set until it is removed.
  /// At most one address is held.
  /// </remarks>
  class IBreakpointManager
  {
  public:
    virtual ~IBreakpointManager() = default;

    /// <summary>
    /// Sets a breakpoint at the address, creating it when the address has none. Its breakpoint instruction is
    /// written at once - except at the held address, where it is written when the hold is released.
    /// </summary>
    /// <returns>Always true.</returns>
    virtual bool SetBreakpoint(DWORD_PTR address) = 0;

    /// <summary>
    /// Removes the breakpoint at the address: restores the original byte and forgets the breakpoint, so a
    /// later set creates it anew. A hold on the address stays.
    /// </summary>
    /// <returns>False when the address has no breakpoint.</returns>
    virtual bool RemoveBreakpoint(DWORD_PTR address) = 0;

    /// <summary>Tells whether a breakpoint is set at the address: set and not removed since.</summary>
    virtual bool HasBreakpoint(DWORD_PTR address) = 0;

    /// <summary>
    /// Holds the address the debuggee is stopped at: the breakpoint instruction of a breakpoint there is taken
    /// out of memory and stays out, also for a breakpoint set there later, until the hold is released. A hold
    /// on another address is released first.
    /// </summary>
    virtual void HoldBreakpointAt(DWORD_PTR address) = 0;

    /// <summary>
    /// Releases the held address once the debuggee has left it: the breakpoint instruction of a breakpoint
    /// set there is written. Does nothing when no address is held.
    /// </summary>
    virtual void ReleaseHeldBreakpoint() = 0;

    /// <summary>Sets or clears the trap flag of a debuggee thread, so that it stops after one instruction.</summary>
    virtual void SetTrapFlag(HANDLE threadHandle, bool enable) = 0;
  };
}
