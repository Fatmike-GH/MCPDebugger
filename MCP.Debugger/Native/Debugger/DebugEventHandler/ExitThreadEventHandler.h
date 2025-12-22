#pragma once
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class ExitThreadEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    ExitThreadEventHandler(DebugEventStorage* debugEventStorage);
    ~ExitThreadEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    DebugEventStorage* _debugEventStorage;
  };
}