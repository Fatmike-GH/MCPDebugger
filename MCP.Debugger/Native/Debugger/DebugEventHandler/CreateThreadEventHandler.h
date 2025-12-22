#pragma once
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class CreateThreadEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    CreateThreadEventHandler(DebugEventStorage* debugEventStorage);
    ~CreateThreadEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    DebugEventStorage* _debugEventStorage;
  };
}