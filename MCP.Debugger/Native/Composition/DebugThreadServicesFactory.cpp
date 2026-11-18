#include "DebugThreadServicesFactory.h"
#include <utility>
#include "BreakpointFactory.h"
#include "DebugThreadServices.h"
#include "..\Debugger\BreakpointManager.h"
#include "..\Debugger\DebuggeeBreakIn.h"
#include "..\Debugger\DebuggerCommandQueue.h"
#include "..\Debugger\DebugEventHandler\DebugEventHandler.h"
#include "..\Debugger\DebugEventHandler\DebugEventStorage.h"
#include "..\Debugger\Interfaces\IDebuggerEventHandler.h"
#include "..\Disassembler\Disassembler.h"
#include "..\Disassembler\SymbolResolver.h"
#include "..\ProcessExplorer\ProcessExplorer.h"
#include "..\Threading\EventWaiter.h"

namespace MCP::Native::Composition
{
  DebugThreadServicesFactory::DebugThreadServicesFactory(MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                                                         MCP::Native::Debugger::DebugEventHandler::DebugEventStorage* debugEventStorage,
                                                         MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler)
    : _processExplorer(processExplorer),
      _debugEventStorage(debugEventStorage),
      _eventHandler(eventHandler)
  {
  }

  std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebugThreadServices> DebugThreadServicesFactory::Create(
    HANDLE processHandle,
    MCP::Native::Threading::EventWaiter& startupWaiter)
  {
    auto breakpointManager = std::make_unique<MCP::Native::Debugger::BreakpointManager>(
      processHandle, std::make_unique<BreakpointFactory>(processHandle));
    auto disassembler = std::make_unique<MCP::Native::Disassembler::Disassembler>(_processExplorer);
    auto symbolResolver = std::make_unique<MCP::Native::Disassembler::SymbolResolver>(_processExplorer);
    auto debuggeeBreakIn = std::make_unique<MCP::Native::Debugger::DebuggeeBreakIn>();
    auto commandQueue = std::make_unique<MCP::Native::Debugger::DebuggerCommandQueue>(
      *_debugEventStorage, *breakpointManager, *disassembler, *symbolResolver, *debuggeeBreakIn);
    auto debugEventHandler = std::make_unique<MCP::Native::Debugger::DebugEventHandler::DebugEventHandler>(
      _debugEventStorage, _processExplorer, breakpointManager.get(), &_eventHandler, &startupWaiter, commandQueue.get());

    return std::make_unique<DebugThreadServices>(std::move(breakpointManager),
                                                 std::move(disassembler),
                                                 std::move(symbolResolver),
                                                 std::move(debuggeeBreakIn),
                                                 std::move(commandQueue),
                                                 std::move(debugEventHandler));
  }
}
