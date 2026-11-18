#include <Windows.h>
#include "ExceptionEventHandler.h"
#include ".\Interfaces\IDebugEventStorage.h"
#include "..\..\Threading\EventWaiter.h"
#include "..\Interfaces\IDebuggerEventHandler.h"
#include "..\Interfaces\BreakpointEventArgs.h"
#include "..\Interfaces\IBreakpointManager.h"
#include "..\Interfaces\IDebuggerCommandQueue.h"
#include "..\..\Disassembler\Interfaces\IDisassembler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  ExceptionEventHandler::ExceptionEventHandler(Interfaces::IDebugEventStorage& debugEventStorage,
                                               MCP::Native::Debugger::Interfaces::IBreakpointManager& breakpointManager,
                                               MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler,
                                               MCP::Native::Threading::EventWaiter& startupWaiter,
                                               MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue& commandQueue,
                                               MCP::Native::Disassembler::Interfaces::IDisassembler& disassembler)
  {
    _debugEventStorage = &debugEventStorage;
    _breakpointManager = &breakpointManager;
    _eventHandler = &eventHandler;
    _startupWaiter = &startupWaiter;
    _commandQueue = &commandQueue;
    _disassembler = &disassembler;

    _systemBreakpoint = true;
    _stepOffPending = false;
    _postBreakpointAction = MCP::Native::Debugger::Commands::Base::CommandType::CMD_NONE;

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

    if (_stepOverBreakpointAddress != 0 && address == _stepOverBreakpointAddress)
    {
      // We landed after the CALL.
      // Remove the temporary breakpoint from the manager and memory
      _breakpointManager->RemoveBreakpoint(address);
      _stepOverBreakpointAddress = 0;

      // Back up EIP/RIP because the INT 3 instruction advanced it
      RewindInstructionPointer(threadInfo.info.hThread, (PVOID)address);

      // Notify that a "Step" completed
      // We treat the end of a Step Over just like a Single Step completion
      return HandleStepHalt(exception, processInfo, threadInfo);
    }

    // User-defined breakpoint or the breakpoint on entryPoint
    if (_breakpointManager->HasBreakpoint(address))
    {
      return HandleUserBreakpoint(exception, processInfo, threadInfo);
    }

    // Unknown breakpoint (e.g. hardcoded int 3)
    return DBG_EXCEPTION_NOT_HANDLED;
  }

  DWORD ExceptionEventHandler::HandleSingleStepException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    // Just stepped one instruction off a stop address that has a breakpoint, to be able to write it afterwards
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

    MCP::Native::Debugger::Commands::Base::CommandType command = _commandQueue->ProcessCommandsSync();

    if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STOP)
    {
      return DBG_EXCEPTION_NOT_HANDLED;
    }

    // CMD_STEP_INTO / CMD_STEP_OVER: Trap Flag doesn't work on system breakpoint.
    if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_CONTINUE)
    {
      DWORD_PTR entryPoint = (DWORD_PTR)processInfo.info.lpStartAddress;
      _breakpointManager->SetBreakpoint(entryPoint);
    }
    else if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_INTO)
    {
      DWORD_PTR entryPoint = (DWORD_PTR)processInfo.info.lpStartAddress;
      _breakpointManager->SetBreakpoint(entryPoint);
    }
    else if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_OVER)
    {
      DWORD_PTR entryPoint = (DWORD_PTR)processInfo.info.lpStartAddress;
      _breakpointManager->SetBreakpoint(entryPoint);
    }

    // OS will handle the initial system breakpoint and run to our new breakpoint (if set) or continue running
    return DBG_EXCEPTION_NOT_HANDLED;
  }

  DWORD ExceptionEventHandler::HandleUserBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    DWORD_PTR address = (DWORD_PTR)exception.ExceptionRecord.ExceptionAddress;
    DWORD_PTR entryPoint = (DWORD_PTR)processInfo.info.lpStartAddress;
    bool isEntryPoint = (address == entryPoint);

    if (isEntryPoint)
    {
      // The breakpoint on the entry point reports this one stop only
      _breakpointManager->RemoveBreakpoint(address);
    }

    // Takes the breakpoint instruction out of memory before the stop is reported: the listener of the event
    // reads the debuggee's memory
    _breakpointManager->HoldBreakpointAt(address);
    RewindInstructionPointer(threadInfo.info.hThread, exception.ExceptionRecord.ExceptionAddress);

    if (isEntryPoint)
    {
      _eventHandler->OnEntryPointBreakpoint(CreateBreakpointEventArgs(exception, processInfo, threadInfo));
    }
    else
    {
      _eventHandler->OnBreakpoint(CreateBreakpointEventArgs(exception, processInfo, threadInfo));
    }

    return ResumeFromStop(exception, processInfo, threadInfo);
  }

  DWORD ExceptionEventHandler::ResumeFromStop(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    MCP::Native::Debugger::Commands::Base::CommandType command = _commandQueue->ProcessCommandsSync();
    if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STOP)
    {
      return DBG_CONTINUE; // Main loop will exit
    }

    DWORD_PTR stopAddress = (DWORD_PTR)exception.ExceptionRecord.ExceptionAddress;

    _stepOverBreakpointAddress = 0;
    if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_OVER)
    {
      size_t instrLen = 0;
      if (IsCallInstruction(processInfo.info.hProcess, stopAddress, instrLen))
      {
        // Step over a CALL: run until the instruction behind it
        _stepOverBreakpointAddress = stopAddress + instrLen;
        _breakpointManager->SetBreakpoint(_stepOverBreakpointAddress);
      }
    }

    // Asked now and not at the stop: breakpoints may have been set and removed while the debuggee was stopped
    if (_breakpointManager->HasBreakpoint(stopAddress))
    {
      // The instruction at the stop address has to run before its breakpoint instruction is written:
      // execute it with one single step; afterwards HandleStepOffBreakpoint acts on the go command
      _breakpointManager->SetTrapFlag(threadInfo.info.hThread, true);
      _stepOffPending = true;
      _postBreakpointAction = command;
      return DBG_CONTINUE;
    }

    // No breakpoint at the stop address: nothing has to be written once the debuggee has left it
    _breakpointManager->ReleaseHeldBreakpoint();

    bool isSingleStep = command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_INTO
      || (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_OVER && _stepOverBreakpointAddress == 0);
    if (isSingleStep)
    {
      _breakpointManager->SetTrapFlag(threadInfo.info.hThread, true);
    }

    return DBG_CONTINUE;
  }

  DWORD ExceptionEventHandler::HandleStepOffBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    // The debuggee has left the stop address: a breakpoint that is still set there is written now
    _stepOffPending = false;
    _breakpointManager->ReleaseHeldBreakpoint();

    MCP::Native::Debugger::Commands::Base::CommandType command = _postBreakpointAction;
    _postBreakpointAction = MCP::Native::Debugger::Commands::Base::CommandType::CMD_NONE;

    if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_CONTINUE)
    {
      return DBG_CONTINUE;
    }
    else if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_INTO)
    {
      return HandleStepHalt(exception, processInfo, threadInfo);
    }
    else if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_OVER)
    {
      // If _stepOverBreakpointAddress is set, it means we just Single Stepped into a function (because it was a CALL), but we have a breakpoint waiting 
      // at the return address.

      if (_stepOverBreakpointAddress != 0)
      {
        // We are inside the function now. Continue running until we hit the _stepOverBreakpointAddress.
        return DBG_CONTINUE;
      }

      // Otherwise, it was a normal instruction (MOV, ADD, etc), so the step is done.
      return HandleStepHalt(exception, processInfo, threadInfo);
    }

    return DBG_CONTINUE;
  }

  DWORD ExceptionEventHandler::HandleStepHalt(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    // A breakpoint at the address the step ended at must not show in the memory the listener of the event
    // reads, and must not stop the debuggee before the instruction there has run
    _breakpointManager->HoldBreakpointAt((DWORD_PTR)exception.ExceptionRecord.ExceptionAddress);

    _eventHandler->OnSingleStep(CreateBreakpointEventArgs(exception, processInfo, threadInfo));

    return ResumeFromStop(exception, processInfo, threadInfo);
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

  bool ExceptionEventHandler::IsCallInstruction(HANDLE processHandle, DWORD_PTR currentIp, size_t& instrLen)
  {
    return _disassembler->IsCallInstruction(processHandle, currentIp, instrLen);
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

