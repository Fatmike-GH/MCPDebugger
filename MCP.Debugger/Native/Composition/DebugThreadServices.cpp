#include "DebugThreadServices.h"
#include <utility>
#include "..\Debugger\DebugEventHandler\Interfaces\IDebugEventDispatcher.h"
#include "..\Debugger\Interfaces\IBreakpointManager.h"
#include "..\Debugger\Interfaces\IDebuggeeBreakIn.h"
#include "..\Debugger\Interfaces\IDebuggerCommandQueue.h"
#include "..\Disassembler\Interfaces\IDisassembler.h"
#include "..\Disassembler\Interfaces\ISymbolResolver.h"

namespace MCP::Native::Composition
{
  DebugThreadServices::DebugThreadServices(std::unique_ptr<MCP::Native::Debugger::Interfaces::IBreakpointManager> breakpointManager,
                                           std::unique_ptr<MCP::Native::Disassembler::Interfaces::ISymbolResolver> symbolResolver,
                                           std::unique_ptr<MCP::Native::Disassembler::Interfaces::IDisassembler> disassembler,
                                           std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggeeBreakIn> debuggeeBreakIn,
                                           std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue> commandQueue,
                                           std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventDispatcher> debugEventDispatcher)
    : _breakpointManager(std::move(breakpointManager)),
      _symbolResolver(std::move(symbolResolver)),
      _disassembler(std::move(disassembler)),
      _debuggeeBreakIn(std::move(debuggeeBreakIn)),
      _commandQueue(std::move(commandQueue)),
      _debugEventDispatcher(std::move(debugEventDispatcher))
  {
  }

  DebugThreadServices::~DebugThreadServices() = default;

  MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue& DebugThreadServices::GetCommandQueue()
  {
    return *_commandQueue;
  }

  MCP::Native::Debugger::DebugEventHandler::HandleResult DebugThreadServices::HandleDebugEvent(DEBUG_EVENT& debugEvent)
  {
    return _debugEventDispatcher->HandleDebugEvent(debugEvent);
  }
}
