#pragma once

#include ".\Interfaces\IDebuggerService.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading;

namespace MCP::Native::Debugger::Interfaces
{
  class IDebugger;
}

namespace MCP::Debugger
{
  interface class IDispatcher;
  interface class IDebuggerEventSource;
  interface class INativeDebuggerHost;
}

namespace MCP::Debugger
{
  // Native helpers
  bool NativeContinue(MCP::Native::Debugger::Interfaces::IDebugger& debugger);
  bool NativeStepInto(MCP::Native::Debugger::Interfaces::IDebugger& debugger);
  bool NativeStepOver(MCP::Native::Debugger::Interfaces::IDebugger& debugger);
}

namespace MCP::Debugger
{
  // When passing managed parameters to native methods/classes, ensure correct pinning/alignment for structures.
  // When possible, use pin_ptr to pin managed objects in memory.
  // If pinning is not possbile, create a deep copy by using marshaling functions and aligned memory allocation.

  public ref class DebuggerService : public IDebuggerService
  {
  internal:
    /// <summary>
    /// Created by the registration code of this assembly only: the collaborators are types a consumer of the
    /// assembly cannot see. The service keeps all three alive and owns none of them.
    /// </summary>
    /// <param name="dispatcher">Decides the thread the handlers of the service's events run on.</param>
    /// <param name="host">Gives access to the native debugger every call is passed to.</param>
    /// <param name="events">The source of the debugger events the service passes on to its own subscribers.</param>
    DebuggerService(IDispatcher^ dispatcher, INativeDebuggerHost^ host, IDebuggerEventSource^ events);

  public:
    /// <summary>
    /// Dispose: stops listening to the debugger events and shuts the dispatcher down, so no handler runs any
    /// more. The native debugger is ended by its owner, not here.
    /// </summary>
    ~DebuggerService();

    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^>^ ExitProcess;
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SystemBreakpoint;
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ EntryPointBreakpoint;
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ Breakpoint;
    virtual event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SingleStep;

    // Debugger methods
    virtual bool Start(String^ path, String^% error, System::UInt32% processId, System::UInt32% threadid, System::IntPtr% imageBase, System::IntPtr% entryPoint, MCP::Debugger::Types::Interop::CONTEXT% context);
    virtual void Stop();
    virtual bool IsRunning();
    virtual bool Continue();
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

  private:
    // Handlers of the debugger event source. They run on the debugger thread and only hand the event to the
    // dispatcher; the event of the service is raised wherever the dispatcher runs it.
    void OnExitProcess(System::Object^ sender, MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^ args);
    void OnSystemBreakpoint(System::Object^ sender, MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void OnEntryPointBreakpoint(System::Object^ sender, MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void OnBreakpoint(System::Object^ sender, MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void OnSingleStep(System::Object^ sender, MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);

    MCP::Debugger::Types::Interop::CONTEXT GetThreadContext(HANDLE threadHandle);
    MCP::Debugger::Types::Interop::CONTEXT ConvertThreadContext(::CONTEXT& ncontext);

    void RaiseExitProcessEvent(MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^ args);
    void RaiseSystemBreakpointEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void RaiseEntryPointBreakpointEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void RaiseBreakpointEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);
    void RaiseSingleStepEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args);

  private:
    MCP::Debugger::IDispatcher^ _dispatcher;
    MCP::Debugger::INativeDebuggerHost^ _host;
    MCP::Debugger::IDebuggerEventSource^ _events;
  };
}
