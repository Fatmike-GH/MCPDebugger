#include "DebugThreadServicesFactory.h"
#include <map>
#include <utility>
#include "BreakpointFactory.h"
#include "DebugThreadServices.h"
#include "..\Debugger\BreakpointManager.h"
#include "..\Debugger\DebuggeeBreakIn.h"
#include "..\Debugger\DebuggerCommandQueue.h"
#include "..\Debugger\DebugEventHandler\CreateProcessEventHandler.h"
#include "..\Debugger\DebugEventHandler\CreateThreadEventHandler.h"
#include "..\Debugger\DebugEventHandler\DebugEventHandler.h"
#include "..\Debugger\DebugEventHandler\DefaultEventHandler.h"
#include "..\Debugger\DebugEventHandler\ExceptionEventHandler.h"
#include "..\Debugger\DebugEventHandler\ExitProcessEventHandler.h"
#include "..\Debugger\DebugEventHandler\ExitThreadEventHandler.h"
#include "..\Debugger\DebugEventHandler\LoadDllEventHandler.h"
#include "..\Debugger\DebugEventHandler\OutputDebugStringEventHandler.h"
#include "..\Debugger\DebugEventHandler\UnloadDllEventHandler.h"
#include "..\Debugger\DebugEventHandler\Interfaces\IDebugEventStorage.h"
#include "..\Debugger\Interfaces\IDebuggerEventHandler.h"
#include "..\Disassembler\Disassembler.h"
#include "..\Disassembler\SymbolResolver.h"
#include "..\Platform\ProcessMemory.h"
#include "..\ProcessExplorer\Interfaces\IProcessExplorer.h"
#include "..\Threading\EventWaiter.h"

namespace MCP::Native::Composition
{
  DebugThreadServicesFactory::DebugThreadServicesFactory(MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer& processExplorer,
                                                         MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventStorage& debugEventStorage,
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
    auto symbolResolver = std::make_unique<MCP::Native::Disassembler::SymbolResolver>(_processExplorer);
    auto processMemory = std::make_unique<MCP::Native::Platform::ProcessMemory>();
    auto disassembler = std::make_unique<MCP::Native::Disassembler::Disassembler>(*symbolResolver, *processMemory);
    auto debuggeeBreakIn = std::make_unique<MCP::Native::Debugger::DebuggeeBreakIn>();
    auto commandQueue = std::make_unique<MCP::Native::Debugger::DebuggerCommandQueue>(
      _debugEventStorage, *breakpointManager, *disassembler, *symbolResolver, *debuggeeBreakIn);
    auto debugEventDispatcher = CreateDebugEventDispatcher(*breakpointManager, *commandQueue, *disassembler, startupWaiter);

    return std::make_unique<DebugThreadServices>(std::move(breakpointManager),
                                                 std::move(symbolResolver),
                                                 std::move(processMemory),
                                                 std::move(disassembler),
                                                 std::move(debuggeeBreakIn),
                                                 std::move(commandQueue),
                                                 std::move(debugEventDispatcher));
  }

  std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventDispatcher> DebugThreadServicesFactory::CreateDebugEventDispatcher(
    MCP::Native::Debugger::Interfaces::IBreakpointManager& breakpointManager,
    MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue& commandQueue,
    MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler,
    MCP::Native::Threading::EventWaiter& startupWaiter)
  {
    namespace Events = MCP::Native::Debugger::DebugEventHandler;

    std::map<DWORD, std::unique_ptr<Events::Interfaces::IDebugEventHandler>> eventHandlers;
    eventHandlers[EXCEPTION_DEBUG_EVENT] = std::make_unique<Events::ExceptionEventHandler>(
      _debugEventStorage, breakpointManager, _eventHandler, startupWaiter, commandQueue, disassembler);
    eventHandlers[EXIT_PROCESS_DEBUG_EVENT] = std::make_unique<Events::ExitProcessEventHandler>(_debugEventStorage, _eventHandler);
    eventHandlers[CREATE_PROCESS_DEBUG_EVENT] = std::make_unique<Events::CreateProcessEventHandler>(_debugEventStorage);
    eventHandlers[CREATE_THREAD_DEBUG_EVENT] = std::make_unique<Events::CreateThreadEventHandler>(_debugEventStorage);
    eventHandlers[EXIT_THREAD_DEBUG_EVENT] = std::make_unique<Events::ExitThreadEventHandler>(_debugEventStorage);
    eventHandlers[LOAD_DLL_DEBUG_EVENT] = std::make_unique<Events::LoadDllEventHandler>(_debugEventStorage);
    eventHandlers[UNLOAD_DLL_DEBUG_EVENT] = std::make_unique<Events::UnloadDllEventHandler>(_debugEventStorage);
    eventHandlers[OUTPUT_DEBUG_STRING_EVENT] = std::make_unique<Events::OutputDebugStringEventHandler>();

    return std::make_unique<Events::DebugEventHandler>(std::move(eventHandlers), std::make_unique<Events::DefaultEventHandler>());
  }
}

