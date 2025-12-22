#pragma once
#include <Windows.h>
#include "..\HandleResult.h"

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventHandler
  {
  public:
    virtual ~IDebugEventHandler() = default;
    virtual MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) = 0;
  };
}