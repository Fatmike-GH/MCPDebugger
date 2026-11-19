#pragma once
#include "..\Interfaces\IResumePlan.h"

namespace MCP::Native::Debugger::Commands
{
  /// <summary>
  /// The plan of a step into: the debuggee executes the one instruction at the stop address, whatever it
  /// is, and halts. It prepares nothing at the stop and places nothing.
  /// </summary>
  class StepIntoPlan : public MCP::Native::Debugger::Interfaces::IResumePlan
  {
  public:
    void Begin(MCP::Native::Debugger::Interfaces::IStopSite& stopSite) override;
    bool HaltsAfterFirstInstruction() const override;
    bool OwnsBreakpointAt(DWORD_PTR address) const override;
    void End(MCP::Native::Debugger::Interfaces::IStopSite& stopSite) override;
  };
}
