#pragma once
#include <Windows.h>
#include <memory>
#include "StopOutcome.h"
#include "..\Commands\CommandResult.h"
#include "..\Commands\DisassembleResult.h"
#include "..\Commands\ReadMemoryResult.h"
#include "..\Commands\TryResolveStringResult.h"

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// Hands operations on the debuggee from client threads to the debug thread, which is the only thread
  /// allowed to touch the debuggee.
  /// </summary>
  /// <remarks>
  /// Threading: the operations (Stop to TryResolveString) are called from any thread except the debug thread;
  /// each blocks until the debug thread executed it and returns its result by value. SetCurrentContext,
  /// ProcessCommandsSync and ProcessCommandsAsync are called on the debug thread only.
  /// Ownership: WriteMemory borrows the buffer for the duration of the call.
  /// </remarks>
  class IDebuggerCommandQueue
  {
  public:
    virtual ~IDebuggerCommandQueue() = default;

    /// <summary>Names the process and thread of the debug event the following operations act on.</summary>
    virtual void SetCurrentContext(DWORD processId, DWORD threadId) = 0;

    virtual Commands::CommandResult Stop() = 0;
    virtual Commands::CommandResult Continue() = 0;
    virtual Commands::CommandResult StepInto() = 0;
    virtual Commands::CommandResult StepOver() = 0;
    virtual Commands::CommandResult SetBreakpoint(DWORD_PTR va) = 0;
    virtual Commands::CommandResult RemoveBreakpoint(DWORD_PTR va) = 0;
    virtual Commands::ReadMemoryResult ReadMemory(DWORD_PTR va, size_t size) = 0;
    virtual Commands::CommandResult WriteMemory(DWORD_PTR va, const BYTE* data, size_t size) = 0;
    virtual Commands::DisassembleResult Disassemble(DWORD_PTR va, size_t length) = 0;
    virtual Commands::TryResolveStringResult TryResolveString(DWORD_PTR va) = 0;

    /// <summary>
    /// Executes operations while the debuggee is stopped and waits for further ones, until one of them resumes
    /// the debuggee or ends the session. Returns that outcome: the plan the resuming operation left, or the
    /// end of the session. Operations queued behind a resuming one stay queued for the next processing.
    /// </summary>
    virtual StopOutcome ProcessCommandsSync() = 0;

    /// <summary>Executes the operations queued so far and returns without waiting.</summary>
    virtual void ProcessCommandsAsync() = 0;
  };
}
