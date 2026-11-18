#pragma once
#include <Windows.h>
#include <vector>
#include "..\ModuleInfo.h"

namespace MCP::Native::ProcessExplorer::Interfaces
{
  /// <summary>
  /// The table of the debuggee processes and the modules loaded into each of them.
  /// </summary>
  /// <remarks>
  /// Threading: thread-safe - the debug thread adds and removes entries while other threads look them up.
  /// Ownership: process handles are kept as given and are neither owned nor closed; module information is
  /// shared with the callers and stays valid after its entry was removed.
  /// Failures: a lookup returns false or null when the process or the module is not known.
  /// </remarks>
  class IProcessDataStore
  {
  public:
    virtual ~IProcessDataStore() = default;

    virtual void AddProcess(DWORD processId, HANDLE hProcess) = 0;

    /// <summary>Forgets the process together with its modules.</summary>
    virtual void RemoveProcess(DWORD processId) = 0;

    virtual HANDLE GetProcessHandle(DWORD processId) const = 0;

    /// <summary>
    /// Records a module of a known process. A module without information or without base address and a
    /// module of an unknown process are ignored.
    /// </summary>
    virtual void AddModule(DWORD processId, const ModuleInfoPtr& modInfo) = 0;
    virtual void RemoveModule(DWORD processId, void* baseOfDll) = 0;

    /// <summary>Appends the modules of the process to the list.</summary>
    virtual bool GetModules(DWORD processId, std::vector<ModuleInfoPtr>& modules) const = 0;
    virtual bool GetModule(DWORD processId, HMODULE moduleHandle, ModuleInfoPtr& outModuleInfo) const = 0;

    /// <summary>Finds the module whose image contains the address.</summary>
    virtual bool GetModuleByVA(DWORD processId, DWORD_PTR va, ModuleInfoPtr& outModuleInfo) const = 0;

    /// <summary>Finds a module by its file name.</summary>
    virtual bool GetModuleByName(DWORD processId, const wchar_t* moduleName, ModuleInfoPtr& outModuleInfo) const = 0;
  };
}
