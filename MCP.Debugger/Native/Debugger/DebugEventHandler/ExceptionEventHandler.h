#pragma once
#include <memory>
#include ".\Interfaces\IDebugEventHandler.h"
#include "DebugEventStorage.h"
#include "ProcessInfo.h"
#include "ThreadInfo.h"
#include "DllInfo.h"
#include "..\Commands\Base\CommandType.h"

namespace MCP::Native::Debugger
{
  class BreakpointManager;
  class Breakpoint;
  class DebuggerCommandQueue;
}

namespace MCP::Native::Debugger::Interfaces
{
  class IDebuggerEventHandler;
  struct BreakpointEventArgs;
  struct ExitProcessEventArgs;
}

namespace MCP::Native::Threading
{
  class EventWaiter;
}

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class ExceptionEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    ExceptionEventHandler(DebugEventStorage* debugEventStorage,
                          MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                          MCP::Native::Debugger::BreakpointManager* breakpointManager,
                          MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* eventHandler,
                          MCP::Native::Threading::EventWaiter* startupWaiter,
                          MCP::Native::Debugger::DebuggerCommandQueue* commandQueue);

    ~ExceptionEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    DWORD HandleException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleBreakpointException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfot);
    DWORD HandleSingleStepException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleGuardPageException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);

    DWORD HandleSystemBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleUserBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo, Breakpoint* breakpoint);
    DWORD HandleStepOffBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleStepHalt(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);

    void RewindInstructionPointer(HANDLE threadHandle, PVOID address);
    bool IsCallInstruction(HANDLE processHandle, DWORD_PTR currentIp, size_t& instrLen);
    std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> CreateBreakpointEventArgs(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);

  private:
    DebugEventStorage* _debugEventStorage;
    MCP::Native::ProcessExplorer::ProcessExplorer* _processExplorer;
    MCP::Native::Debugger::BreakpointManager* _breakpointManager;
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* _eventHandler;
    MCP::Native::Threading::EventWaiter* _startupWaiter;
    MCP::Native::Debugger::DebuggerCommandQueue* _commandQueue;

    bool _systemBreakpoint;
    Breakpoint* _lastBreakpoint; // Required to enable the user breakpoint again
    MCP::Native::Debugger::Commands::Base::CommandType _postBreakpointAction; // Required to enable the user breakpoint again
    DWORD_PTR _ntdllDbgBreakPointAddress; // Required to detect DebugBreakProcess
    DWORD_PTR _stepOverBreakpointAddress = 0; // 0 means no active step-over
  };
}