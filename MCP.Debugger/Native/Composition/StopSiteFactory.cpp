#include "StopSiteFactory.h"
#include "..\Debugger\StopSite.h"

namespace MCP::Native::Composition
{
  StopSiteFactory::StopSiteFactory(MCP::Native::Debugger::Interfaces::IBreakpointManager& breakpointManager,
                                   MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler)
    : _breakpointManager(breakpointManager),
      _disassembler(disassembler)
  {
  }

  std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSite> StopSiteFactory::Create(HANDLE processHandle, DWORD_PTR stopAddress)
  {
    return std::make_unique<MCP::Native::Debugger::StopSite>(processHandle, stopAddress, _breakpointManager, _disassembler);
  }
}
