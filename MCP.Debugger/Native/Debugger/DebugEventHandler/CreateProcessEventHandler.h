#pragma once
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventStorage;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class CreateProcessEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    /// <summary>The storage is borrowed and must outlive the handler.</summary>
    explicit CreateProcessEventHandler(Interfaces::IDebugEventStorage& debugEventStorage);
    ~CreateProcessEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    void CreateThreadDebugInfo(DEBUG_EVENT& debugEvent, CREATE_THREAD_DEBUG_INFO& threadInfo);

  private:
    Interfaces::IDebugEventStorage* _debugEventStorage;
  };
}