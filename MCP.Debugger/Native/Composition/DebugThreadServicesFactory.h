#pragma once
#include <Windows.h>
#include <memory>
#include "..\Debugger\Interfaces\IDebugThreadServicesFactory.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;
}

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventDispatcher;
  class IDebugEventStorage;
}

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpointManager;
  class IDebuggerCommandQueue;
}

namespace MCP::Native::Disassembler::Interfaces
{
  class IDisassembler;
}

namespace MCP::Native::ProcessExplorer::Interfaces
{
  class IProcessExplorer;
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
    DebugThreadServicesFactory(MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer& processExplorer,
                               MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorage& debugEventStorage,
                               MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler);

    std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebugThreadServices> Create(
      HANDLE processHandle,
      MCP::Native::Threading::EventWaiter& startupWaiter) override;

  private:
    /// <summary>
    /// Creates the handler of every debug event code the debugger reacts to, the handler for all other
    /// codes, and the dispatcher that routes the events to them. The parts are borrowed by the handlers.
    /// </summary>
    std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventDispatcher> CreateDebugEventDispatcher(
      MCP::Native::Debugger::Interfaces::IBreakpointManager& breakpointManager,
      MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue& commandQueue,
      MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler,
      MCP::Native::Threading::EventWaiter& startupWaiter);

  private:
    MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer& _processExplorer;
    MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorage& _debugEventStorage;
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& _eventHandler;
  };
}
