#pragma once
#include <Windows.h>
#include <memory>
#include "..\Debugger\Interfaces\IDebugThreadServices.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpointManager;
  class IDebuggeeBreakIn;
  class IDebuggerCommandQueue;
}

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventDispatcher;
}

namespace MCP::Native::Disassembler::Interfaces
{
  class IDisassembler;
  class ISymbolResolver;
}

namespace MCP::Native::Platform::Interfaces
{
  class IProcessMemory;
}

namespace MCP::Native::Composition
{
  /// <summary>
  /// Owns the objects that serve one launched debuggee on its debug thread: the breakpoints, what the command
  /// queue works with, the queue itself and the dispatcher of the debuggee's debug events.
  /// </summary>
  class DebugThreadServices : public MCP::Native::Debugger::Interfaces::IDebugThreadServices
  {
  public:
    /// <summary>
    /// Takes ownership of the parts. They must have been wired to each other: the disassembler uses the
    /// symbol resolver and the process memory; the queue uses the breakpoint manager, the disassembler, the
    /// symbol resolver and the break-in; the handlers behind the dispatcher use the breakpoint manager, the
    /// disassembler and the queue.
    /// </summary>
    DebugThreadServices(std::unique_ptr<MCP::Native::Debugger::Interfaces::IBreakpointManager> breakpointManager,
                        std::unique_ptr<MCP::Native::Disassembler::Interfaces::ISymbolResolver> symbolResolver,
                        std::unique_ptr<MCP::Native::Platform::Interfaces::IProcessMemory> processMemory,
                        std::unique_ptr<MCP::Native::Disassembler::Interfaces::IDisassembler> disassembler,
                        std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggeeBreakIn> debuggeeBreakIn,
                        std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue> commandQueue,
                        std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventDispatcher> debugEventDispatcher);
    ~DebugThreadServices() override;

    DebugThreadServices(const DebugThreadServices&) = delete;
    DebugThreadServices& operator=(const DebugThreadServices&) = delete;
    DebugThreadServices(DebugThreadServices&&) = delete;
    DebugThreadServices& operator=(DebugThreadServices&&) = delete;

    MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue& GetCommandQueue() override;
    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleDebugEvent(DEBUG_EVENT& debugEvent) override;

  private:
    // Declaration order is destruction order reversed: the dispatcher with its handlers goes first, then
    // the queue, then what the queue borrows, and the breakpoint manager - which both of them use - last.
    // The symbol resolver and the process memory are declared before the disassembler because the
    // disassembler borrows them.
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IBreakpointManager> _breakpointManager;
    std::unique_ptr<MCP::Native::Disassembler::Interfaces::ISymbolResolver> _symbolResolver;
    std::unique_ptr<MCP::Native::Platform::Interfaces::IProcessMemory> _processMemory;
    std::unique_ptr<MCP::Native::Disassembler::Interfaces::IDisassembler> _disassembler;
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggeeBreakIn> _debuggeeBreakIn;
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue> _commandQueue;
    std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventDispatcher> _debugEventDispatcher;
  };
}
