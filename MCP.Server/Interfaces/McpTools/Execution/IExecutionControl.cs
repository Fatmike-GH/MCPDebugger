using MCP.Debugger.Types.Disassembler;
using MCP.Debugger.Types.Interop;
using ModelContextProtocol;

namespace MCP.Server.Interfaces.McpTools.Execution
{
  /// <summary>
  /// Lets the debuggee run to its next stop and waits for that stop, and reads from a stop what the tools
  /// report about it.
  /// </summary>
  /// <remarks>
  /// <para>
  /// Waiting. A waiting member subscribes to the stop event it expects, issues the command that lets the
  /// debuggee run and returns the thread context of the first stop it accepts. It gives up when that stop does
  /// not arrive within a fixed time limit. Its subscription is removed again on every path.
  /// </para>
  /// <para>
  /// Running debuggee. A debuggee can only be resumed or stepped from a stop. The members that need one fail
  /// at once, without waiting, when the debugger refuses their command because the debuggee runs already.
  /// <see cref="ContinueToBreakpointAsync"/> is the way back to a stop from a running debuggee and does not
  /// fail then.
  /// </para>
  /// <para>
  /// Threading. Every member may be called from any thread. Only one waiting call may be in progress at a time;
  /// this is not checked. The awaited stop is handled on the thread the debugger service raises its events on;
  /// the returned task completes on a thread-pool thread.
  /// </para>
  /// </remarks>
  public interface IExecutionControl
  {
    /// <summary>Reads the instruction pointer of a thread context of this build's bitness.</summary>
    /// <param name="context">The thread context of a stop.</param>
    /// <returns>The address of the next instruction to execute.</returns>
    nint GetInstructionPointer(CONTEXT context);

    /// <summary>Lets the stopped debuggee run on without waiting for its next stop.</summary>
    /// <exception cref="McpException">
    /// No session is running, or the debuggee is not stopped. Both are thrown by the call itself.
    /// </exception>
    void Continue();

    /// <summary>Continues the debuggee and waits until it stopped at its entry point.</summary>
    /// <returns>The thread context of the entry-point stop.</returns>
    /// <exception cref="McpException">
    /// No session is running, the debuggee is not stopped, or the stop did not arrive in time. All are reported
    /// through the returned task.
    /// </exception>
    Task<CONTEXT> ContinueToEntryPointAsync();

    /// <summary>
    /// Sets a temporary breakpoint, continues the debuggee and waits until it stopped there. Stops at other
    /// addresses are ignored. The breakpoint is removed again, also when the wait fails. A debuggee that runs
    /// already is not an error: the breakpoint is set and the wait begins without resuming anything.
    /// </summary>
    /// <param name="va">Absolute virtual address of the instruction to stop at.</param>
    /// <returns>The thread context of the breakpoint stop.</returns>
    /// <exception cref="McpException">
    /// No session is running or <paramref name="va"/> is zero: thrown by the call itself. The stop did not arrive
    /// in time: reported through the returned task.
    /// </exception>
    Task<CONTEXT> ContinueToBreakpointAsync(nint va);

    /// <summary>Executes one instruction, entering a call, and waits for the stop after it.</summary>
    /// <returns>The thread context of the stop after the step.</returns>
    /// <exception cref="McpException">
    /// No session is running, the debuggee is not stopped, or the stop did not arrive in time. All are reported
    /// through the returned task.
    /// </exception>
    Task<CONTEXT> StepIntoAsync();

    /// <summary>Executes one instruction, running through a call, and waits for the stop after it.</summary>
    /// <returns>The thread context of the stop after the step.</returns>
    /// <exception cref="McpException">
    /// No session is running, the debuggee is not stopped, or the stop did not arrive in time. All are reported
    /// through the returned task.
    /// </exception>
    Task<CONTEXT> StepOverAsync();

    /// <summary>Disassembles the instruction at an address.</summary>
    /// <param name="va">Absolute virtual address of the instruction.</param>
    /// <returns>The instruction; null when nothing could be disassembled there.</returns>
    Instruction DisassembleInstruction(nint va);
  }
}
