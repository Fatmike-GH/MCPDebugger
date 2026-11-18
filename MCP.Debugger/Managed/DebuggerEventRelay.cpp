// Included before any header of a managed type: the Win32 headers rename identifiers through their A/W
// macros, and every source file that sees those headers must see the member names of a managed type the
// same way as the others. The relay's interfaces are also included by source files that use the Win32 API.
#include <Windows.h>

#include "DebuggerEventRelay.h"

namespace MCP::Debugger
{
  void DebuggerEventRelay::OnExitProcess(MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^ args)
  {
    ExitProcess(this, args);
  }

  void DebuggerEventRelay::OnSystemBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    SystemBreakpoint(this, args);
  }

  void DebuggerEventRelay::OnEntryPointBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    EntryPointBreakpoint(this, args);
  }

  void DebuggerEventRelay::OnBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    Breakpoint(this, args);
  }

  void DebuggerEventRelay::OnSingleStep(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    SingleStep(this, args);
  }
}
