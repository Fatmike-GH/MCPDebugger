#pragma once
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class CreateProcessEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    CreateProcessEventHandler(DebugEventStorage* debugEventStorage);
    ~CreateProcessEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    void CreateThreadDebugInfo(DEBUG_EVENT& debugEvent, CREATE_THREAD_DEBUG_INFO& threadInfo);

  private:
    DebugEventStorage* _debugEventStorage;
  };
}