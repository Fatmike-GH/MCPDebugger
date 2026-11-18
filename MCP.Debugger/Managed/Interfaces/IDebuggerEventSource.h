#pragma once

namespace MCP::Debugger
{
  /// <summary>
  /// Publishes the events of the native debugger inside this assembly. The debugger service subscribes to it.
  /// </summary>
  /// <remarks>
  /// Threading: every event is raised synchronously on the debugger thread. A subscriber only hands the event
  /// on to a dispatcher; it never runs client code or calls the debugger from its handler.
  /// Lifetime: a subscriber that ends before the source unsubscribes.
  /// Visibility: a contract between the classes of this assembly, not part of its public surface.
  /// </remarks>
  interface class IDebuggerEventSource
  {
  public:
    /// <summary>The debuggee process has ended.</summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^>^ ExitProcess;

    /// <summary>The debuggee stopped at the system breakpoint.</summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SystemBreakpoint;

    /// <summary>The debuggee stopped at its entry point.</summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ EntryPointBreakpoint;

    /// <summary>The debuggee stopped at a breakpoint set by the client.</summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ Breakpoint;

    /// <summary>The debuggee stopped after a single step.</summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SingleStep;
  };
}
