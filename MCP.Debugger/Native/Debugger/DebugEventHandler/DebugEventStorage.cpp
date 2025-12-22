#include "DebugEventStorage.h"
#include ".\Interfaces\IDebugEventStorageListener.h"
#include "ProcessInfo.h"
#include "ThreadInfo.h"
#include "DllInfo.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  void DebugEventStorage::AddDebugEvent(const DEBUG_EVENT& debugEvent)
  {
    std::lock_guard<std::mutex> lock(_eventsMutex);
    _events.push_back(debugEvent);
  }

  void DebugEventStorage::RetrieveDebugEvents(std::list<DEBUG_EVENT>& outEvents)
  {
    std::lock_guard<std::mutex> lock(_eventsMutex);

    if (_events.empty())
      return;

    outEvents.splice(outEvents.end(), _events); // Move all _events to the end of outEvents
  }

  void DebugEventStorage::AddProcess(DWORD processId, const CREATE_PROCESS_DEBUG_INFO& info)
  {
    auto pi = std::make_shared<ProcessInfo>();
    pi->ProcessId = processId;
    pi->info = info;
    {
      std::lock_guard<std::mutex> lock(_processesMutex);
      _processes[processId] = pi;
    } // Release lock

    auto listeners = GetListeners();
    for (auto* listener : listeners)
    {
      listener->OnProcessCreated(pi);
    }
  }

  void DebugEventStorage::RemoveProcess(DWORD processId)
  {
    {
      std::lock_guard<std::mutex> lock(_processesMutex);
      _processes.erase(processId);
    } // Release lock

    auto listeners = GetListeners();
    for (auto* listener : listeners)
    {
      listener->OnProcessExited(processId);
    }
  }

  void DebugEventStorage::AddThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO& info)
  {
    auto ti = std::make_shared<ThreadInfo>();
    ti->ProcessId = processId;
    ti->ThreadId = threadId;
    ti->info = info;
    {
      std::lock_guard<std::mutex> lock(_threadsMutex);
      _threads[threadId] = ti;
    }  // Release lock

    auto listeners = GetListeners();
    for (auto* listener : listeners)
    {
      listener->OnThreadCreated(ti);
    }
  }

  void DebugEventStorage::RemoveThread(DWORD threadId)
  {
    {
      std::lock_guard<std::mutex> lock(_threadsMutex);
      _threads.erase(threadId);
    } // Release lock

    auto listeners = GetListeners();
    for (auto* listener : listeners)
    {
      listener->OnThreadExited(threadId);
    }
  }

  void DebugEventStorage::AddDll(DWORD processId, const LOAD_DLL_DEBUG_INFO& info)
  {
    auto di = std::make_shared<DllInfo>();
    di->ProcessId = processId;
    di->Info = info;
    {
      std::lock_guard<std::mutex> lock(_dllsMutex);
      _dlls.push_back(di);
    }  // Release lock

    // Make a copy of listeners
    std::list<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener*> listeners;
    {
      std::lock_guard<std::mutex> lock(_listenerMutex);
      listeners = _listeners; // Make a copy
    } // Release lock

    for (auto* listener : listeners)
    {
      listener->OnDllLoaded(di);
    }
  }

  void DebugEventStorage::RemoveDll(DWORD processId, void* baseOfDll)
  {
    {
      std::lock_guard<std::mutex> lock(_dllsMutex);
      _dlls.remove_if([processId, baseOfDll](const std::shared_ptr<DllInfo>& dllInfoPtr)
      {
        if (dllInfoPtr)
        {
          return (dllInfoPtr->ProcessId == processId) &&
            (dllInfoPtr->Info.lpBaseOfDll == baseOfDll);
        }
        return false;
      });
    } // Release lock

    auto listeners = GetListeners();
    for (auto* listener : listeners)
    {
      listener->OnDllUnloaded(processId, baseOfDll);
    }
  }

  void DebugEventStorage::GetProcesses(std::map<DWORD, std::shared_ptr<ProcessInfo>>& outMap)
  {
    std::lock_guard<std::mutex> lock(_processesMutex);
    outMap = _processes;
  }

  void DebugEventStorage::GetThreads(std::map<DWORD, std::shared_ptr<ThreadInfo>>& outMap)
  {
    std::lock_guard<std::mutex> lock(_threadsMutex);
    outMap = _threads;
  }

  void DebugEventStorage::GetDlls(std::list<std::shared_ptr<DllInfo>>& outList)
  {
    std::lock_guard<std::mutex> lock(_dllsMutex);
    outList = _dlls;
  }

  bool DebugEventStorage::GetProcessById(DWORD processId, ProcessInfo& outInfo)
  {
    std::lock_guard<std::mutex> lock(_processesMutex);
    auto it = _processes.find(processId);
    if (it != _processes.end())
    {
      outInfo = *(it->second);
      return true;
    }
    return false;
  }

  bool DebugEventStorage::GetThreadById(DWORD threadId, ThreadInfo& outInfo)
  {
    std::lock_guard<std::mutex> lock(_threadsMutex);
    auto it = _threads.find(threadId);
    if (it != _threads.end())
    {
      outInfo = *(it->second);
      return true;
    }
    return false;
  }

  void DebugEventStorage::GetThreadsByProcessId(DWORD processId, std::list<ThreadInfo>& outThreads)
  {
    std::lock_guard<std::mutex> lock(_threadsMutex);
    outThreads.clear();

    for (const auto& pair : _threads)
    {
      if (pair.second->ProcessId == processId)
      {
        outThreads.push_back(*(pair.second));
      }
    }
  }

  void DebugEventStorage::GetDllsByProcessId(DWORD processId, std::list<DllInfo>& outDlls)
  {
    std::lock_guard<std::mutex> lock(_dllsMutex);
    outDlls.clear();

    for (const auto& dllInfo : _dlls)
    {
      if (dllInfo->ProcessId == processId)
      {
        outDlls.push_back(*dllInfo);
      }
    }
  }

  void DebugEventStorage::AddListener(MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener* listener)
  {
    if (listener)
    {
      std::lock_guard<std::mutex> lock(_listenerMutex);
      _listeners.push_back(listener);
    }
  }

  void DebugEventStorage::RemoveListener(MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener* listener)
  {
    std::lock_guard<std::mutex> lock(_listenerMutex);
    _listeners.remove(listener);
  }

  std::list<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorageListener*> DebugEventStorage::GetListeners()
  {
    std::lock_guard<std::mutex> lock(_listenerMutex);
    return _listeners; // Make a copy for thread safety
  }
}