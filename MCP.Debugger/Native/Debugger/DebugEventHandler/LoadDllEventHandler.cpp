#include "LoadDllEventHandler.h"
#include "DebugEventStorage.h"
#include <psapi.h>
#include <filesystem>
#include <string>

namespace MCP::Native::Debugger::DebugEventHandler
{
  LoadDllEventHandler::LoadDllEventHandler(DebugEventStorage* debugEventStorage)
  {
    _debugEventStorage = debugEventStorage;
  }

  LoadDllEventHandler::~LoadDllEventHandler()
  {
  }


  MCP::Native::Debugger::DebugEventHandler::HandleResult LoadDllEventHandler::HandleEvent(DEBUG_EVENT& debugEvent)
  {
    _debugEventStorage->AddDll(debugEvent.dwProcessId, debugEvent.u.LoadDll);
    return { DBG_CONTINUE, false };
  }
}

