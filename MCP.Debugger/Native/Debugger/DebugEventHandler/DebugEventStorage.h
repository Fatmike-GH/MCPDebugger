#pragma once
#include <windows.h>
#include <mutex>
#include <list>
#include <map>
#include ".\Interfaces\IDebugEventStorage.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  struct ProcessInfo;
  struct ThreadInfo;
  struct DllInfo;
}

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventStorageListener;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage : public Interfaces::IDebugEventStorage
  {
  public:
    DebugEventStorage() = default;
    ~DebugEventStorage() override = default;

    DebugEventStorage(const DebugEventStorage&) = delete;
    DebugEventStorage& operator=(const DebugEventStorage&) = delete;
    DebugEventStorage(DebugEventStorage&&) = delete;
    DebugEventStorage& operator=(DebugEventStorage&&) = delete;

    void AddDebugEvent(const DEBUG_EVENT& debugEvent);
    void RetrieveDebugEvents(std::list<DEBUG_EVENT>& outEvents);

    void AddProcess(DWORD processId, const CREATE_PROCESS_DEBUG_INFO& info) override;
    void RemoveProcess(DWORD processId) override;

    void AddThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO& info) override;
    void RemoveThread(DWORD threadId) override;

    void AddDll(DWORD processId, const LOAD_DLL_DEBUG_INFO& info) override;
    void RemoveDll(DWORD processId, void* baseOfDll) override;

    void GetProcesses(std::map<DWORD, std::shared_ptr<ProcessInfo>>& outMap);
    void GetThreads(std::map<DWORD, std::shared_ptr<ThreadInfo>>& outMap);
    void GetDlls(std::list<std::shared_ptr<DllInfo>>& outList);
    bool GetProcessById(DWORD processId, ProcessInfo& outInfo) override;
    bool GetThreadById(DWORD threadId, ThreadInfo& outInfo) override;
    void GetThreadsByProcessId(DWORD processId, std::list<ThreadInfo>& outThreads);
    void GetDllsByProcessId(DWORD processId, std::list<DllInfo>& outDlls);

    void AddListener(MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener* listener);
    void RemoveListener(MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener* listener);

  private:
    std::list<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener*> GetListeners();

  private:
    std::mutex _eventsMutex;
    std::mutex _processesMutex;
    std::mutex _threadsMutex;
    std::mutex _dllsMutex;
    std::mutex _listenerMutex;

    std::list<DEBUG_EVENT> _events;
    std::map<DWORD, std::shared_ptr<ProcessInfo>> _processes;
    std::map<DWORD, std::shared_ptr<ThreadInfo>> _threads;
    std::list<std::shared_ptr<DllInfo>> _dlls;

    std::list<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener*> _listeners;
  };
}