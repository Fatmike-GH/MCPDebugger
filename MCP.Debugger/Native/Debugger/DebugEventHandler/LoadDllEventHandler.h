#pragma once
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class LoadDllEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    LoadDllEventHandler(DebugEventStorage* debugEventStorage);
    ~LoadDllEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    DebugEventStorage* _debugEventStorage;
  };
}