#pragma once
#include <Windows.h>
#include <memory>
#include "..\Debugger\Interfaces\IStopSiteFactory.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpointManager;
}

namespace MCP::Native::Disassembler::Interfaces
{
  class IDisassembler;
}

namespace MCP::Native::Composition
{
  /// <summary>
  /// Creates the stop sites of one debuggee process with the stop site class of this library.
  /// </summary>
  class StopSiteFactory : public MCP::Native::Debugger::Interfaces::IStopSiteFactory
  {
  public:
    /// <summary>
    /// The breakpoint manager and the disassembler are borrowed: every created stop site uses them, so they
    /// must outlive the factory and the stop sites.
    /// </summary>
    StopSiteFactory(MCP::Native::Debugger::Interfaces::IBreakpointManager& breakpointManager,
                    MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler);

    std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSite> Create(HANDLE processHandle, DWORD_PTR stopAddress) override;

  private:
    MCP::Native::Debugger::Interfaces::IBreakpointManager& _breakpointManager;
    MCP::Native::Disassembler::Interfaces::IDisassembler& _disassembler;
  };
}
