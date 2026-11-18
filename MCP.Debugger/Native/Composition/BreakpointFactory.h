#pragma once
#include <Windows.h>
#include <memory>
#include "..\Debugger\Interfaces\IBreakpointFactory.h"

namespace MCP::Native::Composition
{
  /// <summary>
  /// Creates the software breakpoints of one debuggee process with the breakpoint class of this library.
  /// </summary>
  class BreakpointFactory : public MCP::Native::Debugger::Interfaces::IBreakpointFactory
  {
  public:
    /// <summary>
    /// The process handle is borrowed: every created breakpoint writes into that process, so the handle must
    /// stay valid as long as the breakpoints.
    /// </summary>
    explicit BreakpointFactory(HANDLE processHandle);

    std::unique_ptr<MCP::Native::Debugger::Interfaces::IBreakpoint> Create(DWORD_PTR va) override;

  private:
    HANDLE _processHandle;
  };
}
