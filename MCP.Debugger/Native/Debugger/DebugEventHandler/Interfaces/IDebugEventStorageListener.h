#pragma once
#include <Windows.h>
#include "..\HandleResult.h"
#include <memory>

namespace MCP::Native::Debugger::DebugEventHandler
{
  struct ProcessInfo;
  struct ThreadInfo;
  struct DllInfo;
}

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventStorageListener
  {
  public:
    virtual ~IDebugEventStorageListener() = default;

    virtual void OnProcessCreated(std::shared_ptr<ProcessInfo> info) = 0;
    virtual void OnProcessExited(DWORD processId) = 0;
    virtual void OnThreadCreated(std::shared_ptr<ThreadInfo> info) = 0;
    virtual void OnThreadExited(DWORD threadId) = 0;
    virtual void OnDllLoaded(std::shared_ptr<DllInfo> info) = 0;
    virtual void OnDllUnloaded(DWORD processId, void* baseOfDll) = 0;
  };
}