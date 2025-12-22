#include "DefaultEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  DefaultEventHandler::DefaultEventHandler()
  {
  }

  DefaultEventHandler::~DefaultEventHandler()
  {
  }


  MCP::Native::Debugger::DebugEventHandler::HandleResult DefaultEventHandler::HandleEvent(DEBUG_EVENT& debugEvent)
  {
    // Just continue, do nothing
    return { DBG_CONTINUE, false };
  }
}

