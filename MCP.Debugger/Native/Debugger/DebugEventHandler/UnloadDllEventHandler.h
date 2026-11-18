#pragma once
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventStorage;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class UnloadDllEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    /// <summary>The storage is borrowed and must outlive the handler.</summary>
    explicit UnloadDllEventHandler(Interfaces::IDebugEventStorage& debugEventStorage);
    ~UnloadDllEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    Interfaces::IDebugEventStorage* _debugEventStorage;
  };
}