using MCP.Debugger.IntegrationTests.TestHelpers;

namespace MCP.Debugger.IntegrationTests.Managed
{
  /// <summary>
  /// Starting and stopping a debugging session through <see cref="DebuggerService"/>. Every test owns its
  /// session because each one changes the session state.
  /// </summary>
  public class DebuggerServiceStartStopTests
  {
    /// <summary>Opcode of the breakpoint instruction (<c>int3</c>).</summary>
    private const byte BreakpointOpcode = 0xCC;

    [Test]
    public async Task Start_SampleTarget_StopsAtSystemBreakpoint()
    {
      // Arrange
      using var session = new DebugSession();

      // Act
      bool isStarted = await session.TryStartAsync(SampleTarget.ExecutablePath);

      // Assert
      Assert.That(isStarted, Is.True, session.StartError);
      await session.WaitForSystemBreakpointAsync();
      nint instructionPointer = DebugSession.InstructionPointer(session.StartContext);
      byte[] byteBeforeInstructionPointer = session.Service.ReadMemory(instructionPointer - 1, 1);
      Assert.That(session.StartError, Is.Empty);
      Assert.That(session.ProcessId, Is.Not.Zero);
      Assert.That(session.ThreadId, Is.Not.Zero);
      Assert.That(session.ImageBase, Is.Not.EqualTo((nint)0));
      Assert.That(session.EntryPoint, Is.Not.EqualTo((nint)0));
      Assert.That(session.Debuggee.HasExited, Is.False);
      Assert.That(session.Debuggee.MainModule.FileName, Is.EqualTo(SampleTarget.ExecutablePath).IgnoreCase);
      // The system breakpoint is a breakpoint instruction the debuggee has just executed.
      Assert.That(byteBeforeInstructionPointer, Is.EqualTo(new[] { BreakpointOpcode }));
      Assert.That(session.SystemBreakpointCount, Is.EqualTo(1));
      Assert.That(session.Service.IsRunning(), Is.True);
    }

    [Test]
    public async Task Start_MissingFile_FailsWithErrorText()
    {
      // Arrange
      using var session = new DebugSession();

      // Act
      bool isStarted = await session.TryStartAsync(SampleTarget.MissingExecutablePath);

      // Assert
      AssertStartFailedWithoutSession(session, isStarted);
    }

    [Test]
    public async Task Start_WrongBitnessFile_FailsWithErrorText()
    {
      // Arrange
      using var session = new DebugSession();

      // Act
      bool isStarted = await session.TryStartAsync(SampleTarget.OtherBitnessExecutablePath);

      // Assert
      AssertStartFailedWithoutSession(session, isStarted);
    }

    [Test]
    public async Task Stop_RunningSession_EndsDebuggee()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAsync();

      // Act
      session.Service.Stop();
      await session.WaitForDebuggeeExitAsync();

      // Assert
      Assert.That(session.Service.IsRunning(), Is.False);
      Assert.That(session.Debuggee.HasExited, Is.True);
    }

    /// <summary>Asserts that a start was refused with an error text and that no session exists.</summary>
    /// <param name="session">The session the start was tried on.</param>
    /// <param name="isStarted">The result of the start.</param>
    private static void AssertStartFailedWithoutSession(DebugSession session, bool isStarted)
    {
      Assert.That(isStarted, Is.False);
      Assert.That(session.StartError, Is.Not.Empty);
      Assert.That(session.ProcessId, Is.Zero);
      Assert.That(session.Service.IsRunning(), Is.False);
    }
  }
}
