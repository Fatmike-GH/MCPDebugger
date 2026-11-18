#pragma once
#include <Windows.h>
#include "..\DebugEventHandler\HandleResult.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerCommandQueue;

  /// <summary>
  /// Everything the debug thread uses to serve the debuggee it launched: the queue of operations handed over
  /// by client threads and the handling of the debuggee's debug events.
  /// </summary>
  /// <remarks>
  /// Ownership: the object owns its parts; the queue it hands out is borrowed and valid as long as the object.
  /// Threading: created on the debug thread and destroyed after the debug thread ended. Each member states
  /// which threads may call it.
  /// </remarks>
  class IDebugThreadServices
  {
  public:
    virtual ~IDebugThreadServices() = default;

    /// <summary>
    /// The queue of the launched debuggee, always the same object. May be called from any thread once the
    /// services were handed out; the queue states the threading rules of its own members.
    /// </summary>
    virtual IDebuggerCommandQueue& GetCommandQueue() = 0;

    /// <summary>
    /// Handles one debug event of the debuggee and tells how to continue it and whether the debuggee exited.
    /// Blocks while the debuggee is stopped at a breakpoint or step, until an operation resumes it.
    /// Debug thread only.
    /// </summary>
    virtual MCP::Native::Debugger::DebugEventHandler::HandleResult HandleDebugEvent(DEBUG_EVENT& debugEvent) = 0;
  };
}
