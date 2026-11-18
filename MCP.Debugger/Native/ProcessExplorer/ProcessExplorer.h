#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <mutex>
#include <memory>
#include "ModuleInfo.h"
#include ".\Interfaces\IProcessExplorer.h"
#include "..\Debugger\DebugEventHandler\Interfaces\IDebugEventStorageListener.h"

namespace MCP::Native::ProcessExplorer::Interfaces
{
  class IModuleAnalyzer;
  class IProcessDataStore;
  class IProcessExplorerListener;
}

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer : public Interfaces::IProcessExplorer,
                          public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener
  {
  public:
    /// <summary>
    /// Takes ownership of the table it records processes and modules in and of the analyzer it describes
    /// newly loaded modules with.
    /// </summary>
    ProcessExplorer(std::unique_ptr<Interfaces::IProcessDataStore> store,
                    std::unique_ptr<Interfaces::IModuleAnalyzer> moduleAnalyzer);
    ~ProcessExplorer() override;

    ProcessExplorer(const ProcessExplorer&) = delete;
    ProcessExplorer& operator=(const ProcessExplorer&) = delete;
    ProcessExplorer(ProcessExplorer&&) = delete;
    ProcessExplorer& operator=(ProcessExplorer&&) = delete;

    void OnProcessCreated(std::shared_ptr<MCP::Native::Debugger::DebugEventHandler::ProcessInfo> info);
    void OnProcessExited(DWORD processId);
    void OnThreadCreated(std::shared_ptr<MCP::Native::Debugger::DebugEventHandler::ThreadInfo> info);
    void OnThreadExited(DWORD threadId);
    void OnDllLoaded(std::shared_ptr<MCP::Native::Debugger::DebugEventHandler::DllInfo> info);
    void OnDllUnloaded(DWORD processId, void* baseOfDll);

    void AddListener(Interfaces::IProcessExplorerListener* listener);
    void RemoveListener(Interfaces::IProcessExplorerListener* listener);

    bool GetModules(DWORD processId, std::vector<ModuleInfoPtr>& modules) override;
    bool GetModule(DWORD processId, HMODULE moduleHandle, ModuleInfoPtr& outModuleInfo) override;
    bool GetModuleByVA(DWORD processId, DWORD_PTR va, ModuleInfoPtr& outModuleInfo) override;
    bool GetFunctionByVA(DWORD processId, DWORD_PTR va, ModuleExport& outExport) override;

    HMODULE GetRemoteModuleHandle(DWORD processId, const wchar_t* moduleName) override;
    FARPROC GetRemoteProcAddress(DWORD processId, HMODULE hModule, LPCSTR lpProcName) override;

  private:
    void NotifyProcessAdded(DWORD processId);
    void NotifyProcessRemoved(DWORD processId);
    void NotifyModuleLoaded(DWORD processId, const ModuleInfoPtr& moduleInfo);
    void NotifyModuleUnloaded(DWORD processId, void* baseOfDll);

    std::list<Interfaces::IProcessExplorerListener*> GetListeners();

  private:
    std::unique_ptr<Interfaces::IProcessDataStore> _store;
    std::unique_ptr<Interfaces::IModuleAnalyzer> _moduleAnalyzer;
    std::mutex _listenerMutex;
    std::list<Interfaces::IProcessExplorerListener*> _listeners;
  };
}