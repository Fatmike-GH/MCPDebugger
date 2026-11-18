#pragma once

#include <memory>
#include ".\Interfaces\INativeDebuggerHost.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IDebugger;
  class IDebuggerEventHandler;
}

namespace MCP::Debugger
{
  struct NativeDebuggerObjects;

  /// <summary>
  /// Owns the native debugger and the native event handler the debugger reports to, and releases both in the
  /// order they need: the debugger first, because it uses the event handler until its debugger thread ended.
  /// </summary>
  /// <remarks>
  /// Disposing the host ends a running debugging session and waits for the debugger thread.
  /// The finalizer releases the native objects, in the same order, of a host that became unreachable without
  /// being disposed. That only happens after the debugger service was disposed: until then the native event
  /// handler keeps the event relay alive, the relay's subscriptions keep the service alive and the service
  /// keeps this host alive, so a host whose service is never disposed lives until the process ends.
  /// </remarks>
  ref class NativeDebuggerHost sealed : public INativeDebuggerHost
  {
  public:
    /// <summary>Takes ownership of both native objects.</summary>
    /// <param name="eventHandler">The event handler the debugger was created with.</param>
    /// <param name="debugger">The native debugger.</param>
    NativeDebuggerHost(std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerEventHandler> eventHandler,
                       std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebugger> debugger);

    /// <summary>Releases the native objects.</summary>
    ~NativeDebuggerHost();

    /// <summary>Releases the native objects of a host that became unreachable without being disposed.</summary>
    !NativeDebuggerHost();

    /// <inheritdoc/>
    virtual property MCP::Native::Debugger::Interfaces::IDebugger* Debugger
    {
      MCP::Native::Debugger::Interfaces::IDebugger* get();
    }

  private:
    // A managed class cannot have a native class or a smart pointer as a member, only a plain pointer. So the
    // native objects live in one native struct, and this pointer owns it: destructor and finalizer release
    // it with delete.
    NativeDebuggerObjects* _nativeObjects;
  };
}
