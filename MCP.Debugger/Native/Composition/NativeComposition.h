#pragma once
#include <memory>
#include "..\Debugger\Interfaces\IDebugger.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;
}

namespace MCP::Native::Composition
{
  /// <summary>
  /// Wires the native debugger core: creates the debugger with the factories and checks it needs. This
  /// function and the factory classes next to it are the only native code that names the concrete classes
  /// of the core; everything else receives its collaborators as interfaces.
  /// </summary>
  /// <remarks>
  /// Ownership: the caller owns the returned debugger. The event handler is borrowed and must outlive it:
  /// the debug thread of every session reports its events to it.
  /// Threading: may be called from any thread; the debugger starts no thread before its first start.
  /// </remarks>
  [[nodiscard]] std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebugger> CreateDebugger(
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler);
}
