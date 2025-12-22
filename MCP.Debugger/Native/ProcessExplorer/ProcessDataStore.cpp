#include "ProcessDataStore.h"
#include <algorithm>

namespace MCP::Native::ProcessExplorer
{
  void ProcessDataStore::AddProcess(DWORD processId, HANDLE hProcess)
  {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _processHandles[processId] = hProcess;
    _processes.emplace(processId, ModuleMap{});
  }

  void ProcessDataStore::RemoveProcess(DWORD processId)
  {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _processes.erase(processId);
    _processHandles.erase(processId);
  }

  HANDLE ProcessDataStore::GetProcessHandle(DWORD processId) const
  {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    auto iter = _processHandles.find(processId);
    if (iter != _processHandles.end())
    {
      return iter->second;
    }
    return nullptr;
  }

  void ProcessDataStore::AddModule(DWORD processId, const ModuleInfoPtr& modInfo)
  {
    if (modInfo == nullptr || modInfo->GetBaseAddress() == nullptr) return;

    std::unique_lock<std::shared_mutex> lock(_mutex);
    auto procIt = _processes.find(processId);
    if (procIt != _processes.end())
    {
      procIt->second[modInfo->GetBaseAddress()] = modInfo;
    }
  }

  void ProcessDataStore::RemoveModule(DWORD processId, void* baseOfDll)
  {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    auto iter = _processes.find(processId);
    if (iter != _processes.end())
    {
      iter->second.erase(baseOfDll);
    }
  }

  bool ProcessDataStore::GetModules(DWORD processId, std::vector<ModuleInfoPtr>& modules) const
  {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    const ModuleMap* moduleMap = GetProcessMap_Unlocked(processId);
    if (moduleMap == nullptr)
    {
      return false;
    }
    for (const auto& modPair : *moduleMap)
    {
      ModuleInfoPtr modInfo = modPair.second;
      modules.push_back(modInfo);
    }
    return true;
  }

  bool ProcessDataStore::GetModule(DWORD processId, HMODULE moduleHandle, ModuleInfoPtr& outModuleInfo) const
  {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    void* baseAddress = moduleHandle;
    return FindModuleByBase_Unlocked(processId, baseAddress, outModuleInfo);
  }

  bool ProcessDataStore::GetModuleByVA(DWORD processId, DWORD_PTR va, ModuleInfoPtr& outModuleInfo) const
  {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return FindModuleByVA_Unlocked(processId, (void*)va, outModuleInfo);
  }

  bool ProcessDataStore::GetModuleByName(DWORD processId, const wchar_t* moduleName, ModuleInfoPtr& outModuleInfo) const
  {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    const ModuleMap* moduleMap = GetProcessMap_Unlocked(processId);
    if (moduleMap == nullptr)
    {
      return false;
    }

    for (const auto& modPair : *moduleMap)
    {
      const ModuleInfo& modInfo = *modPair.second;
      if (modInfo.GetModuleName().empty()) continue;

      if (_wcsicmp(modInfo.GetModuleName().c_str(), moduleName) == 0)
      {
        outModuleInfo = modPair.second; // Assign shared_ptr
        return true;
      }
    }
    return false;
  }

  const ModuleMap* ProcessDataStore::GetProcessMap_Unlocked(DWORD processId) const
  {
    auto procIt = _processes.find(processId);
    if (procIt == _processes.end())
    {
      return nullptr;
    }
    return &procIt->second;
  }

  bool ProcessDataStore::FindModuleByBase_Unlocked(DWORD processId, void* baseAddress, ModuleInfoPtr& outModuleInfo) const
  {
    const ModuleMap* moduleMap = GetProcessMap_Unlocked(processId);
    if (moduleMap == nullptr)
    {
      return false;
    }

    auto modIter = moduleMap->find(baseAddress);
    if (modIter != moduleMap->end())
    {
      outModuleInfo = modIter->second; // Fast shared_ptr copy
      return true;
    }
    return false;
  }

  bool ProcessDataStore::FindModuleByVA_Unlocked(DWORD processId, void* va, ModuleInfoPtr& outModuleInfo) const
  {
    const ModuleMap* moduleMap = GetProcessMap_Unlocked(processId);
    if (moduleMap == nullptr || moduleMap->empty())
    {
      return false;
    }
    // Find the first module with a base address greater than the VA
    auto modIter = moduleMap->upper_bound(va);
    // If the iterator is the beginning, va is before the first module
    if (modIter == moduleMap->begin())
    {
      return false;
    }
    // Decrement the iterator to get the last module with BaseAddress <= va
    modIter = std::prev(modIter);

    const ModuleInfoPtr& modInfoPtr = modIter->second;
    void* modStart = modInfoPtr->GetBaseAddress();
    void* modEnd = (BYTE*)modInfoPtr->GetBaseAddress() + modInfoPtr->GetSizeOfImage();

    if (va >= modStart && va < modEnd)
    {
      outModuleInfo = modInfoPtr; // Fast shared_ptr copy
      return true;
    }
    return false;
  }
}