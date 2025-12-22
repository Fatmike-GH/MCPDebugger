#pragma once
#include "..\ProcessExplorer.h"

namespace MCP::Native::ProcessExplorer
{
  class ModuleInfo;
  struct ModuleExport;
}

namespace MCP::Native::ProcessExplorer::Interfaces
{
  class IProcessExplorerListener
  {
  public:
    virtual ~IProcessExplorerListener() = default;

    virtual void OnProcessAdded(DWORD processId) = 0;
    virtual void OnProcessRemoved(DWORD processId) = 0;
    virtual void OnModuleLoaded(DWORD processId, const ModuleInfo& info) = 0;
    virtual void OnModuleUnloaded(DWORD processId, void* baseAddress) = 0;
  };
}