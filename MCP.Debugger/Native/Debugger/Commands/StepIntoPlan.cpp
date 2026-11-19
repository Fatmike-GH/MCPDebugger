#include "StepIntoPlan.h"

namespace MCP::Native::Debugger::Commands
{
  void StepIntoPlan::Begin(MCP::Native::Debugger::Interfaces::IStopSite& stopSite)
  {
  }

  bool StepIntoPlan::HaltsAfterFirstInstruction() const
  {
    return true;
  }

  bool StepIntoPlan::OwnsBreakpointAt(DWORD_PTR address) const
  {
    return false;
  }

  void StepIntoPlan::End(MCP::Native::Debugger::Interfaces::IStopSite& stopSite)
  {
  }
}
