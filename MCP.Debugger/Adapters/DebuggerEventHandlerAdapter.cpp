#include "DebuggerEventHandlerAdapter.h"
#include "..\Native\Debugger\Interfaces\BreakpointEventArgs.h"
#include "..\Native\Debugger\Interfaces\ExitProcessEventArgs.h"
#include "..\Managed\DebuggerService.h"

using namespace System::Runtime::InteropServices;
namespace MCP::Adapters
{
  DebuggerEventHandlerAdapter::DebuggerEventHandlerAdapter(MCP::Debugger::DebuggerService^ managedHandler)
  {
    _managedHandler = managedHandler;
  }

  void DebuggerEventHandlerAdapter::OnExitProcess(std::shared_ptr<MCP::Native::Debugger::Interfaces::ExitProcessEventArgs> args)
  {
    System::UInt32 processId = args->ProcessId;
    auto managedArgs = gcnew MCP::Debugger::Types::EventArgs::ExitProcessEventArgs(processId);
    _managedHandler->OnExitProcess(managedArgs);
  }

  void DebuggerEventHandlerAdapter::OnSystemBreakpoint(std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args)
  {
    auto managedArgs = CreateBreakpointEventArgs(args->ProcessId, args->ThreadId, args->Context);
    _managedHandler->OnSystemBreakpoint(managedArgs);
  }

  void DebuggerEventHandlerAdapter::OnEntryPointBreakpoint(std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args)
  {
    auto managedArgs = CreateBreakpointEventArgs(args->ProcessId, args->ThreadId, args->Context);
    _managedHandler->OnEntryPointBreakpoint(managedArgs);
  }

  void DebuggerEventHandlerAdapter::OnBreakpoint(std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args)
  {
    auto managedArgs = CreateBreakpointEventArgs(args->ProcessId, args->ThreadId, args->Context);
    _managedHandler->OnBreakpoint(managedArgs);
  }

  void DebuggerEventHandlerAdapter::OnSingleStep(std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args)
  {
    auto managedArgs = CreateBreakpointEventArgs(args->ProcessId, args->ThreadId, args->Context);
    _managedHandler->OnSingleStep(managedArgs);
  }

  MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ DebuggerEventHandlerAdapter::CreateBreakpointEventArgs(DWORD processId, DWORD threadId, const ::CONTEXT& context)
  {
    IntPtr buffer = Marshal::AllocHGlobal(sizeof(::CONTEXT));
    try
    {
      // Copy native context into unmanaged buffer
      memcpy_s(buffer.ToPointer(), sizeof(::CONTEXT), &context, sizeof(::CONTEXT));
      // Marshal unmanaged memory into managed CONTEXT struct
      auto managedContext = safe_cast<MCP::Debugger::Types::Interop::CONTEXT>(Marshal::PtrToStructure(buffer, MCP::Debugger::Types::Interop::CONTEXT::typeid));

      return gcnew MCP::Debugger::Types::EventArgs::BreakpointEventArgs(static_cast<UInt32>(processId), static_cast<UInt32>(threadId), managedContext);
    }
    finally
    {
      Marshal::FreeHGlobal(buffer);
    }
  }
}