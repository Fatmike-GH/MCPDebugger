#include "ContinuePlan.h"

namespace MCP::Native::Debugger::Commands
{
  void ContinuePlan::Begin(MCP::Native::Debugger::Interfaces::IStopSite& stopSite)
  {
  }

  bool ContinuePlan::HaltsAfterFirstInstruction() const
  {
    return false;
  }

  bool ContinuePlan::OwnsBreakpointAt(DWORD_PTR address) const
  {
    return false;
  }

  void ContinuePlan::End(MCP::Native::Debugger::Interfaces::IStopSite& stopSite)
  {
  }
}
