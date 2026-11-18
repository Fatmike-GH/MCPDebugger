#include "BreakpointFactory.h"
#include "..\Debugger\Breakpoint.h"

namespace MCP::Native::Composition
{
  BreakpointFactory::BreakpointFactory(HANDLE processHandle)
    : _processHandle(processHandle)
  {
  }

  std::unique_ptr<MCP::Native::Debugger::Interfaces::IBreakpoint> BreakpointFactory::Create(DWORD_PTR va)
  {
    return std::make_unique<MCP::Native::Debugger::Breakpoint>(_processHandle, va);
  }
}
