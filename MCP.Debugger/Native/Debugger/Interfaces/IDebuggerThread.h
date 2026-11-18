#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include "..\Commands\CommandResult.h"
#include "..\Commands\DisassembleResult.h"
#include "..\Commands\ReadMemoryResult.h"
#include "..\Commands\TryResolveStringResult.h"

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// The thread that owns one debuggee: it launches the process, runs the debug loop and executes the
  /// operations handed to it.
  /// </summary>
  /// <remarks>
  /// Threading: every member is called from a thread other than the debug thread. Start blocks until the
  /// debuggee reached its first stop or could not be launched. Every operation blocks until the debug thread
  /// executed it; Stop also waits until the debug thread ended. IsRunning only reads the state.
  /// Ownership: results are returned by value. WriteMemory borrows the buffer for the duration of the call.
  /// </remarks>
  class IDebuggerThread
  {
  public:
    virtual ~IDebuggerThread() = default;

    /// <summary>Launches the executable under the debugger. False when it could not be started.</summary>
    virtual bool Start(const std::string& path) = 0;

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
    /// Process and main-thread ids and handles of the debuggee; zeroed while the thread is not running. The
    /// caller must not close the handles.
    /// </summary>
    virtual PROCESS_INFORMATION GetProcessInformation() = 0;

    /// <summary>True from a successful Start until the debuggee ended or Stop was called.</summary>
    virtual bool IsRunning() const = 0;
  };
}
