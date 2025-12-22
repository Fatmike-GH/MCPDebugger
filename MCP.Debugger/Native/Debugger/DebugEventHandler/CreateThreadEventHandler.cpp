#include "CreateThreadEventHandler.h"
#include "DebugEventStorage.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  CreateThreadEventHandler::CreateThreadEventHandler(DebugEventStorage* debugEventStorage)
  {
    _debugEventStorage = debugEventStorage;
  }

  CreateThreadEventHandler::~CreateThreadEventHandler()
  {
  }


  MCP::Native::Debugger::DebugEventHandler::HandleResult CreateThreadEventHandler::HandleEvent(DEBUG_EVENT& debugEvent)
  {
    _debugEventStorage->AddThread(debugEvent.dwProcessId, debugEvent.dwThreadId, debugEvent.u.CreateThread);
    return { DBG_CONTINUE, false };
  }
}

