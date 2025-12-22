#include "ExitThreadEventHandler.h"
#include "DebugEventStorage.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  ExitThreadEventHandler::ExitThreadEventHandler(DebugEventStorage* debugEventStorage)
  {
    _debugEventStorage = debugEventStorage;
  }

  ExitThreadEventHandler::~ExitThreadEventHandler()
  {
  }


  MCP::Native::Debugger::DebugEventHandler::HandleResult ExitThreadEventHandler::HandleEvent(DEBUG_EVENT& debugEvent)
  {
    _debugEventStorage->RemoveThread(debugEvent.dwThreadId);
    return { DBG_CONTINUE, false };
  }
}

