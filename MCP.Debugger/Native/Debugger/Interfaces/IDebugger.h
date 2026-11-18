#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include "..\Commands\CommandResult.h"
#include "..\Commands\DisassembleResult.h"
#include "..\Commands\ReadMemoryResult.h"
#include "..\Commands\TryResolveStringResult.h"

namespace MCP::Native::ProcessExplorer::Interfaces
{
  class IProcessExplorer;
}

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// The debugger as its clients use it: debugs one executable at a time. A session lasts from a successful
  /// Start until Stop or the end of the debuggee; every Start begins a new session.
  /// </summary>
  /// <remarks>
  /// Threading: called from client threads, never from the debug thread (an operation waits for the debug
  /// thread). Start blocks until the debuggee reached its first stop or could not be started.
  /// Failures: nothing throws. An operation called while no session is running returns a failed result or
  /// zero and does nothing.
  /// Ownership: results are returned by value. WriteMemory borrows the buffer for the duration of the call.
  /// </remarks>
  class IDebugger
  {
  public:
    virtual ~IDebugger() = default;

    virtual bool IsRunning() const = 0;

    /// <summary>
    /// Starts a session. False with the reason in error when a session is already running, the file is not a
    /// native executable of this build's bitness or the process could not be started.
    /// </summary>
    virtual bool Start(const std::string& path, std::string& error) = 0;

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

    /// <summary>Process and main-thread ids and handles of the debuggee; zeroed when no session is running.</summary>
    virtual PROCESS_INFORMATION GetProcessInformation() const = 0;

    /// <summary>Entry point of a debuggee process; 0 when no session is running or the process is unknown.</summary>
    virtual DWORD_PTR GetEntryPoint(DWORD processId) const = 0;

    /// <summary>Image base of a debuggee process; 0 when no session is running or the process is unknown.</summary>
    virtual DWORD_PTR GetImageBase(DWORD processId) const = 0;

    /// <summary>
    /// The module bookkeeping of the current or the last session. Borrowed: null before the first Start, valid
    /// until the next Start or the destruction of the debugger. Its queries may be called from any thread and
    /// stay answerable after the session ended.
    /// </summary>
    virtual MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer* GetProcessExplorer() const = 0;
  };
}
