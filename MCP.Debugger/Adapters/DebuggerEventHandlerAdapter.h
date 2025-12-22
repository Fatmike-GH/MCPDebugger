#pragma once
#include <Windows.h>
#include "..\Native\Debugger\Interfaces\IDebuggerEventHandler.h"
#include <vcclr.h>

namespace MCP::Debugger
{
  ref class DebuggerService;
}

namespace MCP::Adapters
{
  class DebuggerEventHandlerAdapter : public MCP::Native::Debugger::Interfaces::IDebuggerEventHandler
  {
  public:
    DebuggerEventHandlerAdapter(MCP::Debugger::DebuggerService^ managedHandler);

    void OnExitProcess(std::shared_ptr<MCP::Native::Debugger::Interfaces::ExitProcessEventArgs> args) override;
    void OnSystemBreakpoint(std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args) override;
    void OnEntryPointBreakpoint(std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args) override;
    void OnBreakpoint(std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args) override;
    void OnSingleStep(std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args) override;

  private:
    MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ CreateBreakpointEventArgs(DWORD processId, DWORD threadId, const ::CONTEXT& context);

  private:
    gcroot<MCP::Debugger::DebuggerService^> _managedHandler;        
  };
}