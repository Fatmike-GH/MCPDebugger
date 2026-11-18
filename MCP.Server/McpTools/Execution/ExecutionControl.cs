using MCP.Debugger;
using MCP.Debugger.Types.Disassembler;
using MCP.Debugger.Types.EventArgs;
using MCP.Debugger.Types.Interop;
using MCP.Server.Interfaces.McpTools.Execution;
using ModelContextProtocol;

namespace MCP.Server.McpTools.Execution
{
  /// <summary>
  /// Execution control on top of the debugger service: every waiting operation subscribes to its stop event,
  /// issues its command and waits for the stop or for the time limit, whichever comes first.
  /// </summary>
  public class ExecutionControl : IExecutionControl
  {
    /// <summary>Time limit of a waiting operation.</summary>
    private static readonly TimeSpan DefaultTimeout = TimeSpan.FromSeconds(50);

    /// <summary>The debugger that is driven. Owned by the composition root, not by this class.</summary>
    private readonly IDebuggerService _debuggerService;

    /// <summary>Provides the wait that ends a waiting operation when no stop arrives.</summary>
    private readonly IDelay _delay;

    /// <summary>Creates the execution control on top of a debugger service.</summary>
    /// <param name="debuggerService">The debugger that is driven.</param>
    /// <param name="delay">Provides the wait for the time limit of a waiting operation.</param>
    /// <exception cref="ArgumentNullException">An argument is null.</exception>
    public ExecutionControl(IDebuggerService debuggerService, IDelay delay)
    {
      ArgumentNullException.ThrowIfNull(debuggerService);
      ArgumentNullException.ThrowIfNull(delay);

      _debuggerService = debuggerService;
      _delay = delay;
    }

    /// <inheritdoc/>
    public nint GetInstructionPointer(CONTEXT context)
    {
#if X64
      return (nint)context.Rip;
#else
      return (nint)context.Eip;
#endif
    }

    /// <inheritdoc/>
    public Task<CONTEXT> ContinueToEntryPointAsync()
    {
      return RunAndWaitAsync(subscribe: h => _debuggerService.EntryPointBreakpoint += h,
                             unsubscribe: h => _debuggerService.EntryPointBreakpoint -= h,
                             start: _debuggerService.Continue,
                             predicate: _ => true,
                             timeoutMessage: "A timeout occurred before the entry point was reached.");
    }

    /// <inheritdoc/>
    public Task<CONTEXT> ContinueToBreakpointAsync(nint va)
    {
      EnsureRunning();

      if (va == nint.Zero)
        throw new McpException("The given absolute virtual address is zero.");

      _debuggerService.SetBreakpoint(va);

      return RunAndWaitAsync(subscribe: h => _debuggerService.Breakpoint += h,
                             unsubscribe: h =>
                             {
                               _debuggerService.Breakpoint -= h;
                               _debuggerService.RemoveBreakpoint(va);
                             },
                             start: _debuggerService.Continue,
                             predicate: ctx => GetInstructionPointer(ctx) == va,
                             timeoutMessage: "A timeout occurred before the specified breakpoint was reached.");
    }

    /// <inheritdoc/>
    public Task<CONTEXT> StepIntoAsync()
    {
      return RunSingleStepAsync(_debuggerService.StepInto,
                                "A timeout occurred before step-into finished.");
    }

    /// <inheritdoc/>
    public Task<CONTEXT> StepOverAsync()
    {
      return RunSingleStepAsync(_debuggerService.StepOver,
                                "A timeout occurred before step-over finished.");
    }

    /// <inheritdoc/>
    public Instruction DisassembleInstruction(nint va)
    {
      return _debuggerService.Disassemble(va, 16)?.FirstOrDefault();
    }

    /// <summary>Issues a step command and waits for the single-step stop it leads to.</summary>
    /// <param name="stepAction">Issues the step command; its result is ignored.</param>
    /// <param name="timeoutMessage">Error text when the stop does not arrive in time.</param>
    /// <returns>The thread context of the stop after the step.</returns>
    /// <exception cref="McpException">No session is running, or the stop did not arrive in time.</exception>
    private async Task<CONTEXT> RunSingleStepAsync(Func<bool> stepAction, string timeoutMessage)
    {
      return await RunAndWaitAsync(subscribe: h => _debuggerService.SingleStep += h,
                                   unsubscribe: h => _debuggerService.SingleStep -= h,
                                   start: () => _ = stepAction(), // ignore bool intentionally
                                   predicate: _ => true,
                                   timeoutMessage: timeoutMessage
      );
    }

    /// <summary>
    /// Subscribes to a stop event, issues the command that lets the debuggee run and waits for the first stop
    /// the predicate accepts. The handler is unsubscribed again on every path.
    /// </summary>
    /// <param name="subscribe">Adds the handler to the stop event to wait for.</param>
    /// <param name="unsubscribe">Removes the handler from that event; may be called more than once.</param>
    /// <param name="start">Issues the command that lets the debuggee run.</param>
    /// <param name="predicate">Decides from the thread context of a stop whether it is the awaited one.</param>
    /// <param name="timeoutMessage">Error text when no accepted stop arrives in time.</param>
    /// <returns>The thread context of the accepted stop.</returns>
    /// <exception cref="McpException">No session is running, or no accepted stop arrived in time.</exception>
    private async Task<CONTEXT> RunAndWaitAsync(Action<EventHandler<BreakpointEventArgs>> subscribe,
                                                Action<EventHandler<BreakpointEventArgs>> unsubscribe,
                                                Action start,
                                                Func<CONTEXT, bool> predicate,
                                                string timeoutMessage)
    {
      EnsureRunning();

      var tcs = new TaskCompletionSource<CONTEXT>(TaskCreationOptions.RunContinuationsAsynchronously);

      EventHandler<BreakpointEventArgs> handler = null;

      handler = (_, e) =>
      {
        if (!predicate(e.Context))
          return;

        unsubscribe(handler);
        tcs.TrySetResult(e.Context);
      };

      subscribe(handler);

      try
      {
        start();

        var completed = await Task.WhenAny(tcs.Task, _delay.WaitAsync(DefaultTimeout));
        if (completed != tcs.Task)
          throw new McpException(timeoutMessage);

        return await tcs.Task;
      }
      finally
      {
        unsubscribe(handler);
      }
    }

    /// <summary>Rejects a call that needs a running session.</summary>
    /// <exception cref="McpException">No session is running.</exception>
    private void EnsureRunning()
    {
      if (!_debuggerService.IsRunning())
        throw new McpException("Debugger has not been started yet.");
    }
  }
}
