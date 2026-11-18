#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::DebugEventHandler
{
  struct ProcessInfo;
  struct ThreadInfo;
}

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  /// <summary>
  /// The bookkeeping of what the debug events of a session reported: the processes, threads and DLLs the
  /// debuggee has right now.
  /// </summary>
  /// <remarks>
  /// Threading: the members that add or remove an entry are called on the debug thread only; the lookups may
  /// be called from any thread.
  /// Ownership: a lookup copies the entry into the caller's object. The handles inside an entry are kept as
  /// the debug event delivered them; the storage neither owns nor closes them.
  /// </remarks>
  class IDebugEventStorage
  {
  public:
    virtual ~IDebugEventStorage() = default;

    virtual void AddProcess(DWORD processId, const CREATE_PROCESS_DEBUG_INFO& info) = 0;
    virtual void RemoveProcess(DWORD processId) = 0;

    virtual void AddThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO& info) = 0;
    virtual void RemoveThread(DWORD threadId) = 0;

    virtual void AddDll(DWORD processId, const LOAD_DLL_DEBUG_INFO& info) = 0;
    virtual void RemoveDll(DWORD processId, void* baseOfDll) = 0;

    /// <summary>Copies the entry of a process. False when the process is not known; outInfo is then untouched.</summary>
    virtual bool GetProcessById(DWORD processId, ProcessInfo& outInfo) = 0;

    /// <summary>Copies the entry of a thread. False when the thread is not known; outInfo is then untouched.</summary>
    virtual bool GetThreadById(DWORD threadId, ThreadInfo& outInfo) = 0;
  };
}
