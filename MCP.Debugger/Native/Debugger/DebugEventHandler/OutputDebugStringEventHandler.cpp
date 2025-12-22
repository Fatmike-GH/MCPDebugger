#include "OutputDebugStringEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  OutputDebugStringEventHandler::OutputDebugStringEventHandler()
  {
  }

  OutputDebugStringEventHandler::~OutputDebugStringEventHandler()
  {
  }


  MCP::Native::Debugger::DebugEventHandler::HandleResult OutputDebugStringEventHandler::HandleEvent(DEBUG_EVENT& debugEvent)
  {
    // Just continue, do nothing
    return { DBG_CONTINUE, false };
  }
}

