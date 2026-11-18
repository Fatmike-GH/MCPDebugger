#pragma once

namespace MCP::Native::Debugger::Interfaces
{
  class IDebugger;
}

namespace MCP::Debugger
{
  /// <summary>
  /// Gives the managed classes of this assembly access to the native debugger, which a managed class cannot
  /// hold as a member itself.
  /// </summary>
  /// <remarks>
  /// Ownership: the host owns the native debugger. A user of this interface borrows the pointer for one call
  /// and never deletes it.
  /// Threading: the property may be read from any thread; the calls made through the pointer follow the rules
  /// of the native debugger.
  /// Visibility: a contract between the classes of this assembly, not part of its public surface.
  /// </remarks>
  interface class INativeDebuggerHost
  {
  public:
    /// <summary>The native debugger; valid while the host is alive and not disposed.</summary>
    /// <exception cref="System::ObjectDisposedException">The host was disposed.</exception>
    property MCP::Native::Debugger::Interfaces::IDebugger* Debugger
    {
      MCP::Native::Debugger::Interfaces::IDebugger* get();
    }
  };
}
