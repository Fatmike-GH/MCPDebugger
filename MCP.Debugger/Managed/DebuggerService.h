#pragma once

#include ".\Interfaces\IDebuggerService.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading;

namespace MCP::Native::Debugger
{
  class Debugger;
}

namespace MCP::Adapters
{
  class DebuggerEventHandlerAdapter;
}

namespace MCP::Debugger
{
  interface class IDispatcher;
}

namespace MCP::Debugger
{
  // Native helpers
  bool NativeStepInto(MCP::Native::Debugger::Debugger* debugger);
  bool NativeStepOver(MCP::Native::Debugger::Debugger* debugger);
}

namespace MCP::Debugger
{
  // When passing managed parameters to native methods/classes, ensure correct pinning/alignment for structures.
  // When possible, use pin_ptr to pin managed objects in memory.
  // If pinning is not possbile, create a deep copy by using marshaling functions and aligned memory allocation.

  public ref class DebuggerService : public IDebuggerService
  {
  public:
    DebuggerService(IDispatcher^ dispatcher);
    /*DebuggerService(System::Threading::SynchronizationContext^ synchronizationContext);*/
    ~DebuggerService(); // Dispose: Clean up managed resources
    !DebuggerService(); // Finalizer: Clean up unmanaged resources

    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^>^ ExitProcess;
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SystemBreakpoint;
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ EntryPointBreakpoint;
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ Breakpoint;
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SingleStep;

    // Debugger methods
    virtual bool Start(String^ path, String^% error, System::UInt32% processId, System::UInt32% threadid, System::IntPtr% imageBase, System::IntPtr% entryPoint, MCP::Debugger::Types::Interop::CONTEXT% context);
    virtual void Stop();
    virtual bool IsRunning();
    virtual void Continue();
    virtual bool StepInto();
    virtual bool StepOver();
    virtual void SetBreakpoint(System::IntPtr va);
    virtual void RemoveBreakpoint(System::IntPtr va);
    virtual array<System::Byte>^ ReadMemory(System::IntPtr va, System::UInt32 size);
    virtual bool WriteMemory(System::IntPtr va, const array<System::Byte>^ data);
    virtual IEnumerable<MCP::Debugger::Types::Disassembler::Instruction^>^ Disassemble(System::IntPtr va, System::UInt32 length);
    virtual System::String^ TryResolveString(System::IntPtr va);

    // Module information
    virtual IEnumerable<MCP::Debugger::Types::Modules::Module^>^ GetModules(System::UInt32 processId);
    virtual IEnumerable<MCP::Debugger::Types::Modules::ModuleImport^>^ GetModuleImports(System::UInt32 processId, System::IntPtr moduleHandle);
    virtual IEnumerable<MCP::Debugger::Types::Modules::ModuleExport^>^ GetModuleExports(System::UInt32 processId, System::IntPtr moduleHandle);
    virtual IEnumerable<MCP::Debugger::Types::Modules::ModuleSection^>^ GetModuleSections(System::UInt32 processId, System::IntPtr moduleHandle);

    // Standard API
    virtual System::IntPtr GetRemoteModuleHandle(System::UInt32 processId, String^ moduleName);
    virtual System::IntPtr GetRemoteProcAddress(System::UInt32 processId, System::IntPtr moduleHandle, String^ procedureName);

    // Event Handlers are called from native adapter
    void OnExitProcess(MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^ args);
    void OnSystemBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void OnEntryPointBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void OnBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void OnSingleStep(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);

  private:
    MCP::Debugger::Types::Interop::CONTEXT GetThreadContext(HANDLE threadHandle);
    MCP::Debugger::Types::Interop::CONTEXT ConvertThreadContext(::CONTEXT& ncontext);

    void RaiseExitProcessEvent(MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^ args);
    void RaiseSystemBreakpointEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void RaiseEntryPointBreakpointEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void RaiseBreakpointEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void RaiseSingleStepEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);

  private:
    MCP::Adapters::DebuggerEventHandlerAdapter* _adapter;
    MCP::Native::Debugger::Debugger* _debugger;
    MCP::Debugger::IDispatcher^ _dispatcher;
  };
}
