#pragma once

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

  public ref class DebuggerService
  {
  public:
    DebuggerService(IDispatcher^ dispatcher);
    /*DebuggerService(System::Threading::SynchronizationContext^ synchronizationContext);*/
    ~DebuggerService(); // Dispose: Clean up managed resources
    !DebuggerService(); // Finalizer: Clean up unmanaged resources

    event System::EventHandler<MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^>^ ExitProcess;
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SystemBreakpoint;
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ EntryPointBreakpoint;
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ Breakpoint;
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SingleStep;

    // Debugger methods
    bool Start(String^ path, String^% error, System::UInt32% processId, System::UInt32% threadid, System::IntPtr% imageBase, System::IntPtr% entryPoint, MCP::Debugger::Types::Interop::CONTEXT% context);
    void Stop();
    bool IsRunning();
    void Continue();
    bool StepInto();
    bool StepOver();
    void SetBreakpoint(System::IntPtr va);
    void RemoveBreakpoint(System::IntPtr va);
    array<System::Byte>^ ReadMemory(System::IntPtr va, System::UInt32 size);
    bool WriteMemory(System::IntPtr va, const array<System::Byte>^ data);
    IEnumerable<MCP::Debugger::Types::Disassembler::Instruction^>^ Disassemble(System::IntPtr va, System::UInt32 length);
    System::String^ TryResolveString(System::IntPtr va);

    // Module information
    IEnumerable<MCP::Debugger::Types::Modules::Module^>^ GetModules(System::UInt32 processId);
    IEnumerable<MCP::Debugger::Types::Modules::ModuleImport^>^ GetModuleImports(System::UInt32 processId, System::IntPtr moduleHandle);
    IEnumerable<MCP::Debugger::Types::Modules::ModuleExport^>^ GetModuleExports(System::UInt32 processId, System::IntPtr moduleHandle);
    IEnumerable<MCP::Debugger::Types::Modules::ModuleSection^>^ GetModuleSections(System::UInt32 processId, System::IntPtr moduleHandle);
    
    // Standard API
    System::IntPtr GetRemoteModuleHandle(System::UInt32 processId, String^ moduleName);
    System::IntPtr GetRemoteProcAddress(System::UInt32 processId, System::IntPtr moduleHandle, String^ procedureName);

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
