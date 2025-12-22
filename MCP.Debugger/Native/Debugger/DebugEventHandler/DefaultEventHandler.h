#pragma once
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DefaultEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    DefaultEventHandler();
    ~DefaultEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;
  };
}