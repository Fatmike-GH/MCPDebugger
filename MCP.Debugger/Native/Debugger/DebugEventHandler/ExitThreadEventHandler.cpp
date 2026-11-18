#include "ExitThreadEventHandler.h"
#include ".\Interfaces\IDebugEventStorage.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  ExitThreadEventHandler::ExitThreadEventHandler(Interfaces::IDebugEventStorage& debugEventStorage)
  {
    _debugEventStorage = &debugEventStorage;
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

