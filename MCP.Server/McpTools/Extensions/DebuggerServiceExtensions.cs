using MCP.Debugger;
using MCP.Debugger.Types.Disassembler;
using MCP.Debugger.Types.EventArgs;
using MCP.Debugger.Types.Interop;
using ModelContextProtocol;

namespace MCP.Server.McpTools.Extensions
{
  public static class DebuggerServiceExtensions
  {
    private static readonly TimeSpan DefaultTimeout = TimeSpan.FromSeconds(50);

    /// <summary>Reads the instruction pointer of a thread context of this build's bitness.</summary>
    /// <param name="debuggerService">The debugger service; not used by the method.</param>
    /// <param name="context">The thread context of a stop.</param>
    /// <returns>The address of the next instruction to execute.</returns>
    public static nint GetInstructionPointer(this IDebuggerService debuggerService, CONTEXT context)
    {
#if X64
      return (nint)context.Rip;
#else
      return (nint)context.Eip;
#endif
    }

    /// <summary>Continues the debuggee and waits until it stopped at its entry point.</summary>
    /// <param name="debuggerService">The debugger service with a running session.</param>
    /// <returns>The thread context of the entry-point stop.</returns>
    /// <exception cref="McpException">No session is running, or the stop did not arrive in time.</exception>
    public static Task<CONTEXT> ContinueToEntrypointAsync(this IDebuggerService debuggerService)
    {
      return debuggerService.RunAndWaitAsync(subscribe: h => debuggerService.EntryPointBreakpoint += h,
                                             unsubscribe: h => debuggerService.EntryPointBreakpoint -= h,
                                             start: debuggerService.Continue,
                                             predicate: _ => true,
                                             timeoutMessage: "A timeout occurred before the entry point was reached.");
    }

    /// <summary>
    /// Sets a temporary breakpoint, continues the debuggee and waits until it stopped there. The breakpoint is
    /// removed again, also when the wait fails.
    /// </summary>
    /// <param name="debuggerService">The debugger service with a running session.</param>
    /// <param name="va">Absolute virtual address of the instruction to stop at.</param>
    /// <returns>The thread context of the breakpoint stop.</returns>
    /// <exception cref="McpException">
    /// No session is running, <paramref name="va"/> is zero, or the stop did not arrive in time.
    /// </exception>
    public static Task<CONTEXT> ContinueToBreakpointAsync(this IDebuggerService debuggerService, nint va)
    {
      EnsureRunning(debuggerService);

      if (va == nint.Zero)
        throw new McpException("The given absolute virtual address is zero.");

      debuggerService.SetBreakpoint(va);

      return debuggerService.RunAndWaitAsync(subscribe: h => debuggerService.Breakpoint += h,
                                             unsubscribe: h =>
                                             {
                                               debuggerService.Breakpoint -= h;
                                               debuggerService.RemoveBreakpoint(va);
                                             },
                                             start: debuggerService.Continue,
                                             predicate: ctx => debuggerService.GetInstructionPointer(ctx) == va,
                                             timeoutMessage: "A timeout occurred before the specified breakpoint was reached.");
    }

    /// <summary>Executes one instruction, entering a call, and waits for the stop after it.</summary>
    /// <param name="debuggerService">The debugger service with a running session.</param>
    /// <returns>The thread context of the stop after the step.</returns>
    /// <exception cref="McpException">No session is running, or the stop did not arrive in time.</exception>
    public static Task<CONTEXT> StepIntoAsync(this IDebuggerService debuggerService)
    {
      return debuggerService.RunSingleStepAsync(debuggerService.StepInto,
                                                "A timeout occurred before step-into finished.");
    }

    /// <summary>Executes one instruction, running through a call, and waits for the stop after it.</summary>
    /// <param name="debuggerService">The debugger service with a running session.</param>
    /// <returns>The thread context of the stop after the step.</returns>
    /// <exception cref="McpException">No session is running, or the stop did not arrive in time.</exception>
    public static Task<CONTEXT> StepOverAsync(this IDebuggerService debuggerService)
    {
      return debuggerService.RunSingleStepAsync(debuggerService.StepOver,
                                                "A timeout occurred before step-over finished.");
    }

    /// <summary>Disassembles the instruction at an address.</summary>
    /// <param name="debuggerService">The debugger service.</param>
    /// <param name="va">Absolute virtual address of the instruction.</param>
    /// <returns>The instruction; null when nothing could be disassembled there.</returns>
    public static Instruction Disassemble(this IDebuggerService debuggerService, nint va)
    {
      return debuggerService.Disassemble(va, 16)?.FirstOrDefault();
    }

    /// <summary>Issues a step command and waits for the single-step stop it leads to.</summary>
    /// <param name="debuggerService">The debugger service with a running session.</param>
    /// <param name="stepAction">Issues the step command; its result is ignored.</param>
    /// <param name="timeoutMessage">Error text when the stop does not arrive in time.</param>
    /// <returns>The thread context of the stop after the step.</returns>
    /// <exception cref="McpException">No session is running, or the stop did not arrive in time.</exception>
    private static async Task<CONTEXT> RunSingleStepAsync(this IDebuggerService debuggerService,
                                                          Func<bool> stepAction,
                                                          string timeoutMessage)
    {
      return await debuggerService.RunAndWaitAsync(subscribe: h => debuggerService.SingleStep += h,
                                                   unsubscribe: h => debuggerService.SingleStep -= h,
                                                   start: () => _ = stepAction(), // ignore bool intentionally
                                                   predicate: _ => true,
                                                   timeoutMessage: timeoutMessage
      );
    }

    /// <summary>
    /// Subscribes to a stop event, issues the command that lets the debuggee run and waits for the first stop
    /// the predicate accepts. The handler is unsubscribed again on every path.
    /// </summary>
    /// <param name="debuggerService">The debugger service with a running session.</param>
    /// <param name="subscribe">Adds the handler to the stop event to wait for.</param>
    /// <param name="unsubscribe">Removes the handler from that event; may be called more than once.</param>
    /// <param name="start">Issues the command that lets the debuggee run.</param>
    /// <param name="predicate">Decides from the thread context of a stop whether it is the awaited one.</param>
    /// <param name="timeoutMessage">Error text when no accepted stop arrives in time.</param>
    /// <returns>The thread context of the accepted stop.</returns>
    /// <exception cref="McpException">No session is running, or no accepted stop arrived in time.</exception>
    private static async Task<CONTEXT> RunAndWaitAsync(this IDebuggerService debuggerService,
                                                       Action<EventHandler<BreakpointEventArgs>> subscribe,
                                                       Action<EventHandler<BreakpointEventArgs>> unsubscribe,
                                                       Action start,
                                                       Func<CONTEXT, bool> predicate,
                                                       string timeoutMessage)
    {
      EnsureRunning(debuggerService);

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

        var completed = await Task.WhenAny(tcs.Task, Task.Delay(DefaultTimeout));
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
    /// <param name="debuggerService">The debugger service.</param>
    /// <exception cref="McpException">No session is running.</exception>
    private static void EnsureRunning(IDebuggerService debuggerService)
    {
      if (!debuggerService.IsRunning())
        throw new McpException("Debugger has not been started yet.");
    }
  }
}
