#include "StopSite.h"
#include ".\Interfaces\IBreakpointManager.h"
#include "..\Disassembler\Interfaces\IDisassembler.h"

namespace MCP::Native::Debugger
{
  StopSite::StopSite(HANDLE processHandle,
                     DWORD_PTR stopAddress,
                     Interfaces::IBreakpointManager& breakpointManager,
                     MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler)
    : _processHandle(processHandle),
      _stopAddress(stopAddress),
      _breakpointManager(breakpointManager),
      _disassembler(disassembler)
  {
  }

  DWORD_PTR StopSite::GetStopAddress() const
  {
    return _stopAddress;
  }

  std::optional<size_t> StopSite::TryGetCallLength()
  {
    size_t instructionLength = 0;
    if (!_disassembler.IsCallInstruction(_processHandle, _stopAddress, instructionLength))
    {
      return std::nullopt;
    }
    return instructionLength;
  }

  void StopSite::SetBreakpoint(DWORD_PTR address)
  {
    _breakpointManager.SetBreakpoint(address);
  }

  void StopSite::RemoveBreakpoint(DWORD_PTR address)
  {
    // False only tells that the address had no breakpoint any more, which is what the caller wants
    _breakpointManager.RemoveBreakpoint(address);
  }
}
