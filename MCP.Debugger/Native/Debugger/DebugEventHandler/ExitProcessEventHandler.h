#pragma once
#include ".\Interfaces\IDebugEventHandler.h"
#include <memory>

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventStorage;
}

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;
  struct BreakpointEventArgs;
  struct ExitProcessEventArgs;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class ExitProcessEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    /// <summary>The storage and the client's event handler are borrowed and must outlive the handler.</summary>
    ExitProcessEventHandler(Interfaces::IDebugEventStorage& debugEventStorage, MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler);
    ~ExitProcessEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    void NotifyEventHandler(DEBUG_EVENT& debugEvent);
    std::shared_ptr<MCP::Native::Debugger::Interfaces::ExitProcessEventArgs> CreateExitProcessEventArgs(DEBUG_EVENT& debugEvent);

  private:
    Interfaces::IDebugEventStorage* _debugEventStorage;
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* _eventHandler;
  };
}
