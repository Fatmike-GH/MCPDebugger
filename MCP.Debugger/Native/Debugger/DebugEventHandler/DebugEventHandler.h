#pragma once
#include <Windows.h>
#include <map>
#include <memory>
#include ".\Interfaces\IDebugEventDispatcher.h"
#include ".\Interfaces\IDebugEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventHandler : public Interfaces::IDebugEventDispatcher
  {
  public:
    /// <summary>
    /// Takes ownership of the handlers: one per event code it is responsible for, and the default handler
    /// for every event code without an entry. None of them may be null.
    /// </summary>
    DebugEventHandler(std::map<DWORD, std::unique_ptr<Interfaces::IDebugEventHandler>> eventHandlers,
                      std::unique_ptr<Interfaces::IDebugEventHandler> defaultHandler);

    ~DebugEventHandler() override;

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleDebugEvent(DEBUG_EVENT& debugEvent) override;

  private:
    std::map<DWORD, std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler>> _eventHandlers;
    std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler> _defaultHandler;
  };
}
