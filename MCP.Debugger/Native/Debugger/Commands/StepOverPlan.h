#pragma once
#include <Windows.h>
#include <optional>
#include "..\Interfaces\IResumePlan.h"

namespace MCP::Native::Debugger::Commands
{
  /// <summary>
  /// The plan of a step over: the debuggee executes the instruction at the stop address and halts - unless
  /// that instruction is one to run past (a call or a repeated instruction). Then the plan places a
  /// breakpoint behind it and lets the debuggee run on until it gets there. That breakpoint belongs to the
  /// plan: it is removed when the plan ends, whether the debuggee reached it or another stop came first.
  /// </summary>
  class StepOverPlan : public MCP::Native::Debugger::Interfaces::IResumePlan
  {
  public:
    void Begin(MCP::Native::Debugger::Interfaces::IStopSite& stopSite) override;
    bool HaltsAfterFirstInstruction() const override;
    bool OwnsBreakpointAt(DWORD_PTR address) const override;
    void End(MCP::Native::Debugger::Interfaces::IStopSite& stopSite) override;

  private:
    // Where the breakpoint behind the instruction to run past was placed; empty while the plan owns none
    std::optional<DWORD_PTR> _returnAddress;
  };
}
