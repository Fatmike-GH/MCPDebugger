#include <Windows.h>
#include "ExceptionEventHandler.h"
#include "..\..\Threading\EventWaiter.h"
#include "..\Interfaces\IDebuggerEventHandler.h"
#include "..\Interfaces\BreakpointEventArgs.h"
#include "..\BreakpointManager.h"
#include "..\BreakPoint.h"
#include "..\DebuggerCommandQueue.h"
#include "..\DebuggerThread.h"
#include "..\..\Disassembler\Disassembler.h"

namespace MCP::Native::Debugger::DebugEventHandler
{
  ExceptionEventHandler::ExceptionEventHandler(DebugEventStorage* debugEventStorage,
                                               MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                                               MCP::Native::Debugger::BreakpointManager* breakpointManager,
                                               MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* eventHandler,
                                               MCP::Native::Threading::EventWaiter* startupWaiter,
                                               MCP::Native::Debugger::DebuggerCommandQueue* commandQueue)
  {
    _processExplorer = processExplorer;
    _debugEventStorage = debugEventStorage;
    _breakpointManager = breakpointManager;
    _eventHandler = eventHandler;
    _startupWaiter = startupWaiter;
    _commandQueue = commandQueue;
    
    _systemBreakpoint = true;
    _lastBreakpoint = nullptr;
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
    Breakpoint* breakpoint = _breakpointManager->GetBreakpoint((DWORD_PTR)address);
    if (breakpoint != nullptr)
    {
      return HandleUserBreakpoint(exception, processInfo, threadInfo, breakpoint);
    }

    // Unknown breakpoint (e.g. hardcoded int 3)
    return DBG_EXCEPTION_NOT_HANDLED;
  }

  DWORD ExceptionEventHandler::HandleSingleStepException(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    // Just stepped one instruction off a breakpoint using the trap flag, to be able to enable it again
    if (_lastBreakpoint != nullptr)
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

  DWORD ExceptionEventHandler::HandleUserBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo, Breakpoint* breakpoint)
  {
    breakpoint->Disable();
    RewindInstructionPointer(threadInfo.info.hThread, exception.ExceptionRecord.ExceptionAddress);

    DWORD_PTR entryPoint = (DWORD_PTR)processInfo.info.lpStartAddress;
    if (entryPoint == breakpoint->GetVa())
    {
      _eventHandler->OnEntryPointBreakpoint(CreateBreakpointEventArgs(exception, processInfo, threadInfo));
    }
    else
    {
      _eventHandler->OnBreakpoint(CreateBreakpointEventArgs(exception, processInfo, threadInfo));
    }

    MCP::Native::Debugger::Commands::Base::CommandType command = _commandQueue->ProcessCommandsSync();
    if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STOP)
    {
      return DBG_CONTINUE;
    }

    _stepOverBreakpointAddress = 0;

    if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_OVER)
    {
      DWORD_PTR currentIp = (DWORD_PTR)exception.ExceptionRecord.ExceptionAddress;
      size_t instrLen = 0;

      if (IsCallInstruction(processInfo.info.hProcess, currentIp, instrLen))
      {
        _stepOverBreakpointAddress = currentIp + instrLen;
        _breakpointManager->SetBreakpoint(_stepOverBreakpointAddress);
      }
    }
    else if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_INTO)
    {
      // Explicitly do nothing here. 
    }

    _breakpointManager->SetTrapFlag(threadInfo.info.hThread, true);
    _lastBreakpoint = breakpoint;
    _postBreakpointAction = command;

    // Continue to execute the one instruction; afterwards HandleStepOffBreakpoint
    return DBG_CONTINUE;
  }

  DWORD ExceptionEventHandler::HandleStepOffBreakpoint(EXCEPTION_DEBUG_INFO& exception, ProcessInfo& processInfo, ThreadInfo& threadInfo)
  {
    DWORD_PTR entryPoint = (DWORD_PTR)processInfo.info.lpStartAddress;
    if (_lastBreakpoint != nullptr && _lastBreakpoint->GetVa() != entryPoint) // Only user defined breakpoints shall be enabled again
    {
      _lastBreakpoint->Enable();
    }    
    _lastBreakpoint = nullptr;

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
    _eventHandler->OnSingleStep(CreateBreakpointEventArgs(exception, processInfo, threadInfo));

    MCP::Native::Debugger::Commands::Base::CommandType command = _commandQueue->ProcessCommandsSync();

    if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STOP)
    {
      return DBG_CONTINUE; // Main loop will exit
    }
    else if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_CONTINUE)
    {
      return DBG_CONTINUE;
    }
    else if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_INTO)
    {
      _breakpointManager->SetTrapFlag(threadInfo.info.hThread, true);
    }
    else if (command == MCP::Native::Debugger::Commands::Base::CommandType::CMD_STEP_OVER)
    {
      DWORD_PTR currentIp = (DWORD_PTR)exception.ExceptionRecord.ExceptionAddress;
      size_t instrLen = 0;
      _stepOverBreakpointAddress = 0;
      if (IsCallInstruction(processInfo.info.hProcess, currentIp, instrLen))
      {
        DWORD_PTR nextAddr = currentIp + instrLen;
        _stepOverBreakpointAddress = nextAddr;
        _breakpointManager->SetBreakpoint(nextAddr);
        return DBG_CONTINUE;
      }
      else
      {
        _breakpointManager->SetTrapFlag(threadInfo.info.hThread, true);
      }
    }
    return DBG_CONTINUE;
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
    MCP::Native::Disassembler::Disassembler disassembler(_processExplorer);
    return disassembler.IsCallInstruction(processHandle, currentIp, instrLen);
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

