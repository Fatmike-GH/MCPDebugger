#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <mutex>
#include <memory>
#include "PEImage.h"
#include "ProcessDataStore.h" 
#include "ModuleInfo.h"
#include "..\Debugger\DebugEventHandler\Interfaces\IDebugEventStorageListener.h"

namespace MCP::Native::ProcessExplorer::Interfaces
{
  class IProcessExplorerListener;
}

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener
  {
  public:
    void OnProcessCreated(std::shared_ptr<MCP::Native::Debugger::DebugEventHandler::ProcessInfo> info);
    void OnProcessExited(DWORD processId);
    void OnThreadCreated(std::shared_ptr<MCP::Native::Debugger::DebugEventHandler::ThreadInfo> info);
    void OnThreadExited(DWORD threadId);
    void OnDllLoaded(std::shared_ptr<MCP::Native::Debugger::DebugEventHandler::DllInfo> info);
    void OnDllUnloaded(DWORD processId, void* baseOfDll);

    void AddListener(Interfaces::IProcessExplorerListener* listener);
    void RemoveListener(Interfaces::IProcessExplorerListener* listener);

    bool GetModules(DWORD processId, std::vector<ModuleInfoPtr>& modules);
    bool GetModule(DWORD processId, HMODULE moduleHandle, ModuleInfoPtr& outModuleInfo);
    bool GetModuleByVA(DWORD processId, DWORD_PTR va, ModuleInfoPtr& outModuleInfo);
    bool GetFunctionByVA(DWORD processId, DWORD_PTR va, ModuleExport& outExport);

    HMODULE GetRemoteModuleHandle(DWORD processId, const wchar_t* moduleName);
    FARPROC GetRemoteProcAddress(DWORD processId, HMODULE hModule, LPCSTR lpProcName);

  private:
    void NotifyProcessAdded(DWORD processId);
    void NotifyProcessRemoved(DWORD processId);
    void NotifyModuleLoaded(DWORD processId, const ModuleInfoPtr& moduleInfo);
    void NotifyModuleUnloaded(DWORD processId, void* baseOfDll);

    std::list<Interfaces::IProcessExplorerListener*> GetListeners();

  private:
    ProcessDataStore _store;
    std::mutex _listenerMutex;
    std::list<Interfaces::IProcessExplorerListener*> _listeners;
  };
}