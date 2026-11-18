#pragma once
#include <memory>
#include ".\Interfaces\IDebugEventHandler.h"
#include "ProcessInfo.h"
#include "ThreadInfo.h"
#include "DllInfo.h"
#include "..\Commands\Base\CommandType.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpoint;
  class IBreakpointManager;
  class IDebuggerCommandQueue;
  class IDebuggerEventHandler;
  struct BreakpointEventArgs;
  struct ExitProcessEventArgs;
}

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventStorage;
}

namespace MCP::Native::Disassembler::Interfaces
{
  class IDisassembler;
}

namespace MCP::Native::Threading
{
  class EventWaiter;
}

namespace MCP::Native::Debugger::DebugEventHandler
{
  class ExceptionEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    /// <summary>
    /// All collaborators are borrowed and must outlive the handler. The waiter is released when the debuggee
    /// reached its initial breakpoint.
    /// </summary>
    ExceptionEventHandler(Interfaces::IDebugEventStorage& debugEventStorage,
                          MCP::Native::Debugger::Interfaces::IBreakpointManager& breakpointManager,
                          MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler,
                          MCP::Native::Threading::EventWaiter& startupWaiter,
                          MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue& commandQueue,
                          MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler);

    ~ExceptionEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    DWORD HandleException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleBreakpointException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfot);
    DWORD HandleSingleStepException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleGuardPageException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);

    DWORD HandleSystemBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleUserBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo, MCP::Native::Debugger::Interfaces::IBreakpoint* breakpoint);
    DWORD HandleStepOffBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleStepHalt(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);

    void RewindInstructionPointer(HANDLE threadHandle, PVOID address);
    bool IsCallInstruction(HANDLE processHandle, DWORD_PTR currentIp, size_t& instrLen);
    std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> CreateBreakpointEventArgs(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);

  private:
    Interfaces::IDebugEventStorage* _debugEventStorage;
    MCP::Native::Debugger::Interfaces::IBreakpointManager* _breakpointManager;
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* _eventHandler;
    MCP::Native::Threading::EventWaiter* _startupWaiter;
    MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue* _commandQueue;
    MCP::Native::Disassembler::Interfaces::IDisassembler* _disassembler;

    bool _systemBreakpoint;
    MCP::Native::Debugger::Interfaces::IBreakpoint* _lastBreakpoint; // Required to enable the user breakpoint again
    MCP::Native::Debugger::Commands::Base::CommandType _postBreakpointAction; // Required to enable the user breakpoint again
    DWORD_PTR _ntdllDbgBreakPointAddress; // Required to detect DebugBreakProcess
    DWORD_PTR _stepOverBreakpointAddress = 0; // 0 means no active step-over
  };
}