#pragma once
#include <Windows.h>
#include <memory>
#include "..\Debugger\Interfaces\IDebugThreadServicesFactory.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class DebugEventStorage;
}

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer;
}

namespace MCP::Native::Composition
{
  /// <summary>
  /// Builds the objects that serve one launched debuggee on its debug thread from the classes of this
  /// library.
  /// </summary>
  class DebugThreadServicesFactory : public MCP::Native::Debugger::Interfaces::IDebugThreadServicesFactory
  {
  public:
    /// <summary>
    /// The explorer, the storage and the event handler are borrowed: everything the factory creates uses
    /// them, so they must outlive the factory and its results.
    /// </summary>
    DebugThreadServicesFactory(MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                               MCP::Native::Debugger::DebugEventHandler::DebugEventStorage* debugEventStorage,
                               MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler);

    std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebugThreadServices> Create(
      HANDLE processHandle,
      MCP::Native::Threading::EventWaiter& startupWaiter) override;

  private:
    MCP::Native::ProcessExplorer::ProcessExplorer* _processExplorer;
    MCP::Native::Debugger::DebugEventHandler::DebugEventStorage* _debugEventStorage;
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& _eventHandler;
  };
}
