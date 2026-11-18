#pragma once
#include <memory>

namespace MCP::Native::Debugger::Interfaces
{
  struct BreakpointEventArgs;
  struct ExitProcessEventArgs;
}

namespace MCP::Native::Debugger::Interfaces
{
  // Event notification for managed clients; implemented by DebuggerEventHandlerAdapter, which converts the
  // events and passes them on to the managed side
  class IDebuggerEventHandler
  {
  public:
    virtual ~IDebuggerEventHandler() = default;
    virtual void OnExitProcess(std::shared_ptr<ExitProcessEventArgs> args) = 0;
    virtual void OnSystemBreakpoint(std::shared_ptr<BreakpointEventArgs> args) = 0;
    virtual void OnEntryPointBreakpoint(std::shared_ptr<BreakpointEventArgs> args) = 0;
    virtual void OnBreakpoint(std::shared_ptr<BreakpointEventArgs> args) = 0;
    virtual void OnSingleStep(std::shared_ptr<BreakpointEventArgs> args) = 0;
  };
}