#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// One software breakpoint at a fixed address of the debuggee: enabled, it replaces the instruction byte at
  /// that address; disabled, the original byte is back.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only - enabling and disabling write into the debuggee's memory.
  /// Ownership: owned by the breakpoint manager that created it.
  /// </remarks>
  class IBreakpoint
  {
  public:
    virtual ~IBreakpoint() = default;

    /// <summary>The address the breakpoint was created for.</summary>
    virtual DWORD_PTR GetVa() const = 0;

    /// <summary>Writes the breakpoint instruction; does nothing when already enabled.</summary>
    virtual void Enable() = 0;

    /// <summary>Restores the original byte; does nothing when not enabled.</summary>
    virtual void Disable() = 0;

    /// <summary>True while the breakpoint instruction is written at the address.</summary>
    virtual bool IsEnabled() const = 0;
  };
}
