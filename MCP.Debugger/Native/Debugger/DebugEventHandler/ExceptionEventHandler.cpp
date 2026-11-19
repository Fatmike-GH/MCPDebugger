#include <Windows.h>
#include "ExceptionEventHandler.h"
#include ".\Interfaces\IDebugEventStorage.h"
#include "..\..\Threading\EventWaiter.h"
#include "..\Interfaces\IDebuggerEventHandler.h"
#include "..\Interfaces\BreakpointEventArgs.h"
#include "..\Interfaces\IBreakpointManager.h"
#include "..\Interfaces\IDebuggerCommandQueue.h"
#include "..\Interfaces\IResumePlan.h"
#include "..\Interfaces\IStopSite.h"
#include "..\Interfaces\StopOutcome.h"
#include <utility>

namespace MCP::Native::Debugger::DebugEventHandler
{
  ExceptionEventHandler::ExceptionEventHandler(Interfaces::IDebugEventStorage& debugEventStorage,
                                               MCP::Native::Debugger::Interfaces::IBreakpointManager& breakpointManager,
                                               MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler,
                                               MCP::Native::Threading::EventWaiter& startupWaiter,
                                               MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue& commandQueue,
                                               std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSiteFactory> stopSiteFactory)
    : _stopSiteFactory(std::move(stopSiteFactory))
  {
    _debugEventStorage = &debugEventStorage;
    _breakpointManager = &breakpointManager;
    _eventHandler = &eventHandler;
    _startupWaiter = &startupWaiter;
    _commandQueue = &commandQueue;

    _systemBreakpoint = true;
    _stepOffPending = false;

    _ntdllDbgBreakPointAddress = (DWORD_PTR)GetProcAddress(GetModuleHandleA("ntdll.dll"), "DbgBreakPoint");
  }

  ExceptionEventHandler::~ExceptionEventHandler()
  {
  }


  MCP::Native::Debugger::DebugEventHandler::HandleResult ExceptionEventHandler::HandleEvent(DEBUG_EVENT& debugEvent)
  {
    ProcessInfo processInfo = { 0 };
    _debugEventStorage->GetProcessById(debugEvent.dwProcessId, processInfo);

    ThreadInfo threadInfo = { 0 };
    _debugEventStorage->GetThreadById(debugEvent.dwThreadId, threadInfo);

    DWORD continueStatus = HandleException(debugEvent.u.Exception, processInfo, threadInfo);
    return { continueStatus, false };
  }

  DWORD ExceptionEventHandler::HandleException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    switch (exception.ExceptionRecord.ExceptionCode)
    {
    case EXCEPTION_BREAKPOINT:
      return HandleBreakpointException(exception, processInfo, threadInfo);
    case EXCEPTION_SINGLE_STEP:
      return HandleSingleStepException(exception, processInfo, threadInfo);
    case EXCEPTION_GUARD_PAGE:
      return HandleGuardPageException(exception, processInfo, threadInfo);
    default:
      return DBG_EXCEPTION_NOT_HANDLED;
    }
  }

  DWORD ExceptionEventHandler::HandleBreakpointException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    DWORD_PTR address = (DWORD_PTR)exception.ExceptionRecord.ExceptionAddress;

    // Breakpoint from DebugBreakProcess
    if (address == _ntdllDbgBreakPointAddress)
    {
      return DBG_CONTINUE;
    }

    // Initial system breakpoint
    if (_systemBreakpoint)
    {
      _systemBreakpoint = false;
      return HandleSystemBreakpoint(exception, processInfo, threadInfo);
    }

    // A breakpoint the current plan placed itself: the debuggee got where the plan wanted it. Asked before
    // the breakpoints of the client are, so that the hit is reported as the end of a step
    if (_currentPlan && _currentPlan->OwnsBreakpointAt(address))
    {
      // Ending the plan takes its breakpoint out of the manager and the debuggee's memory
      std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSite> stopSite = CreateStopSite(exception, processInfo);
      EndCurrentPlan(*stopSite);

      // Back up EIP/RIP because the INT 3 instruction advanced it
      RewindInstructionPointer(threadInfo.info.hThread, (PVOID)address);

      return ReportStepStop(exception, processInfo, threadInfo, *stopSite);
    }

    // A breakpoint of the breakpoint manager. At the process start address it is always the entry-point stop,
    // also for a breakpoint a client set there
    if (_breakpointManager->HasBreakpoint(address))
    {
      DWORD_PTR entryPoint = (DWORD_PTR)processInfo.info.lpStartAddress;
      if (address == entryPoint)
      {
        return HandleEntryPointBreakpoint(exception, processInfo, threadInfo);
      }

      return HandleUserBreakpoint(exception, processInfo, threadInfo);
    }

    // Unknown breakpoint (e.g. hardcoded int 3)
    return DBG_EXCEPTION_NOT_HANDLED;
  }

  DWORD ExceptionEventHandler::HandleSingleStepException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    // The single step every resumed run starts with has ended: the debuggee has left the stop address
    if (_stepOffPending)
    {
      return HandleStepOffBreakpoint(exception, processInfo, threadInfo);
    }

    // Single-step by user
    return HandleStepHalt(exception, processInfo, threadInfo);
  }

  DWORD ExceptionEventHandler::HandleGuardPageException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    /*if (!_breakpointManager || !_breakpointManager->IsMemoryBreakPointActive())
    {
      return DBG_EXCEPTION_NOT_HANDLED;
    }
    _breakpointManager->NotifyMemoryBreakPointRemoved();
    return DBG_CONTINUE;*/

    return DBG_CONTINUE;
  }

  DWORD ExceptionEventHandler::HandleSystemBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    _startupWaiter->Wakeup(true);

    _eventHandler->OnSystemBreakpoint(CreateBreakpointEventArgs(exception, processInfo, threadInfo));

    MCP::Native::Debugger::Interfaces::StopOutcome outcome = _commandQueue->ProcessCommandsSync();

    if (outcome.IsSessionEnded())
    {
      return DBG_EXCEPTION_NOT_HANDLED;
    }

    // Whatever the plan is, the debuggee runs to its entry point from here: the trap flag doesn't work on
    // the system breakpoint, so no plan can step from it. The plan is dropped without being begun
    DWORD_PTR entryPoint = (DWORD_PTR)processInfo.info.lpStartAddress;
    _breakpointManager->SetBreakpoint(entryPoint);

    // OS will handle the initial system breakpoint and run to our new breakpoint
    return DBG_EXCEPTION_NOT_HANDLED;
  }

  DWORD ExceptionEventHandler::HandleEntryPointBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    // First of all: what the plan of the run that ends here placed must be gone when the stop is reported
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSite> stopSite = CreateStopSite(exception, processInfo);
    EndCurrentPlan(*stopSite);

    // The breakpoint on the entry point reports this one stop only
    _breakpointManager->RemoveBreakpoint((DWORD_PTR)exception.ExceptionRecord.ExceptionAddress);

    HoldAndRewindAtBreakpoint(exception, threadInfo);

    _eventHandler->OnEntryPointBreakpoint(CreateBreakpointEventArgs(exception, processInfo, threadInfo));

    return ResumeFromStop(threadInfo, *stopSite);
  }

  DWORD ExceptionEventHandler::HandleUserBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    // First of all: what the plan of the run that ends here placed must be gone when the stop is reported
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSite> stopSite = CreateStopSite(exception, processInfo);
    EndCurrentPlan(*stopSite);

    HoldAndRewindAtBreakpoint(exception, threadInfo);

    _eventHandler->OnBreakpoint(CreateBreakpointEventArgs(exception, processInfo, threadInfo));

    return ResumeFromStop(threadInfo, *stopSite);
  }

  void ExceptionEventHandler::HoldAndRewindAtBreakpoint(EXCEPTION_DEBUG_INFO& exception, ThreadInfo& threadInfo)
  {
    // Takes the breakpoint instruction out of memory before the stop is reported: the listener of the event
    // reads the debuggee's memory
    _breakpointManager->HoldBreakpointAt((DWORD_PTR)exception.ExceptionRecord.ExceptionAddress);

    // Back up EIP/RIP because the INT 3 instruction advanced it
    RewindInstructionPointer(threadInfo.info.hThread, exception.ExceptionRecord.ExceptionAddress);
  }

  DWORD ExceptionEventHandler::ResumeFromStop(ThreadInfo& threadInfo, MCP::Native::Debugger::Interfaces::IStopSite& stopSite)
  {
    MCP::Native::Debugger::Interfaces::StopOutcome outcome = _commandQueue->ProcessCommandsSync();
    if (outcome.IsSessionEnded())
    {
      return DBG_CONTINUE; // Main loop will exit
    }

    // The plan prepares its run while the debuggee still stands at the stop address, before the single
    // step is asked for
    _currentPlan = outcome.TakePlan();
    _currentPlan->Begin(stopSite);

    // The instruction at the stop address has to run before a breakpoint instruction may be written there.
    // That also goes for a breakpoint that is set only after the debuggee was resumed, while the thread
    // still stands at the address - so the hold on the address cannot end here, and it does not matter
    // whether a breakpoint is set there right now. Whatever the plan is, the instruction is executed with
    // one single step; afterwards HandleStepOffBreakpoint releases the address and asks the plan how to go on
    _breakpointManager->SetTrapFlag(threadInfo.info.hThread, true);
    _stepOffPending = true;
    return DBG_CONTINUE;
  }

  DWORD ExceptionEventHandler::HandleStepOffBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    // The debuggee has left the stop address: a breakpoint that is still set there is written now
    _stepOffPending = false;
    _breakpointManager->ReleaseHeldBreakpoint();

    // Without a current plan there is nobody to halt for: another stop has ended the plan while the single
    // step was still pending
    if (_currentPlan && _currentPlan->HaltsAfterFirstInstruction())
    {
      return HandleStepHalt(exception, processInfo, threadInfo);
    }

    return DBG_CONTINUE;
  }

  DWORD ExceptionEventHandler::HandleStepHalt(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSite> stopSite = CreateStopSite(exception, processInfo);
    EndCurrentPlan(*stopSite);

    return ReportStepStop(exception, processInfo, threadInfo, *stopSite);
  }

  DWORD ExceptionEventHandler::ReportStepStop(EXCEPTION_DEBUG_INFO& exception,
                                              ProcessInfo& processInfo,
                                              ThreadInfo& threadInfo,
                                              MCP::Native::Debugger::Interfaces::IStopSite& stopSite)
  {
    // A breakpoint at the address the step ended at must not show in the memory the listener of the event
    // reads, and must not stop the debuggee before the instruction there has run
    _breakpointManager->HoldBreakpointAt((DWORD_PTR)exception.ExceptionRecord.ExceptionAddress);

    _eventHandler->OnSingleStep(CreateBreakpointEventArgs(exception, processInfo, threadInfo));

    return ResumeFromStop(threadInfo, stopSite);
  }

  std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSite> ExceptionEventHandler::CreateStopSite(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo)
  {
    return _stopSiteFactory->Create(processInfo.info.hProcess, (DWORD_PTR)exception.ExceptionRecord.ExceptionAddress);
  }

  void ExceptionEventHandler::EndCurrentPlan(MCP::Native::Debugger::Interfaces::IStopSite& stopSite)
  {
    if (!_currentPlan)
    {
      return;
    }

    _currentPlan->End(stopSite);
    _currentPlan.reset();
  }

  void ExceptionEventHandler::RewindInstructionPointer(HANDLE threadHandle, PVOID address)
  {
    CONTEXT context = { 0 };
    context.ContextFlags = CONTEXT_FULL;
    GetThreadContext(threadHandle, &context);
#ifdef _WIN64
    context.Rip = (DWORD64)address;
#else
    context.Eip = (DWORD32)address;
#endif
    SetThreadContext(threadHandle, &context);
  }

  std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> ExceptionEventHandler::CreateBreakpointEventArgs(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    auto args = std::make_shared<MCP::Native::Debugger::Interfaces::BreakpointEventArgs>();
    args->ProcessId = processInfo.ProcessId;
    args->ThreadId = threadInfo.ThreadId;

    args->Context = { 0 };
    args->Context.ContextFlags = CONTEXT_FULL;
    GetThreadContext(threadInfo.info.hThread, &args->Context);

    return args;
  }
}

