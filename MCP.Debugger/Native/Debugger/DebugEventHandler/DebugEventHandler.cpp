#include "DebugEventHandler.h"
#include "CreateProcessEventHandler.h"
#include "CreateThreadEventHandler.h"
#include "DefaultEventHandler.h"
#include "ExceptionEventHandler.h"
#include "ExitProcessEventHandler.h"
#include "ExitThreadEventHandler.h"
#include "LoadDllEventHandler.h"
#include "OutputDebugStringEventHandler.h"
#include "UnloadDllEventHandler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  DebugEventHandler::DebugEventHandler(DebugEventStorage* debugEventStorage,
                                       MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                                       MCP::Native::Debugger::BreakpointManager* breakpointManager,
                                       MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* eventHandler,
                                       MCP::Native::Threading::EventWaiter* startupWaiter,
                                       MCP::Native::Debugger::DebuggerCommandQueue* commandQueue)
  {
    _eventHandlers.clear();
    _eventHandlers[EXCEPTION_DEBUG_EVENT] = std::make_unique<ExceptionEventHandler>(debugEventStorage, processExplorer, breakpointManager, eventHandler, startupWaiter, commandQueue);
    _eventHandlers[EXIT_PROCESS_DEBUG_EVENT] = std::make_unique<ExitProcessEventHandler>(debugEventStorage, eventHandler);
    _eventHandlers[CREATE_PROCESS_DEBUG_EVENT] = std::make_unique<CreateProcessEventHandler>(debugEventStorage);
    _eventHandlers[CREATE_THREAD_DEBUG_EVENT] = std::make_unique<CreateThreadEventHandler>(debugEventStorage);
    _eventHandlers[EXIT_THREAD_DEBUG_EVENT] = std::make_unique<ExitThreadEventHandler>(debugEventStorage);
    _eventHandlers[LOAD_DLL_DEBUG_EVENT] = std::make_unique<LoadDllEventHandler>(debugEventStorage);
    _eventHandlers[UNLOAD_DLL_DEBUG_EVENT] = std::make_unique<UnloadDllEventHandler>(debugEventStorage);
    _eventHandlers[OUTPUT_DEBUG_STRING_EVENT] = std::make_unique<OutputDebugStringEventHandler>();
  }

  DebugEventHandler::~DebugEventHandler()
  {
  }

  void DebugEventHandler::HandleDebugEvent(DEBUG_EVENT& debugEvent, MCP::Native::Debugger::DebugEventHandler::HandleResult& handleResult)
  {
    auto it = _eventHandlers.find(debugEvent.dwDebugEventCode);
    if (it != _eventHandlers.end())
    {
      handleResult = it->second->HandleEvent(debugEvent);
    }
    else
    {
      DefaultEventHandler defaultHandler;
      handleResult = defaultHandler.HandleEvent(debugEvent);
    }
  }
}

