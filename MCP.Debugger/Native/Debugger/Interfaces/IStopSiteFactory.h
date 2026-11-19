#pragma once
#include <Windows.h>
#include <memory>
#include "IStopSite.h"

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// Creates the stop site of each reported stop, so that the handler of the stops does not depend on how a
  /// stop site reaches the debuggee.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only, like the stop sites it creates.
  /// Ownership: the caller owns the returned stop site; the process handle is borrowed by it and must stay
  /// valid as long as the stop site.
  /// </remarks>
  class IStopSiteFactory
  {
  public:
    virtual ~IStopSiteFactory() = default;

    /// <summary>
    /// Returns a new stop site for a debuggee that stands at the address in the given process; never null.
    /// </summary>
    [[nodiscard]] virtual std::unique_ptr<IStopSite> Create(HANDLE processHandle, DWORD_PTR stopAddress) = 0;
  };
}
