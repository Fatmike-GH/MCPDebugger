#pragma once

namespace MCP::Debugger
{
  /// <summary>
  /// Receives the events of the native debugger as managed event arguments. The native event handler adapter
  /// writes to it.
  /// </summary>
  /// <remarks>
  /// Threading: every member is called on the debugger thread while the debuggee is stopped for the event. An
  /// implementation must return quickly and must not call back into the debugger, which would wait for the
  /// thread it is called on.
  /// Visibility: a contract between the classes of this assembly, not part of its public surface.
  /// </remarks>
  interface class IDebuggerEventSink
  {
  public:
    /// <summary>The debuggee process has ended.</summary>
    void OnExitProcess(MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^ args);

    /// <summary>The debuggee stopped at the system breakpoint.</summary>
    void OnSystemBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);

    /// <summary>The debuggee stopped at its entry point.</summary>
    void OnEntryPointBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);

    /// <summary>The debuggee stopped at a breakpoint set by the client.</summary>
    void OnBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);

    /// <summary>The debuggee stopped after a single step.</summary>
    void OnSingleStep(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
  };
}
