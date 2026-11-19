#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::Interfaces
{
  class IStopSite;
}

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// What a command that resumes the stopped debuggee wants to happen: how far the debuggee runs and what
  /// has to be prepared for that. The command creates its plan and hands it to the debug thread, which
  /// drives it; everything in which resuming commands differ is answered here, so the debug thread resumes
  /// the debuggee without knowing which command it serves.
  /// </summary>
  /// <remarks>
  /// A plan is driven in this order: Begin once at the stop it resumes the debuggee from; then the queries,
  /// any number of times, while the debuggee runs; End once at the next stop that is reported, whatever its
  /// kind. A plan that is never begun is destroyed without any call.
  /// Threading: debug thread only.
  /// Ownership: the stop sites are borrowed for the duration of the call.
  /// </remarks>
  class IResumePlan
  {
  public:
    virtual ~IResumePlan() = default;

    /// <summary>
    /// Prepares the run at the stop the debuggee is resumed from, before it leaves the stop address.
    /// </summary>
    virtual void Begin(IStopSite& stopSite) = 0;

    /// <summary>
    /// Tells whether the debuggee halts again as soon as the instruction at the stop address has run. When
    /// not, it runs on until something else stops it.
    /// </summary>
    [[nodiscard]] virtual bool HaltsAfterFirstInstruction() const = 0;

    /// <summary>
    /// Tells whether a breakpoint at the address is one the plan placed itself and has not given up yet.
    /// The debuggee reaching it is the end of the run the plan asked for, not a breakpoint of the client.
    /// </summary>
    [[nodiscard]] virtual bool OwnsBreakpointAt(DWORD_PTR address) const = 0;

    /// <summary>
    /// Ends the plan at the stop that follows its run: the plan gives up what it placed and still owns, and
    /// owns nothing afterwards.
    /// </summary>
    virtual void End(IStopSite& stopSite) = 0;
  };
}
