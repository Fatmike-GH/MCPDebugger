#include "UnloadDllEventHandler.h"
#include "DebugEventStorage.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  UnloadDllEventHandler::UnloadDllEventHandler(DebugEventStorage* debugEventStorage)
  {
    _debugEventStorage = debugEventStorage;
  }

  UnloadDllEventHandler::~UnloadDllEventHandler()
  {
  }


  MCP::Native::Debugger::DebugEventHandler::HandleResult UnloadDllEventHandler::HandleEvent(DEBUG_EVENT& debugEvent)
  {
    _debugEventStorage->RemoveDll(debugEvent.dwProcessId, debugEvent.u.UnloadDll.lpBaseOfDll);
    return { DBG_CONTINUE, false };
  }
}

