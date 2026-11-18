#include "DebuggerSessionFactory.h"
#include "DebuggerSession.h"
#include "DebugThreadServicesFactory.h"
#include "..\Debugger\DebuggerThread.h"
#include "..\Debugger\DebugEventHandler\DebugEventStorage.h"
#include "..\Debugger\Interfaces\IDebuggerEventHandler.h"
#include "..\ProcessExplorer\ProcessExplorer.h"

namespace MCP::Native::Composition
{
  std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerSession> DebuggerSessionFactory::CreateSession(
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler)
  {
    auto processExplorer = std::make_unique<MCP::Native::ProcessExplorer::ProcessExplorer>();
    auto debugEventStorage = std::make_unique<MCP::Native::Debugger::DebugEventHandler::DebugEventStorage>();
    auto servicesFactory = std::make_unique<DebugThreadServicesFactory>(processExplorer.get(), debugEventStorage.get(), eventHandler);
    auto debuggerThread = std::make_unique<MCP::Native::Debugger::DebuggerThread>(std::move(servicesFactory));

    // The explorer builds its module bookkeeping from what the storage records about processes and DLLs.
    debugEventStorage->AddListener(processExplorer.get());

    return std::make_unique<DebuggerSession>(std::move(processExplorer), std::move(debugEventStorage), std::move(debuggerThread));
  }
}
