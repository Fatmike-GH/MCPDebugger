#include "DebugEventHandler.h"
#include <utility>

namespace MCP::Native::Debugger::DebugEventHandler
{
  DebugEventHandler::DebugEventHandler(std::map<DWORD, std::unique_ptr<Interfaces::IDebugEventHandler>> eventHandlers,
                                       std::unique_ptr<Interfaces::IDebugEventHandler> defaultHandler)
    : _eventHandlers(std::move(eventHandlers)),
      _defaultHandler(std::move(defaultHandler))
  {
  }

  DebugEventHandler::~DebugEventHandler()
  {
  }

  MCP::Native::Debugger::DebugEventHandler::HandleResult DebugEventHandler::HandleDebugEvent(DEBUG_EVENT& debugEvent)
  {
    auto it = _eventHandlers.find(debugEvent.dwDebugEventCode);
    if (it != _eventHandlers.end())
    {
      return it->second->HandleEvent(debugEvent);
    }

    return _defaultHandler->HandleEvent(debugEvent);
  }
}

