#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <shared_mutex>
#include "ModuleInfo.h"
#include ".\Interfaces\IProcessDataStore.h"

namespace MCP::Native::ProcessExplorer
{
  class ProcessDataStore : public Interfaces::IProcessDataStore
  {
  public:
    ProcessDataStore() = default;
    ~ProcessDataStore() override = default;

    void AddProcess(DWORD processId, HANDLE hProcess) override;
    void RemoveProcess(DWORD processId) override;
    HANDLE GetProcessHandle(DWORD processId) const override;

    void AddModule(DWORD processId, const ModuleInfoPtr& modInfo) override;
    void RemoveModule(DWORD processId, void* baseOfDll) override;

    bool GetModules(DWORD processId, std::vector<ModuleInfoPtr>& modules) const override;
    bool GetModule(DWORD processId, HMODULE moduleHandle, ModuleInfoPtr& outModuleInfo) const override;
    bool GetModuleByVA(DWORD processId, DWORD_PTR va, ModuleInfoPtr& outModuleInfo) const override;
    bool GetModuleByName(DWORD processId, const wchar_t* moduleName, ModuleInfoPtr& outModuleInfo) const override;

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