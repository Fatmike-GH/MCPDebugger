#include "UnloadDllEventHandler.h"
#include ".\Interfaces\IDebugEventStorage.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  UnloadDllEventHandler::UnloadDllEventHandler(Interfaces::IDebugEventStorage& debugEventStorage)
  {
    _debugEventStorage = &debugEventStorage;
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

