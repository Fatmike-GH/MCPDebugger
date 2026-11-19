#include "StepOverPlan.h"
#include "..\Interfaces\IStopSite.h"

namespace MCP::Native::Debugger::Commands
{
  void StepOverPlan::Begin(MCP::Native::Debugger::Interfaces::IStopSite& stopSite)
  {
    // Without a length there is nothing to run past, or the instruction is unknown: no breakpoint may be
    // placed behind an instruction whose length is not known, so the step then is a single step
    std::optional<size_t> callLength = stopSite.TryGetCallLength();
    if (!callLength)
    {
      return;
    }

    _returnAddress = stopSite.GetStopAddress() + *callLength;
    stopSite.SetBreakpoint(*_returnAddress);
  }

  bool StepOverPlan::HaltsAfterFirstInstruction() const
  {
    // With a breakpoint placed the first instruction led into the code to run past: the debuggee runs on
    // to the breakpoint
    return !_returnAddress.has_value();
  }

  bool StepOverPlan::OwnsBreakpointAt(DWORD_PTR address) const
  {
    return _returnAddress == address;
  }

  void StepOverPlan::End(MCP::Native::Debugger::Interfaces::IStopSite& stopSite)
  {
    if (!_returnAddress)
    {
      return;
    }

    stopSite.RemoveBreakpoint(*_returnAddress);
    _returnAddress.reset();
  }
}
