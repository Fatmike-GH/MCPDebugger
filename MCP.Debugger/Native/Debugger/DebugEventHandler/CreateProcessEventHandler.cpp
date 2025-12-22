#include "CreateProcessEventHandler.h"
#include "DebugEventStorage.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  CreateProcessEventHandler::CreateProcessEventHandler(DebugEventStorage* debugEventStorage)
  {
    _debugEventStorage = debugEventStorage;
  }

  CreateProcessEventHandler::~CreateProcessEventHandler()
  {
  }

  MCP::Native::Debugger::DebugEventHandler::HandleResult CreateProcessEventHandler::HandleEvent(DEBUG_EVENT& debugEvent)
  {
    _debugEventStorage->AddProcess(debugEvent.dwProcessId, debugEvent.u.CreateProcessInfo);

    // Add main thread; for main thread there won't be a CreateThreadEvent
    CREATE_THREAD_DEBUG_INFO threadInfo = { 0 };
    CreateThreadDebugInfo(debugEvent, threadInfo);
    _debugEventStorage->AddThread(debugEvent.dwProcessId, debugEvent.dwThreadId, threadInfo);

    return { DBG_CONTINUE, false };
  }

  void CreateProcessEventHandler::CreateThreadDebugInfo(DEBUG_EVENT& debugEvent, CREATE_THREAD_DEBUG_INFO& threadInfo)
  {
    threadInfo.hThread = debugEvent.u.CreateProcessInfo.hThread;
    threadInfo.lpStartAddress = debugEvent.u.CreateProcessInfo.lpStartAddress;
    threadInfo.lpThreadLocalBase = debugEvent.u.CreateProcessInfo.lpThreadLocalBase;
  }
}

