#pragma once
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventStorage;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class CreateThreadEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    /// <summary>The storage is borrowed and must outlive the handler.</summary>
    explicit CreateThreadEventHandler(Interfaces::IDebugEventStorage& debugEventStorage);
    ~CreateThreadEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    Interfaces::IDebugEventStorage* _debugEventStorage;
  };
}