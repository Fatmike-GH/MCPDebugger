using System.ComponentModel;
using System.Diagnostics;
using MCP.Debugger.Types.EventArgs;
using MCP.Debugger.Types.Interop;
using Microsoft.Extensions.DependencyInjection;

namespace MCP.Debugger.IntegrationTests.TestHelpers
{
  /// <summary>
  /// One debugging session of the sample target, driven through <see cref="IDebuggerService"/>.
  /// Every wait is bounded by <see cref="WaitTimeout"/>. Disposing ends the session and makes sure the debuggee
  /// process is gone, so a failed test leaves nothing behind (one exception: a start that never returns, see
  /// <see cref="Dispose"/>).
  /// </summary>
  /// <remarks>
  /// The service raises its events on its dispatcher thread, which swallows exceptions. The handlers of this
  /// class therefore only record that a stop happened; assertions belong into the test body.
  /// A go command (continue, step) may only be issued while the debuggee is stopped.
  /// </remarks>
  public sealed class DebugSession : IDisposable
  {
    /// <summary>
    /// Upper bound of every single wait (start, a stop event, exit of the debuggee). A healthy session needs
    /// about half a second to start and a few milliseconds per stop; the bound only ends a hanging test.
    /// </summary>
    public static readonly TimeSpan WaitTimeout = TimeSpan.FromSeconds(15);

    /// <summary>
    /// The container that created the service, set up the way an executable sets it up. It owns the service:
    /// disposing it disposes the service.
    /// </summary>
    private readonly ServiceProvider _serviceProvider;

    /// <summary>The service under test.</summary>
    private readonly IDebuggerService _service;

    /// <summary>Completed with the context of the first system breakpoint event.</summary>
    private readonly TaskCompletionSource<CONTEXT> _systemBreakpoint =
      new TaskCompletionSource<CONTEXT>(TaskCreationOptions.RunContinuationsAsynchronously);

    /// <summary>Number of system breakpoint events raised so far.</summary>
    private int _systemBreakpointCount;

    /// <summary>Number of single-step events raised so far.</summary>
    private int _singleStepCount;

    /// <summary>The debuggee process; null until a start succeeded.</summary>
    private Process _debuggee;

    /// <summary>The last start call, running or finished; null before the first start.</summary>
    private Task<bool> _start;

    /// <summary>True once <see cref="Dispose"/> ran.</summary>
    private bool _isDisposed;

    /// <summary>
    /// Creates the service the way an executable obtains it: from a container that holds a console dispatcher
    /// and the registrations of the debugger assembly. No debuggee is started yet.
    /// </summary>
    public DebugSession()
    {
      var serviceCollection = new ServiceCollection();
      serviceCollection.AddSingleton<IDispatcher, ConsoleDispatcher>();
      serviceCollection.AddDebuggerService();
      _serviceProvider = serviceCollection.BuildServiceProvider();

      _service = _serviceProvider.GetRequiredService<IDebuggerService>();
      _service.SystemBreakpoint += OnSystemBreakpoint;
      _service.SingleStep += OnSingleStep;
    }

    /// <summary>The service under test, for the calls a test makes itself.</summary>
    public IDebuggerService Service => _service;

    /// <summary>The debuggee process; null until a start succeeded.</summary>
    public Process Debuggee => _debuggee;

    /// <summary>Error text of the last start; empty when it succeeded.</summary>
    public string StartError { get; private set; } = string.Empty;

    /// <summary>Process id reported by the last start; 0 when it failed.</summary>
    public uint ProcessId { get; private set; }

    /// <summary>Id of the main thread reported by the last start.</summary>
    public uint ThreadId { get; private set; }

    /// <summary>Image base of the debuggee reported by the last start.</summary>
    public nint ImageBase { get; private set; }

    /// <summary>Entry point of the debuggee reported by the last start.</summary>
    public nint EntryPoint { get; private set; }

    /// <summary>Thread context reported by the last start (the debuggee stands at the system breakpoint).</summary>
    public CONTEXT StartContext { get; private set; }

    /// <summary>Number of system breakpoint events raised so far.</summary>
    public int SystemBreakpointCount => Volatile.Read(ref _systemBreakpointCount);

    /// <summary>Number of single-step events raised so far.</summary>
    public int SingleStepCount => Volatile.Read(ref _singleStepCount);

    /// <summary>Reads the instruction pointer of a thread context of this build's bitness.</summary>
    /// <param name="context">The thread context of a stop.</param>
    /// <returns>The address of the next instruction to execute.</returns>
    public static nint InstructionPointer(CONTEXT context)
    {
#if X64
      return unchecked((nint)context.Rip);
#else
      return unchecked((nint)context.Eip);
#endif
    }

    /// <summary>
    /// Starts debugging the given executable and records what the start reported. The start call blocks
    /// without a timeout of its own, so it runs on a pool thread and is bounded here.
    /// </summary>
    /// <param name="path">Full path of the executable to debug.</param>
    /// <returns>True when the debuggee was started; false with <see cref="StartError"/> set otherwise.</returns>
    /// <exception cref="TimeoutException">
    /// The start did not return within <see cref="WaitTimeout"/>. The start call cannot be cancelled and keeps
    /// running; see <see cref="Dispose"/> for what that means for the clean-up.
    /// </exception>
    public async Task<bool> TryStartAsync(string path)
    {
      _start = Task.Run(() => Start(path));
      bool isStarted = await _start.WaitAsync(WaitTimeout);
      if (isStarted)
      {
        _debuggee = Process.GetProcessById((int)ProcessId);
      }

      return isStarted;
    }

    /// <summary>
    /// Starts debugging the sample target and waits until the system breakpoint was reported, which happens
    /// shortly after the start call returned.
    /// </summary>
    /// <returns>A task that completes when the debuggee stands at the system breakpoint.</returns>
    /// <exception cref="InvalidOperationException">The debugger could not start the sample target.</exception>
    /// <exception cref="TimeoutException">A wait exceeded <see cref="WaitTimeout"/>.</exception>
    public async Task StartAsync()
    {
      if (!await TryStartAsync(SampleTarget.ExecutablePath))
      {
        throw new InvalidOperationException($"Starting '{SampleTarget.ExecutablePath}' failed: {StartError}");
      }

      await WaitForSystemBreakpointAsync();
    }

    /// <summary>Starts debugging the sample target and runs it to its entry point.</summary>
    /// <returns>A task that completes when the debuggee stands at its entry point.</returns>
    /// <exception cref="InvalidOperationException">The debugger could not start the sample target.</exception>
    /// <exception cref="TimeoutException">A wait exceeded <see cref="WaitTimeout"/>.</exception>
    public async Task StartAtEntryPointAsync()
    {
      await StartAsync();
      await ContinueToEntryPointAsync();
    }

    /// <summary>Waits for the system breakpoint event of the session.</summary>
    /// <returns>The thread context of the system breakpoint stop.</returns>
    /// <exception cref="TimeoutException">The event was not raised within <see cref="WaitTimeout"/>.</exception>
    public Task<CONTEXT> WaitForSystemBreakpointAsync()
    {
      return _systemBreakpoint.Task.WaitAsync(WaitTimeout);
    }

    /// <summary>Continues from the system breakpoint and waits for the stop at the entry point.</summary>
    /// <returns>The thread context of the entry-point stop.</returns>
    /// <exception cref="TimeoutException">The stop was not reported within <see cref="WaitTimeout"/>.</exception>
    public Task<CONTEXT> ContinueToEntryPointAsync()
    {
      return WaitForStopAsync(
        handler => _service.EntryPointBreakpoint += handler,
        handler => _service.EntryPointBreakpoint -= handler,
        _service.Continue);
    }

    /// <summary>Executes one instruction, entering a call, and waits for the step stop.</summary>
    /// <returns>The thread context of the step stop.</returns>
    /// <exception cref="InvalidOperationException">The debugger refused the command.</exception>
    /// <exception cref="TimeoutException">The stop was not reported within <see cref="WaitTimeout"/>.</exception>
    public Task<CONTEXT> StepIntoAsync()
    {
      return WaitForStopAsync(
        handler => _service.SingleStep += handler,
        handler => _service.SingleStep -= handler,
        IssueStepInto);
    }

    /// <summary>Executes one instruction, running through a call, and waits for the step stop.</summary>
    /// <returns>The thread context of the step stop.</returns>
    /// <exception cref="InvalidOperationException">The debugger refused the command.</exception>
    /// <exception cref="TimeoutException">The stop was not reported within <see cref="WaitTimeout"/>.</exception>
    public Task<CONTEXT> StepOverAsync()
    {
      return WaitForStopAsync(
        handler => _service.SingleStep += handler,
        handler => _service.SingleStep -= handler,
        IssueStepOver);
    }

    /// <summary>Sets a breakpoint, continues and waits for the breakpoint stop.</summary>
    /// <param name="virtualAddress">Address of the instruction to stop at.</param>
    /// <returns>The thread context of the breakpoint stop.</returns>
    /// <exception cref="TimeoutException">The stop was not reported within <see cref="WaitTimeout"/>.</exception>
    public Task<CONTEXT> ContinueToBreakpointAsync(nint virtualAddress)
    {
      _service.SetBreakpoint(virtualAddress);
      return WaitForStopAsync(
        handler => _service.Breakpoint += handler,
        handler => _service.Breakpoint -= handler,
        _service.Continue);
    }

    /// <summary>Waits until the debuggee process has ended.</summary>
    /// <returns>A task that completes when the process has exited.</returns>
    /// <exception cref="TimeoutException">The process still ran after <see cref="WaitTimeout"/>.</exception>
    public Task WaitForDebuggeeExitAsync()
    {
      return _debuggee.WaitForExitAsync().WaitAsync(WaitTimeout);
    }

    /// <summary>
    /// Ends the session: stops the debugger when it runs, disposes the container - and with it the service -
    /// and, should the debuggee have survived that, ends the process.
    /// </summary>
    /// <remarks>
    /// One case is not cleaned up: a start that ran into its timeout and is still blocked inside the service.
    /// Stopping or disposing the service underneath that call is not safe - it could hang or crash the test
    /// host and take the whole run with it - so the container and its service are left alone. A debuggee the
    /// hanging start created lives until the test host ends, and the check at the end of the run reports it.
    /// </remarks>
    public void Dispose()
    {
      if (_isDisposed)
      {
        return;
      }

      _isDisposed = true;
      _service.SystemBreakpoint -= OnSystemBreakpoint;
      _service.SingleStep -= OnSingleStep;
      if (_start != null && !_start.IsCompleted)
      {
        return;
      }

      try
      {
        if (_service.IsRunning())
        {
          _service.Stop();
        }

        _serviceProvider.Dispose();
      }
      finally
      {
        EndDebuggee();
      }
    }

    /// <summary>Calls the blocking start of the service and records its results.</summary>
    /// <param name="path">Full path of the executable to debug.</param>
    /// <returns>The result of the start call.</returns>
    private bool Start(string path)
    {
      string error = string.Empty;
      uint processId = 0;
      uint threadId = 0;
      nint imageBase = 0;
      nint entryPoint = 0;
      var context = new CONTEXT();

      bool isStarted = _service.Start(path, ref error, ref processId, ref threadId, ref imageBase, ref entryPoint, ref context);

      StartError = error;
      ProcessId = processId;
      ThreadId = threadId;
      ImageBase = imageBase;
      EntryPoint = entryPoint;
      StartContext = context;
      return isStarted;
    }

    /// <summary>
    /// Subscribes a one-shot handler to a stop event, issues the command that leads to the stop and waits for
    /// the event.
    /// </summary>
    /// <param name="subscribe">Adds the handler to the stop event the command produces.</param>
    /// <param name="unsubscribe">Removes the handler from that event.</param>
    /// <param name="issueCommand">Issues the go command.</param>
    /// <returns>The thread context of the stop.</returns>
    /// <exception cref="TimeoutException">The stop was not reported within <see cref="WaitTimeout"/>.</exception>
    private static async Task<CONTEXT> WaitForStopAsync(
      Action<EventHandler<BreakpointEventArgs>> subscribe,
      Action<EventHandler<BreakpointEventArgs>> unsubscribe,
      Action issueCommand)
    {
      var stop = new TaskCompletionSource<CONTEXT>(TaskCreationOptions.RunContinuationsAsynchronously);
      EventHandler<BreakpointEventArgs> handler = (sender, e) => stop.TrySetResult(e.Context);
      subscribe(handler);
      try
      {
        issueCommand();
        return await stop.Task.WaitAsync(WaitTimeout);
      }
      finally
      {
        unsubscribe(handler);
      }
    }

    /// <summary>Issues a step-into command.</summary>
    /// <exception cref="InvalidOperationException">The debugger refused the command.</exception>
    private void IssueStepInto()
    {
      if (!_service.StepInto())
      {
        throw new InvalidOperationException("The debugger refused the step-into command.");
      }
    }

    /// <summary>Issues a step-over command.</summary>
    /// <exception cref="InvalidOperationException">The debugger refused the command.</exception>
    private void IssueStepOver()
    {
      if (!_service.StepOver())
      {
        throw new InvalidOperationException("The debugger refused the step-over command.");
      }
    }

    /// <summary>Records a system breakpoint event. Runs on the dispatcher thread and must not throw.</summary>
    /// <param name="sender">The service.</param>
    /// <param name="e">The stop.</param>
    private void OnSystemBreakpoint(object sender, BreakpointEventArgs e)
    {
      Interlocked.Increment(ref _systemBreakpointCount);
      _systemBreakpoint.TrySetResult(e.Context);
    }

    /// <summary>Records a single-step event. Runs on the dispatcher thread and must not throw.</summary>
    /// <param name="sender">The service.</param>
    /// <param name="e">The stop.</param>
    private void OnSingleStep(object sender, BreakpointEventArgs e)
    {
      Interlocked.Increment(ref _singleStepCount);
    }

    /// <summary>
    /// Makes sure the debuggee process is gone: gives it the time to end that stopping the debugger normally
    /// needs, and kills it when it is still there.
    /// </summary>
    private void EndDebuggee()
    {
      if (_debuggee == null)
      {
        return;
      }

      try
      {
        if (!_debuggee.WaitForExit(WaitTimeout))
        {
          _debuggee.Kill();
          _debuggee.WaitForExit(WaitTimeout);
        }
      }
      catch (InvalidOperationException)
      {
        // The process ended between the wait and the kill: nothing is left to end.
      }
      catch (Win32Exception)
      {
        // The process is already terminating and can no longer be opened: nothing is left to end.
      }
      finally
      {
        _debuggee.Dispose();
      }
    }
  }
}
