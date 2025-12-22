#include "ProcessExplorer.h"
#include "ModuleAnalyzer.h"
#include "..\Debugger\DebugEventHandler\ProcessInfo.h"
#include "..\Debugger\DebugEventHandler\ThreadInfo.h"
#include "..\Debugger\DebugEventHandler\DllInfo.h"
#include ".\Interfaces\IProcessExplorerListener.h"

#include <Psapi.h>
#include <vector>
#include <string>

namespace MCP::Native::ProcessExplorer
{
  void ProcessExplorer::OnProcessCreated(std::shared_ptr<MCP::Native::Debugger::DebugEventHandler::ProcessInfo> info)
  {
    _store.AddProcess(info->ProcessId, info->info.hProcess);

    ModuleInfoPtr moduleInfo = ModuleAnalyzer::AnalyzeModule(info->info.hProcess, info->info.hFile, info->info.lpBaseOfImage);
    _store.AddModule(info->ProcessId, moduleInfo);

    NotifyProcessAdded(info->ProcessId);
    NotifyModuleLoaded(info->ProcessId, moduleInfo);
  }

  void ProcessExplorer::OnProcessExited(DWORD processId)
  {
    _store.RemoveProcess(processId);
    NotifyProcessRemoved(processId);
  }

  void ProcessExplorer::OnThreadCreated(std::shared_ptr<MCP::Native::Debugger::DebugEventHandler::ThreadInfo> info) {}
  void ProcessExplorer::OnThreadExited(DWORD threadId) {}

  void ProcessExplorer::OnDllLoaded(std::shared_ptr<MCP::Native::Debugger::DebugEventHandler::DllInfo> info)
  {
    HANDLE hProcess = _store.GetProcessHandle(info->ProcessId);
    if (hProcess == nullptr) return;

    ModuleInfoPtr moduleInfo = ModuleAnalyzer::AnalyzeModule(hProcess, info->Info.hFile, info->Info.lpBaseOfDll);
    _store.AddModule(info->ProcessId, moduleInfo);

    NotifyModuleLoaded(info->ProcessId, moduleInfo);
  }

  void ProcessExplorer::OnDllUnloaded(DWORD processId, void* baseOfDll)
  {
    _store.RemoveModule(processId, baseOfDll);
    NotifyModuleUnloaded(processId, baseOfDll);
  }

  void ProcessExplorer::AddListener(MCP::Native::ProcessExplorer::Interfaces::IProcessExplorerListener* listener)
  {
    if (listener)
    {
      std::lock_guard<std::mutex> lock(_listenerMutex);
      _listeners.push_back(listener);
    }
  }

  void ProcessExplorer::RemoveListener(MCP::Native::ProcessExplorer::Interfaces::IProcessExplorerListener* listener)
  {
    std::lock_guard<std::mutex> lock(_listenerMutex);
    _listeners.remove(listener);
  }

  bool ProcessExplorer::GetModules(DWORD processId, std::vector<ModuleInfoPtr>& modules)
  {
    return _store.GetModules(processId, modules);
  }

  bool ProcessExplorer::GetModule(DWORD processId, HMODULE moduleHandle, ModuleInfoPtr& outModuleInfo)
  {
    return _store.GetModule(processId, moduleHandle, outModuleInfo);
  }

  bool ProcessExplorer::GetModuleByVA(DWORD processId, DWORD_PTR va, ModuleInfoPtr& outModuleInfo)
  {
    return _store.GetModuleByVA(processId, va, outModuleInfo);
  }

  bool ProcessExplorer::GetFunctionByVA(DWORD processId, DWORD_PTR va, ModuleExport& outExport)
  {
    ModuleInfoPtr modInfo;
    if (!_store.GetModuleByVA(processId, va, modInfo))
    {
      return false;
    }
    return modInfo->FindExportByAddress(reinterpret_cast<void*>(va), outExport);
  }

  HMODULE ProcessExplorer::GetRemoteModuleHandle(DWORD processId, const wchar_t* moduleName)
  {
    ModuleInfoPtr modInfo;
    if (_store.GetModuleByName(processId, moduleName, modInfo))
    {
      return (HMODULE)modInfo->GetBaseAddress();
    }

    return nullptr;
  }

  FARPROC ProcessExplorer::GetRemoteProcAddress(DWORD processId, HMODULE hModule, LPCSTR lpProcName)
  {
    ModuleInfoPtr modInfo;
    if (!_store.GetModule(processId, hModule, modInfo))
    {
      return nullptr;
    }

    if (IS_INTRESOURCE(lpProcName))
    {
      return modInfo->FindExportByOrdinal((DWORD)(DWORD_PTR)lpProcName);
    }
    else
    {
      return modInfo->FindExportByName(lpProcName);
    }
  }

  void ProcessExplorer::NotifyProcessAdded(DWORD processId)
  {
    auto listeners = GetListeners();
    for (auto* listener : listeners)
    {
      listener->OnProcessAdded(processId);
    }
  }

  void ProcessExplorer::NotifyProcessRemoved(DWORD processId)
  {
    auto listeners = GetListeners();
    for (auto* listener : listeners)
    {
      listener->OnProcessRemoved(processId);
    }
  }

  void ProcessExplorer::NotifyModuleLoaded(DWORD processId, const ModuleInfoPtr& moduleInfo)
  {
    if (moduleInfo == nullptr || moduleInfo->GetBaseAddress() == nullptr) return;
    auto listeners = GetListeners();
    for (auto* listener : listeners)
    {
      // Dereference the shared_ptr to pass a const ModuleInfo&
      listener->OnModuleLoaded(processId, *moduleInfo);
    }
  }

  void ProcessExplorer::NotifyModuleUnloaded(DWORD processId, void* baseOfDll)
  {
    auto listeners = GetListeners();
    for (auto* listener : listeners)
    {
      listener->OnModuleUnloaded(processId, baseOfDll);
    }
  }

  std::list<Interfaces::IProcessExplorerListener*> ProcessExplorer::GetListeners()
  {
    std::lock_guard<std::mutex> lock(_listenerMutex);
    return _listeners; // Make a copy for thread safety
  }
}