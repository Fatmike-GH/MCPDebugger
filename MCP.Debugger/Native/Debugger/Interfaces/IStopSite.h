#pragma once
#include <Windows.h>
#include <optional>

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// One reported stop of the debuggee, as a resume plan sees it: the address the debuggee stands at and the
  /// few things a plan may do there. A plan gets the stop it is begun at and, later, the stop that ends it.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only - every member reads or writes debuggee state.
  /// Ownership: a plan borrows the stop site for the duration of one call and must not keep it.
  /// </remarks>
  class IStopSite
  {
  public:
    virtual ~IStopSite() = default;

    /// <summary>The address the debuggee is stopped at.</summary>
    [[nodiscard]] virtual DWORD_PTR GetStopAddress() const = 0;

    /// <summary>
    /// The length of the instruction at the stop address when it is one a step over must run past instead
    /// of into - a call or an instruction with a repeat prefix. Empty when it is another instruction, when
    /// the memory there holds no instruction or when it cannot be read.
    /// </summary>
    [[nodiscard]] virtual std::optional<size_t> TryGetCallLength() = 0;

    /// <summary>Sets a breakpoint at the address in the process of the stop.</summary>
    virtual void SetBreakpoint(DWORD_PTR address) = 0;

    /// <summary>Removes the breakpoint at the address in the process of the stop.</summary>
    virtual void RemoveBreakpoint(DWORD_PTR address) = 0;
  };
}
