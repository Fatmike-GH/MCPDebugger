#pragma once
#include <memory>
#include "IDebuggerSession.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;

  /// <summary>
  /// Creates the objects of one debugging session. The debugger asks for a new session on every start, so no
  /// state of an earlier session survives.
  /// </summary>
  /// <remarks>
  /// Ownership: the caller owns the returned session. The event handler is borrowed and must outlive the
  /// session: the session's debug thread reports its events to it.
  /// Threading: called on the thread that starts the debugger.
  /// </remarks>
  class IDebuggerSessionFactory
  {
  public:
    virtual ~IDebuggerSessionFactory() = default;

    /// <summary>Returns a new, not yet started session; never null.</summary>
    virtual std::unique_ptr<IDebuggerSession> CreateSession(IDebuggerEventHandler& eventHandler) = 0;
  };
}
