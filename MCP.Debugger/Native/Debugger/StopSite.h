#pragma once
#include <Windows.h>
#include <optional>
#include ".\Interfaces\IStopSite.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpointManager;
}

namespace MCP::Native::Disassembler::Interfaces
{
  class IDisassembler;
}

namespace MCP::Native::Debugger
{
  /// <summary>
  /// The stop site of one reported stop: answers from the debuggee's code at the stop address and places
  /// breakpoints through the breakpoint manager of the process.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only.
  /// </remarks>
  class StopSite : public Interfaces::IStopSite
  {
  public:
    /// <summary>
    /// The process handle, the breakpoint manager and the disassembler are borrowed and must outlive the
    /// stop site.
    /// </summary>
    StopSite(HANDLE processHandle,
             DWORD_PTR stopAddress,
             Interfaces::IBreakpointManager& breakpointManager,
             MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler);

    DWORD_PTR GetStopAddress() const override;
    std::optional<size_t> TryGetCallLength() override;
    void SetBreakpoint(DWORD_PTR address) override;
    void RemoveBreakpoint(DWORD_PTR address) override;

  private:
    HANDLE _processHandle;
    DWORD_PTR _stopAddress;
    Interfaces::IBreakpointManager& _breakpointManager;
    MCP::Native::Disassembler::Interfaces::IDisassembler& _disassembler;
  };
}
