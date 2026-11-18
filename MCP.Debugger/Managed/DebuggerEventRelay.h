#pragma once

#include ".\Interfaces\IDebuggerEventSink.h"
#include ".\Interfaces\IDebuggerEventSource.h"

namespace MCP::Debugger
{
  /// <summary>
  /// Passes every debugger event it receives on to its subscribers. It stands between the native event handler
  /// adapter, which writes to it, and the debugger service, which subscribes to it: neither of the two has to
  /// know the other, so both can be created without a reference cycle.
  /// </summary>
  /// <remarks>
  /// Threading: an event is raised on the thread that called the matching On... method, i.e. the debugger
  /// thread, before that method returns.
  /// </remarks>
  ref class DebuggerEventRelay sealed : public IDebuggerEventSink, public IDebuggerEventSource
  {
  public:
    /// <inheritdoc/>
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^>^ ExitProcess;
    /// <inheritdoc/>
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SystemBreakpoint;
    /// <inheritdoc/>
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ EntryPointBreakpoint;
    /// <inheritdoc/>
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ Breakpoint;
    /// <inheritdoc/>
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SingleStep;

    /// <inheritdoc/>
    virtual void OnExitProcess(MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^ args);
    /// <inheritdoc/>
    virtual void OnSystemBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    /// <inheritdoc/>
    virtual void OnEntryPointBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    /// <inheritdoc/>
    virtual void OnBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    /// <inheritdoc/>
    virtual void OnSingleStep(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
  };
}
