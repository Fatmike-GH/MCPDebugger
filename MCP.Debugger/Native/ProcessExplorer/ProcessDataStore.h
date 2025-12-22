#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <shared_mutex>
#include "ModuleInfo.h"

namespace MCP::Native::ProcessExplorer
{
  class ProcessDataStore
  {
  public:
    ProcessDataStore() = default;
    ~ProcessDataStore() = default;

    void AddProcess(DWORD processId, HANDLE hProcess);
    void RemoveProcess(DWORD processId);
    HANDLE GetProcessHandle(DWORD processId) const;

    void AddModule(DWORD processId, const ModuleInfoPtr& modInfo);
    void RemoveModule(DWORD processId, void* baseOfDll);

    bool GetModules(DWORD processId, std::vector<ModuleInfoPtr>& modules) const;
    bool GetModule(DWORD processId, HMODULE moduleHandle, ModuleInfoPtr& outModuleInfo) const;
    bool GetModuleByVA(DWORD processId, DWORD_PTR va, ModuleInfoPtr& outModuleInfo) const;
    bool GetModuleByName(DWORD processId, const wchar_t* moduleName, ModuleInfoPtr& outModuleInfo) const;

  private:
    // _Unlocked methods require a lock on _mutex before calling
    const ModuleMap* GetProcessMap_Unlocked(DWORD processId) const;
    bool FindModuleByBase_Unlocked(DWORD processId, void* baseAddress, ModuleInfoPtr& outModuleInfo) const;
    bool FindModuleByVA_Unlocked(DWORD processId, void* va, ModuleInfoPtr& outModuleInfo) const;
    

  private:
    mutable std::shared_mutex _mutex; // Protects both maps
    std::map<DWORD, ModuleMap> _processes;
    std::map<DWORD, HANDLE> _processHandles;
  };
}