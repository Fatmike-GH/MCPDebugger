#pragma once
#include <Windows.h>
#include <map>
#include <memory>
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger
{
  class BreakpointManager;
  class DebuggerCommandQueue;
}

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
}

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventHandler;
}

namespace MCP::Native::Threading
{
  class EventWaiter;
}

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventHandler
  {
  public:
    DebugEventHandler(DebugEventStorage* debugEventStorage,
                      MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                      MCP::Native::Debugger::BreakpointManager* breakpointManager,
                      MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* eventHandler,
                      MCP::Native::Threading::EventWaiter* startupWaiter,
                      MCP::Native::Debugger::DebuggerCommandQueue* commandQueue);

    ~DebugEventHandler();

    void HandleDebugEvent(DEBUG_EVENT& debugEvent, MCP::Native::Debugger::DebugEventHandler::HandleResult& handleResult);

  private:
    std::map<DWORD, std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler>> _eventHandlers;
  };
}