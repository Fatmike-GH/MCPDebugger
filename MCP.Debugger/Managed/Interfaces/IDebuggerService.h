#pragma once

namespace MCP::Debugger
{
  /// <summary>
  /// Debugs one native Windows executable at a time: execution control, breakpoints, memory, disassembly and
  /// module information of the debuggee.
  /// </summary>
  /// <remarks>
  /// <para>
  /// Threading. A debugger thread owns the debuggee. <see cref="Start"/> creates that thread and blocks until
  /// the first stop. Every other member reaches it in one of two ways, named in its description:
  /// "queued" - the call is handed to the debugger thread and blocks the caller until that thread ran it; when the
  /// debuggee is running, it is interrupted for the call;
  /// "direct" - the call reads the debugger's own bookkeeping on the calling thread and does not interrupt the
  /// debuggee.
  /// </para>
  /// <para>
  /// Events. The service never raises an event itself: it hands it to the <see cref="IDispatcher"/> it was
  /// created with, and the dispatcher decides the thread a handler runs on. A dispatcher that passes the event
  /// to another thread keeps handlers off the debugger thread, and only then may a handler call the service. A
  /// dispatcher that runs the handler right away runs it on the debugger thread, where a queued call would wait
  /// for the thread it is made on.
  /// </para>
  /// <para>
  /// Failures. A failed operation does not throw. Each member reports failure through its return value: false,
  /// nullptr, an empty list, an empty string or a zero address, as its description says. Members without a return
  /// value do nothing when no debuggee is being debugged.
  /// </para>
  /// <para>
  /// Lifetime. The service belongs to whoever created it (normally the dependency-injection container). A
  /// consumer of this interface must not dispose it.
  /// </para>
  /// </remarks>
  public interface class IDebuggerService
  {
  public:
    /// <summary>The debuggee process has ended.</summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^>^ ExitProcess;

    /// <summary>
    /// The debuggee stopped at the system breakpoint, the first stop of every session. Raised once per session.
    /// It may arrive after <see cref="Start"/> returned or while it is returning: do not expect it earlier.
    /// </summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SystemBreakpoint;

    /// <summary>The debuggee stopped at its entry point.</summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ EntryPointBreakpoint;

    /// <summary>The debuggee stopped at a breakpoint set with <see cref="SetBreakpoint"/>.</summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ Breakpoint;

    /// <summary>The debuggee stopped after a step started with <see cref="StepInto"/> or <see cref="StepOver"/>.</summary>
    event System::EventHandler<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>^ SingleStep;

    /// <summary>
    /// Starts the executable under the debugger. Blocks, without a timeout, until the debuggee stopped at the
    /// system breakpoint or could not be started. The executable must be a native image of this build's bitness.
    /// </summary>
    /// <param name="path">Full path of the executable.</param>
    /// <param name="error">Receives the reason when the start failed; unchanged otherwise.</param>
    /// <param name="processId">Receives the process id of the debuggee; unchanged when the start failed.</param>
    /// <param name="threadid">Receives the id of the debuggee's main thread; unchanged when the start failed.</param>
    /// <param name="imageBase">Receives the image base of the debuggee; unchanged when the start failed.</param>
    /// <param name="entryPoint">Receives the entry point of the debuggee; unchanged when the start failed.</param>
    /// <param name="context">Receives the main thread's context at the system breakpoint; unchanged when the start failed.</param>
    /// <returns>
    /// True when the debuggee runs under the debugger; false with <paramref name="error"/> set when a session is
    /// already running, the file is not a matching executable or the process could not be created.
    /// </returns>
    bool Start(System::String^ path, System::String^% error, System::UInt32% processId, System::UInt32% threadid, System::IntPtr% imageBase, System::IntPtr% entryPoint, MCP::Debugger::Types::Interop::CONTEXT% context);

    /// <summary>
    /// Queued. Ends the session and the debuggee process. Returns after the debugger thread ended. Does nothing
    /// when no session is running.
    /// </summary>
    void Stop();

    /// <summary>Direct. Tells whether a session is running.</summary>
    /// <returns>True while a session started with <see cref="Start"/> is running.</returns>
    bool IsRunning();

    /// <summary>
    /// Queued. Lets the stopped debuggee run on. The next stop is reported through an event. Does nothing when no
    /// session is running.
    /// </summary>
    void Continue();

    /// <summary>
    /// Queued. Executes one instruction and enters a call. The stop after the step is reported through
    /// <see cref="SingleStep"/>.
    /// </summary>
    /// <returns>False when no session is running or the step could not be started.</returns>
    bool StepInto();

    /// <summary>
    /// Queued. Executes one instruction and runs through a call. The stop after the step is reported through
    /// <see cref="SingleStep"/>.
    /// </summary>
    /// <returns>False when no session is running or the step could not be started.</returns>
    bool StepOver();

    /// <summary>
    /// Queued. Sets a breakpoint. A failure is not reported; nothing happens when no session is running.
    /// </summary>
    /// <param name="va">Absolute virtual address of the instruction to stop at.</param>
    void SetBreakpoint(System::IntPtr va);

    /// <summary>
    /// Queued. Removes a breakpoint. A failure is not reported; nothing happens when no session is running.
    /// </summary>
    /// <param name="va">Absolute virtual address the breakpoint was set at.</param>
    void RemoveBreakpoint(System::IntPtr va);

    /// <summary>Queued. Reads memory of the debuggee.</summary>
    /// <param name="va">Absolute virtual address of the first byte.</param>
    /// <param name="size">Number of bytes to read.</param>
    /// <returns>
    /// The bytes; nullptr when no session is running, <paramref name="size"/> is 0 or the range could not be read
    /// completely.
    /// </returns>
    array<System::Byte>^ ReadMemory(System::IntPtr va, System::UInt32 size);

    /// <summary>Queued. Writes memory of the debuggee.</summary>
    /// <param name="va">Absolute virtual address of the first byte.</param>
    /// <param name="data">The bytes to write; at least one.</param>
    /// <returns>False when no session is running or the range could not be written completely.</returns>
    bool WriteMemory(System::IntPtr va, const array<System::Byte>^ data);

    /// <summary>Queued. Disassembles memory of the debuggee.</summary>
    /// <param name="va">Absolute virtual address of the first instruction.</param>
    /// <param name="length">Number of bytes to disassemble.</param>
    /// <returns>The instructions in address order; an empty list when no session is running or nothing could be decoded.</returns>
    System::Collections::Generic::IEnumerable<MCP::Debugger::Types::Disassembler::Instruction^>^ Disassemble(System::IntPtr va, System::UInt32 length);

    /// <summary>Queued. Reads a string from the memory of the debuggee.</summary>
    /// <param name="va">Absolute virtual address where a string is expected.</param>
    /// <returns>The string; an empty string when no session is running or no string was found.</returns>
    System::String^ TryResolveString(System::IntPtr va);

    /// <summary>Direct. Lists the modules currently loaded in a debuggee process.</summary>
    /// <param name="processId">Process id of the debuggee.</param>
    /// <returns>The modules; an empty list when the process is not known.</returns>
    System::Collections::Generic::IEnumerable<MCP::Debugger::Types::Modules::Module^>^ GetModules(System::UInt32 processId);

    /// <summary>Direct. Lists the imports of a module of a debuggee process.</summary>
    /// <param name="processId">Process id of the debuggee.</param>
    /// <param name="moduleHandle">Module handle, i.e. the base address of the module in the debuggee.</param>
    /// <returns>The imports per imported module; an empty list when the process or the module is not known.</returns>
    System::Collections::Generic::IEnumerable<MCP::Debugger::Types::Modules::ModuleImport^>^ GetModuleImports(System::UInt32 processId, System::IntPtr moduleHandle);

    /// <summary>Direct. Lists the exports of a module of a debuggee process.</summary>
    /// <param name="processId">Process id of the debuggee.</param>
    /// <param name="moduleHandle">Module handle, i.e. the base address of the module in the debuggee.</param>
    /// <returns>The exports; an empty list when the process or the module is not known.</returns>
    System::Collections::Generic::IEnumerable<MCP::Debugger::Types::Modules::ModuleExport^>^ GetModuleExports(System::UInt32 processId, System::IntPtr moduleHandle);

    /// <summary>Direct. Lists the sections of a module of a debuggee process.</summary>
    /// <param name="processId">Process id of the debuggee.</param>
    /// <param name="moduleHandle">Module handle, i.e. the base address of the module in the debuggee.</param>
    /// <returns>The sections; an empty list when the process or the module is not known.</returns>
    System::Collections::Generic::IEnumerable<MCP::Debugger::Types::Modules::ModuleSection^>^ GetModuleSections(System::UInt32 processId, System::IntPtr moduleHandle);

    /// <summary>Direct. Finds a module of a debuggee process by its file name.</summary>
    /// <param name="processId">Process id of the debuggee.</param>
    /// <param name="moduleName">File name of the module, for example kernel32.dll.</param>
    /// <returns>The module handle (the base address of the module in the debuggee); zero when it was not found.</returns>
    System::IntPtr GetRemoteModuleHandle(System::UInt32 processId, System::String^ moduleName);

    /// <summary>Direct. Finds an exported function of a module of a debuggee process.</summary>
    /// <param name="processId">Process id of the debuggee.</param>
    /// <param name="moduleHandle">Module handle as returned by <see cref="GetRemoteModuleHandle"/>.</param>
    /// <param name="procedureName">Name of the exported function.</param>
    /// <returns>The absolute virtual address of the function; zero when it was not found.</returns>
    System::IntPtr GetRemoteProcAddress(System::UInt32 processId, System::IntPtr moduleHandle, System::String^ procedureName);
  };
}
