#include "ExitProcessEventHandler.h"
#include "DebugEventStorage.h"
#include "..\Interfaces\IDebuggerEventHandler.h"
#include "..\Interfaces\ExitProcessEventArgs.h"
#include <memory>

namespace MCP::Native::Debugger::DebugEventHandler
{
  ExitProcessEventHandler::ExitProcessEventHandler(DebugEventStorage* debugEventStorage, MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* eventHandler)
  {
    _eventHandler = eventHandler;
    _debugEventStorage = debugEventStorage;
  }

  ExitProcessEventHandler::~ExitProcessEventHandler()
  {
  }


  MCP::Native::Debugger::DebugEventHandler::HandleResult ExitProcessEventHandler::HandleEvent(DEBUG_EVENT& debugEvent)
  {
    NotifyEventHandler(debugEvent);
    _debugEventStorage->RemoveProcess(debugEvent.dwProcessId);

    return { DBG_CONTINUE, true }; // Signal that the process has exited
  }

  void ExitProcessEventHandler::NotifyEventHandler(DEBUG_EVENT& debugEvent)
  {
    if (_eventHandler != nullptr)
    {
      auto args = CreateExitProcessEventArgs(debugEvent);
      _eventHandler->OnExitProcess(args);
    }
  }

  std::shared_ptr<MCP::Native::Debugger::Interfaces::ExitProcessEventArgs> ExitProcessEventHandler::CreateExitProcessEventArgs(DEBUG_EVENT& debugEvent)
  {
    auto args = std::make_shared<MCP::Native::Debugger::Interfaces::ExitProcessEventArgs>();
    args->ProcessId = debugEvent.dwProcessId;
    return args;
  }
}

