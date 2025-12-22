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

    public static nint GetInstructionPointer(this DebuggerService debuggerService, CONTEXT context)
    {
#if X64
      return (nint)context.Rip;
#else
      return (nint)context.Eip;
#endif
    }

    public static Task<CONTEXT> ContinueToEntrypointAsync(this DebuggerService debuggerService)
    {
      return debuggerService.RunAndWaitAsync(subscribe: h => debuggerService.EntryPointBreakpoint += h,
                                             unsubscribe: h => debuggerService.EntryPointBreakpoint -= h,
                                             start: debuggerService.Continue,
                                             predicate: _ => true,
                                             timeoutMessage: "A timeout occurred before the entry point was reached.");
    }

    public static Task<CONTEXT> ContinueToBreakpointAsync(this DebuggerService debuggerService, nint va)
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

    public static Task<CONTEXT> StepIntoAsync(this DebuggerService debuggerService)
    {
      return debuggerService.RunSingleStepAsync(debuggerService.StepInto,
                                                "A timeout occurred before step-into finished.");
    }

    public static Task<CONTEXT> StepOverAsync(this DebuggerService debuggerService)
    {
      return debuggerService.RunSingleStepAsync(debuggerService.StepOver,
                                                "A timeout occurred before step-over finished.");
    }

    public static Instruction Disassemble(this DebuggerService debuggerService, nint va)
    {
      return debuggerService.Disassemble(va, 16)?.FirstOrDefault();
    }

    private static async Task<CONTEXT> RunSingleStepAsync(this DebuggerService debuggerService,
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

    private static async Task<CONTEXT> RunAndWaitAsync(this DebuggerService debuggerService,
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

    private static void EnsureRunning(DebuggerService debuggerService)
    {
      if (!debuggerService.IsRunning())
        throw new McpException("Debugger has not been started yet.");
    }
  }
}
