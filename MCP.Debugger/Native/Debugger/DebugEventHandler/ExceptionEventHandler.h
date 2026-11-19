#pragma once
#include <memory>
#include ".\Interfaces\IDebugEventHandler.h"
#include "ProcessInfo.h"
#include "ThreadInfo.h"
#include "DllInfo.h"
#include "..\Interfaces\IStopSiteFactory.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpointManager;
  class IDebuggerCommandQueue;
  class IDebuggerEventHandler;
  class IResumePlan;
  struct BreakpointEventArgs;
  struct ExitProcessEventArgs;
}

namespace MCP::Native::Debugger::DebugEventHandler::Interfaces
{
  class IDebugEventStorage;
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
    /// The handler owns the stop-site factory; all other collaborators are borrowed and must outlive the
    /// handler. The waiter is released when the debuggee reached its initial breakpoint.
    /// </summary>
    ExceptionEventHandler(Interfaces::IDebugEventStorage& debugEventStorage,
                          MCP::Native::Debugger::Interfaces::IBreakpointManager& breakpointManager,
                          MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler,
                          MCP::Native::Threading::EventWaiter& startupWaiter,
                          MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue& commandQueue,
                          std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSiteFactory> stopSiteFactory);

    ~ExceptionEventHandler();

    MCP::Native::Debugger::DebugEventHandler::HandleResult HandleEvent(DEBUG_EVENT& debugEvent) override;

  private:
    DWORD HandleException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleBreakpointException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfot);
    DWORD HandleSingleStepException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleGuardPageException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);

    DWORD HandleSystemBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleUserBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleStepOffBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);
    DWORD HandleStepHalt(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);

    /// <summary>
    /// Reports a stop at the end of a step - the exception address - and waits for the client's answer. The
    /// plan that led here must have been ended already; the stop site is the one of this stop.
    /// </summary>
    DWORD ReportStepStop(EXCEPTION_DEBUG_INFO& exception,
                         ProcessInfo& processInfo,
                         ThreadInfo& threadInfo,
                         MCP::Native::Debugger::Interfaces::IStopSite& stopSite);

    /// <summary>
    /// Waits for the command that ends a stop that was reported. When it resumes the debuggee, its plan
    /// becomes the current plan and is begun at the stop site, and the debuggee is resumed from the stop
    /// address. The breakpoint manager must hold that address; it stays held until the instruction there has
    /// run, which is done with one single step whatever the plan is (HandleStepOffBreakpoint handles its
    /// end). When the command ends the session, the debuggee is not resumed.
    /// </summary>
    DWORD ResumeFromStop(ThreadInfo& threadInfo, MCP::Native::Debugger::Interfaces::IStopSite& stopSite);

    /// <summary>Creates the stop site of the stop the exception reports: its process and address.</summary>
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSite> CreateStopSite(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo);

    /// <summary>
    /// Ends the plan the debuggee was resumed with, at the stop that follows it. A plan lives until the next
    /// stop that is reported and no longer: what it placed must be gone before the client sees that stop.
    /// Does nothing when no plan is current.
    /// </summary>
    void EndCurrentPlan(MCP::Native::Debugger::Interfaces::IStopSite& stopSite);

    void RewindInstructionPointer(HANDLE threadHandle, PVOID address);
    std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> CreateBreakpointEventArgs(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo);

  private:
    Interfaces::IDebugEventStorage* _debugEventStorage;
    MCP::Native::Debugger::Interfaces::IBreakpointManager* _breakpointManager;
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* _eventHandler;
    MCP::Native::Threading::EventWaiter* _startupWaiter;
    MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue* _commandQueue;
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSiteFactory> _stopSiteFactory;

    bool _systemBreakpoint;
    bool _stepOffPending; // True while the debuggee executes the one instruction at the stop address it was resumed from
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IResumePlan> _currentPlan; // The plan the debuggee was resumed with, until the next stop is reported; null otherwise
    DWORD_PTR _ntdllDbgBreakPointAddress; // Required to detect DebugBreakProcess
  };
}