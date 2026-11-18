#include "DebugThreadServices.h"
#include <utility>
#include "..\Debugger\DebugEventHandler\DebugEventHandler.h"
#include "..\Debugger\Interfaces\IBreakpointManager.h"
#include "..\Debugger\Interfaces\IDebuggeeBreakIn.h"
#include "..\Debugger\Interfaces\IDebuggerCommandQueue.h"
#include "..\Disassembler\Interfaces\IDisassembler.h"
#include "..\Disassembler\Interfaces\ISymbolResolver.h"

namespace MCP::Native::Composition
{
  DebugThreadServices::DebugThreadServices(std::unique_ptr<MCP::Native::Debugger::Interfaces::IBreakpointManager> breakpointManager,
                                           std::unique_ptr<MCP::Native::Disassembler::Interfaces::IDisassembler> disassembler,
                                           std::unique_ptr<MCP::Native::Disassembler::Interfaces::ISymbolResolver> symbolResolver,
                                           std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggeeBreakIn> debuggeeBreakIn,
                                           std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue> commandQueue,
                                           std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::DebugEventHandler> debugEventHandler)
    : _breakpointManager(std::move(breakpointManager)),
      _disassembler(std::move(disassembler)),
      _symbolResolver(std::move(symbolResolver)),
      _debuggeeBreakIn(std::move(debuggeeBreakIn)),
      _commandQueue(std::move(commandQueue)),
      _debugEventHandler(std::move(debugEventHandler))
  {
  }

  DebugThreadServices::~DebugThreadServices() = default;

  MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue& DebugThreadServices::GetCommandQueue()
  {
    return *_commandQueue;
  }

  MCP::Native::Debugger::DebugEventHandler::HandleResult DebugThreadServices::HandleDebugEvent(DEBUG_EVENT& debugEvent)
  {
    MCP::Native::Debugger::DebugEventHandler::HandleResult result;
    _debugEventHandler->HandleDebugEvent(debugEvent, result);
    return result;
  }
}
