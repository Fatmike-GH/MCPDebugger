using MCP.Debugger;
using MCP.Debugger.Types.Disassembler;
using MCP.Debugger.Types.EventArgs;
using MCP.Debugger.Types.Interop;
using MCP.Server.Interfaces.McpTools.Execution;
using MCP.Server.McpTools.Execution;
using MCP.Server.Tests.TestHelpers;
using ModelContextProtocol;
using Moq;

namespace MCP.Server.Tests.McpTools.Execution
{
  /// <summary>
  /// The waiting operations on a mocked debugger service: which stop ends a wait, what happens when the time
  /// limit runs out, and that the subscription to the stop event is removed on every path. No test waits for
  /// time to pass: the wait for the time limit is a mock that either never ends or has ended already.
  /// </summary>
  public class ExecutionControlTests
  {
    /// <summary>Error text of a waiting operation without a running session.</summary>
    private const string NotStartedText = "Debugger has not been started yet.";

    /// <summary>Error text of an operation that needs a stopped debuggee while the debuggee runs.</summary>
    private const string DebuggeeRunningText =
      "The debuggee is running. This command is only possible while it is stopped.";

    /// <summary>The address the breakpoint tests run to.</summary>
    private const nint BreakpointAddress = 0x401000;

    /// <summary>A flags value that marks the thread context of the stop a test raises.</summary>
    private const uint StopFlags = 0x246;

    /// <summary>The time limit of a waiting operation.</summary>
    private static readonly TimeSpan TimeLimit = TimeSpan.FromSeconds(50);

    /// <summary>
    /// Upper bound for a task that the tested code is expected to complete. It only matters when the code is
    /// broken: the test then fails instead of waiting forever.
    /// </summary>
    private static readonly TimeSpan CompletionSafetyNet = TimeSpan.FromSeconds(30);

    /// <summary>
    /// The debugger service; a session is running and the debuggee is stopped - the service accepts every
    /// command that resumes it - unless a test says otherwise.
    /// </summary>
    private Mock<IDebuggerService> _service;

    /// <summary>The wait for the time limit; it never ends unless a test says otherwise.</summary>
    private Mock<IDelay> _delay;

    /// <summary>The execution control under test.</summary>
    private ExecutionControl _executionControl;

    /// <summary>The handler the tested code added to the entry-point event; null when it added none.</summary>
    private EventHandler<BreakpointEventArgs> _entryPointHandler;

    /// <summary>The handler the tested code added to the breakpoint event; null when it added none.</summary>
    private EventHandler<BreakpointEventArgs> _breakpointHandler;

    /// <summary>The handler the tested code added to the single-step event; null when it added none.</summary>
    private EventHandler<BreakpointEventArgs> _singleStepHandler;

    /// <summary>Creates the mocks and the execution control for a test.</summary>
    [SetUp]
    public void CreateExecutionControl()
    {
      _entryPointHandler = null;
      _breakpointHandler = null;
      _singleStepHandler = null;

      _service = new Mock<IDebuggerService>();
      _service.Setup(service => service.IsRunning()).Returns(true);
      // Without these the mock would answer false, which is the service's answer for a debuggee that runs.
      _service.Setup(service => service.Continue()).Returns(true);
      _service.Setup(service => service.StepInto()).Returns(true);
      _service.Setup(service => service.StepOver()).Returns(true);
      _service.SetupAdd(service => service.EntryPointBreakpoint += It.IsAny<EventHandler<BreakpointEventArgs>>())
              .Callback<EventHandler<BreakpointEventArgs>>(handler => _entryPointHandler = handler);
      _service.SetupAdd(service => service.Breakpoint += It.IsAny<EventHandler<BreakpointEventArgs>>())
              .Callback<EventHandler<BreakpointEventArgs>>(handler => _breakpointHandler = handler);
      _service.SetupAdd(service => service.SingleStep += It.IsAny<EventHandler<BreakpointEventArgs>>())
              .Callback<EventHandler<BreakpointEventArgs>>(handler => _singleStepHandler = handler);

      _delay = new Mock<IDelay>();
      _delay.Setup(delay => delay.WaitAsync(It.IsAny<TimeSpan>())).Returns(new TaskCompletionSource().Task);

      _executionControl = new ExecutionControl(_service.Object, _delay.Object);
    }

    [Test]
    public void Constructor_NullDebuggerService_ThrowsArgumentNullException()
    {
      // Act
      ArgumentNullException caught = Assert.Throws<ArgumentNullException>(
        () => new ExecutionControl(null, _delay.Object));

      // Assert
      Assert.That(caught.ParamName, Is.EqualTo("debuggerService"));
    }

    [Test]
    public void Constructor_NullDelay_ThrowsArgumentNullException()
    {
      // Act
      ArgumentNullException caught = Assert.Throws<ArgumentNullException>(
        () => new ExecutionControl(_service.Object, null));

      // Assert
      Assert.That(caught.ParamName, Is.EqualTo("delay"));
    }

    [Test]
    public void GetInstructionPointer_ContextOfThisBitness_ReturnsItsInstructionPointer()
    {
      // Arrange
      CONTEXT context = TestContexts.AtInstructionPointer(BreakpointAddress);

      // Act
      nint instructionPointer = _executionControl.GetInstructionPointer(context);

      // Assert
      Assert.That(instructionPointer, Is.EqualTo(BreakpointAddress));
    }

    [Test]
    public void DisassembleInstruction_SeveralInstructions_ReturnsTheFirst()
    {
      // Arrange
      var first = new Instruction(BreakpointAddress, 1, "push ebp");
      var second = new Instruction(BreakpointAddress + 1, 2, "mov ebp, esp");
      _service.Setup(service => service.Disassemble(BreakpointAddress, 16))
              .Returns(new List<Instruction> { first, second });

      // Act
      Instruction instruction = _executionControl.DisassembleInstruction(BreakpointAddress);

      // Assert
      Assert.That(instruction, Is.SameAs(first));
    }

    [Test]
    public void DisassembleInstruction_NoInstruction_ReturnsNull()
    {
      // Arrange
      _service.Setup(service => service.Disassemble(BreakpointAddress, 16)).Returns(new List<Instruction>());

      // Act
      Instruction instruction = _executionControl.DisassembleInstruction(BreakpointAddress);

      // Assert
      Assert.That(instruction, Is.Null);
    }

    [Test]
    public void DisassembleInstruction_NullList_ReturnsNull()
    {
      // Arrange
      _service.Setup(service => service.Disassemble(BreakpointAddress, 16)).Returns((IEnumerable<Instruction>)null);

      // Act
      Instruction instruction = _executionControl.DisassembleInstruction(BreakpointAddress);

      // Assert
      Assert.That(instruction, Is.Null);
    }

    [Test]
    public void Continue_DebuggeeStopped_ContinuesTheService()
    {
      // Act
      _executionControl.Continue();

      // Assert
      _service.Verify(service => service.Continue(), Times.Once());
    }

    [Test]
    public void Continue_DebuggeeRunning_ThrowsRunningText()
    {
      // Arrange
      _service.Setup(service => service.Continue()).Returns(false);

      // Act
      McpException caught = Assert.Throws<McpException>(() => _executionControl.Continue());

      // Assert
      Assert.That(caught.Message, Is.EqualTo(DebuggeeRunningText));
    }

    [Test]
    public void Continue_NotRunning_ThrowsNotStartedText()
    {
      // Arrange
      _service.Setup(service => service.IsRunning()).Returns(false);

      // Act
      McpException caught = Assert.Throws<McpException>(() => _executionControl.Continue());

      // Assert
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
      _service.Verify(service => service.Continue(), Times.Never());
    }

    [Test]
    public async Task ContinueToEntryPointAsync_AwaitedStopRaised_ReturnsItsContext()
    {
      // Arrange
      var stop = new CONTEXT { EFlags = StopFlags };

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToEntryPointAsync();
      RaiseEntryPointStop(stop);
      CONTEXT context = await Completed(pending);

      // Assert
      Assert.That(context.EFlags, Is.EqualTo(StopFlags));
      _service.Verify(service => service.Continue(), Times.Once());
      _service.VerifyRemove(service => service.EntryPointBreakpoint -= _entryPointHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task ContinueToEntryPointAsync_DelayElapsed_ThrowsItsTimeoutText()
    {
      // Arrange
      LetTheTimeLimitRunOut();

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToEntryPointAsync();

      // Assert
      Assert.That(pending.IsFaulted, Is.True, "The call must have failed without waiting.");
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo("A timeout occurred before the entry point was reached."));
      _delay.Verify(delay => delay.WaitAsync(TimeLimit), Times.Once());
      _service.VerifyRemove(service => service.EntryPointBreakpoint -= _entryPointHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task ContinueToEntryPointAsync_DebuggeeRunning_ThrowsRunningTextWithoutWaiting()
    {
      // Arrange
      _service.Setup(service => service.Continue()).Returns(false);

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToEntryPointAsync();

      // Assert
      Assert.That(pending.IsFaulted, Is.True, "The call must have failed without waiting.");
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo(DebuggeeRunningText));
      _delay.Verify(delay => delay.WaitAsync(It.IsAny<TimeSpan>()), Times.Never());
      Assert.That(_entryPointHandler, Is.Not.Null);
      _service.VerifyRemove(service => service.EntryPointBreakpoint -= _entryPointHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task ContinueToEntryPointAsync_CommandThrows_UnsubscribesHandler()
    {
      // Arrange
      _service.Setup(service => service.Continue()).Throws(new InvalidOperationException("boom"));

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToEntryPointAsync();

      // Assert
      await Assert.ThrowsAsync<InvalidOperationException>(async () => await pending);
      Assert.That(_entryPointHandler, Is.Not.Null);
      _service.VerifyRemove(service => service.EntryPointBreakpoint -= _entryPointHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task ContinueToEntryPointAsync_NotRunning_ThrowsNotStartedText()
    {
      // Arrange
      _service.Setup(service => service.IsRunning()).Returns(false);

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToEntryPointAsync();

      // Assert
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
      Assert.That(_entryPointHandler, Is.Null);
      _service.Verify(service => service.Continue(), Times.Never());
    }

    [Test]
    public async Task ContinueToBreakpointAsync_AwaitedStopRaised_ReturnsItsContext()
    {
      // Arrange
      CONTEXT stop = TestContexts.AtInstructionPointer(BreakpointAddress);
      stop.EFlags = StopFlags;

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToBreakpointAsync(BreakpointAddress);
      RaiseBreakpointStop(stop);
      CONTEXT context = await Completed(pending);

      // Assert
      Assert.That(context.EFlags, Is.EqualTo(StopFlags));
      _service.Verify(service => service.SetBreakpoint(BreakpointAddress), Times.Once());
      _service.Verify(service => service.Continue(), Times.Once());
      _service.VerifyRemove(service => service.Breakpoint -= _breakpointHandler, Times.AtLeastOnce());
      _service.Verify(service => service.RemoveBreakpoint(BreakpointAddress), Times.AtLeastOnce());
    }

    [Test]
    public async Task ContinueToBreakpointAsync_StopAtOtherAddress_KeepsWaiting()
    {
      // Arrange
      CONTEXT otherStop = TestContexts.AtInstructionPointer(BreakpointAddress + 0x1000);
      CONTEXT awaitedStop = TestContexts.AtInstructionPointer(BreakpointAddress);
      awaitedStop.EFlags = StopFlags;

      // Act: a stop somewhere else
      Task<CONTEXT> pending = _executionControl.ContinueToBreakpointAsync(BreakpointAddress);
      RaiseBreakpointStop(otherStop);

      // Assert
      Assert.That(pending.IsCompleted, Is.False);
      _service.VerifyRemove(service => service.Breakpoint -= It.IsAny<EventHandler<BreakpointEventArgs>>(), Times.Never());
      _service.Verify(service => service.RemoveBreakpoint(It.IsAny<nint>()), Times.Never());

      // Act: the stop at the breakpoint
      RaiseBreakpointStop(awaitedStop);
      CONTEXT context = await Completed(pending);

      // Assert
      Assert.That(context.EFlags, Is.EqualTo(StopFlags));
    }

    [Test]
    public async Task ContinueToBreakpointAsync_DelayElapsed_ThrowsItsTimeoutText()
    {
      // Arrange
      LetTheTimeLimitRunOut();

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToBreakpointAsync(BreakpointAddress);

      // Assert
      Assert.That(pending.IsFaulted, Is.True, "The call must have failed without waiting.");
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo("A timeout occurred before the specified breakpoint was reached."));
      _delay.Verify(delay => delay.WaitAsync(TimeLimit), Times.Once());
      _service.VerifyRemove(service => service.Breakpoint -= _breakpointHandler, Times.AtLeastOnce());
      _service.Verify(service => service.RemoveBreakpoint(BreakpointAddress), Times.AtLeastOnce());
    }

    [Test]
    public async Task ContinueToBreakpointAsync_DebuggeeRunningAndStopRaised_ReturnsItsContext()
    {
      // Arrange
      // A debuggee that runs already reaches the breakpoint without being resumed.
      _service.Setup(service => service.Continue()).Returns(false);
      CONTEXT stop = TestContexts.AtInstructionPointer(BreakpointAddress);
      stop.EFlags = StopFlags;

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToBreakpointAsync(BreakpointAddress);
      bool isWaiting = !pending.IsCompleted;
      RaiseBreakpointStop(stop);
      CONTEXT context = await Completed(pending);

      // Assert
      Assert.That(isWaiting, Is.True, "The refused continue must not end the wait.");
      Assert.That(context.EFlags, Is.EqualTo(StopFlags));
      _service.Verify(service => service.SetBreakpoint(BreakpointAddress), Times.Once());
      _service.VerifyRemove(service => service.Breakpoint -= _breakpointHandler, Times.AtLeastOnce());
      _service.Verify(service => service.RemoveBreakpoint(BreakpointAddress), Times.AtLeastOnce());
    }

    [Test]
    public async Task ContinueToBreakpointAsync_DebuggeeRunningAndDelayElapsed_ThrowsItsTimeoutText()
    {
      // Arrange
      _service.Setup(service => service.Continue()).Returns(false);
      LetTheTimeLimitRunOut();

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToBreakpointAsync(BreakpointAddress);

      // Assert
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo("A timeout occurred before the specified breakpoint was reached."));
      _service.Verify(service => service.SetBreakpoint(BreakpointAddress), Times.Once());
      _delay.Verify(delay => delay.WaitAsync(TimeLimit), Times.Once());
      _service.VerifyRemove(service => service.Breakpoint -= _breakpointHandler, Times.AtLeastOnce());
      _service.Verify(service => service.RemoveBreakpoint(BreakpointAddress), Times.AtLeastOnce());
    }

    [Test]
    public async Task ContinueToBreakpointAsync_CommandThrows_UnsubscribesHandlerAndRemovesBreakpoint()
    {
      // Arrange
      _service.Setup(service => service.Continue()).Throws(new InvalidOperationException("boom"));

      // Act
      Task<CONTEXT> pending = _executionControl.ContinueToBreakpointAsync(BreakpointAddress);

      // Assert
      await Assert.ThrowsAsync<InvalidOperationException>(async () => await pending);
      Assert.That(_breakpointHandler, Is.Not.Null);
      _service.VerifyRemove(service => service.Breakpoint -= _breakpointHandler, Times.AtLeastOnce());
      _service.Verify(service => service.RemoveBreakpoint(BreakpointAddress), Times.AtLeastOnce());
    }

    [Test]
    public void ContinueToBreakpointAsync_NotRunning_ThrowsNotStartedText()
    {
      // Arrange
      _service.Setup(service => service.IsRunning()).Returns(false);

      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _executionControl.ContinueToBreakpointAsync(BreakpointAddress));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
      Assert.That(_breakpointHandler, Is.Null);
      _service.Verify(service => service.SetBreakpoint(It.IsAny<nint>()), Times.Never());
      _service.Verify(service => service.Continue(), Times.Never());
    }

    [Test]
    public void ContinueToBreakpointAsync_ZeroAddress_ThrowsZeroAddressText()
    {
      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _executionControl.ContinueToBreakpointAsync(nint.Zero));

      // Assert
      Assert.That(caught.Message, Is.EqualTo("The given absolute virtual address is zero."));
      Assert.That(_breakpointHandler, Is.Null);
      _service.Verify(service => service.SetBreakpoint(It.IsAny<nint>()), Times.Never());
      _service.Verify(service => service.Continue(), Times.Never());
    }

    [Test]
    public async Task StepIntoAsync_AwaitedStopRaised_ReturnsItsContext()
    {
      // Arrange
      var stop = new CONTEXT { EFlags = StopFlags };

      // Act
      Task<CONTEXT> pending = _executionControl.StepIntoAsync();
      RaiseSingleStepStop(stop);
      CONTEXT context = await Completed(pending);

      // Assert
      Assert.That(context.EFlags, Is.EqualTo(StopFlags));
      _service.Verify(service => service.StepInto(), Times.Once());
      _service.Verify(service => service.StepOver(), Times.Never());
      _service.VerifyRemove(service => service.SingleStep -= _singleStepHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task StepIntoAsync_AwaitedStopRaised_CallsInOrderSubscribeCommandWaitUnsubscribe()
    {
      // Arrange
      var calls = new List<string>();
      _service.SetupAdd(service => service.SingleStep += It.IsAny<EventHandler<BreakpointEventArgs>>())
              .Callback(() => calls.Add("subscribe"));
      _service.SetupRemove(service => service.SingleStep -= It.IsAny<EventHandler<BreakpointEventArgs>>())
              .Callback(() => calls.Add("unsubscribe"));
      _service.Setup(service => service.StepInto()).Callback(() => calls.Add("command")).Returns(true);
      _delay.Setup(delay => delay.WaitAsync(It.IsAny<TimeSpan>()))
            .Callback(() => calls.Add("wait"))
            .Returns(new TaskCompletionSource().Task);

      // Act
      Task<CONTEXT> pending = _executionControl.StepIntoAsync();
      RaiseSingleStepStop(new CONTEXT());
      await Completed(pending);

      // Assert
      Assert.That(calls.Take(4), Is.EqualTo(new[] { "subscribe", "command", "wait", "unsubscribe" }));
    }

    [Test]
    public async Task StepIntoAsync_DelayElapsed_ThrowsItsTimeoutText()
    {
      // Arrange
      LetTheTimeLimitRunOut();

      // Act
      Task<CONTEXT> pending = _executionControl.StepIntoAsync();

      // Assert
      Assert.That(pending.IsFaulted, Is.True, "The call must have failed without waiting.");
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo("A timeout occurred before step-into finished."));
      _delay.Verify(delay => delay.WaitAsync(TimeLimit), Times.Once());
      _service.VerifyRemove(service => service.SingleStep -= _singleStepHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task StepIntoAsync_DebuggeeRunning_ThrowsRunningTextWithoutWaiting()
    {
      // Arrange
      _service.Setup(service => service.StepInto()).Returns(false);

      // Act
      Task<CONTEXT> pending = _executionControl.StepIntoAsync();

      // Assert
      Assert.That(pending.IsFaulted, Is.True, "The call must have failed without waiting.");
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo(DebuggeeRunningText));
      _delay.Verify(delay => delay.WaitAsync(It.IsAny<TimeSpan>()), Times.Never());
      Assert.That(_singleStepHandler, Is.Not.Null);
      _service.VerifyRemove(service => service.SingleStep -= _singleStepHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task StepIntoAsync_CommandThrows_UnsubscribesHandler()
    {
      // Arrange
      _service.Setup(service => service.StepInto()).Throws(new InvalidOperationException("boom"));

      // Act
      Task<CONTEXT> pending = _executionControl.StepIntoAsync();

      // Assert
      await Assert.ThrowsAsync<InvalidOperationException>(async () => await pending);
      Assert.That(_singleStepHandler, Is.Not.Null);
      _service.VerifyRemove(service => service.SingleStep -= _singleStepHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task StepIntoAsync_NotRunning_ThrowsNotStartedText()
    {
      // Arrange
      _service.Setup(service => service.IsRunning()).Returns(false);

      // Act
      Task<CONTEXT> pending = _executionControl.StepIntoAsync();

      // Assert
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
      Assert.That(_singleStepHandler, Is.Null);
      _service.Verify(service => service.StepInto(), Times.Never());
    }

    [Test]
    public async Task StepOverAsync_AwaitedStopRaised_ReturnsItsContext()
    {
      // Arrange
      var stop = new CONTEXT { EFlags = StopFlags };

      // Act
      Task<CONTEXT> pending = _executionControl.StepOverAsync();
      RaiseSingleStepStop(stop);
      CONTEXT context = await Completed(pending);

      // Assert
      Assert.That(context.EFlags, Is.EqualTo(StopFlags));
      _service.Verify(service => service.StepOver(), Times.Once());
      _service.Verify(service => service.StepInto(), Times.Never());
      _service.VerifyRemove(service => service.SingleStep -= _singleStepHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task StepOverAsync_DelayElapsed_ThrowsItsTimeoutText()
    {
      // Arrange
      LetTheTimeLimitRunOut();

      // Act
      Task<CONTEXT> pending = _executionControl.StepOverAsync();

      // Assert
      Assert.That(pending.IsFaulted, Is.True, "The call must have failed without waiting.");
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo("A timeout occurred before step-over finished."));
      _delay.Verify(delay => delay.WaitAsync(TimeLimit), Times.Once());
      _service.VerifyRemove(service => service.SingleStep -= _singleStepHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task StepOverAsync_DebuggeeRunning_ThrowsRunningTextWithoutWaiting()
    {
      // Arrange
      _service.Setup(service => service.StepOver()).Returns(false);

      // Act
      Task<CONTEXT> pending = _executionControl.StepOverAsync();

      // Assert
      Assert.That(pending.IsFaulted, Is.True, "The call must have failed without waiting.");
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo(DebuggeeRunningText));
      _delay.Verify(delay => delay.WaitAsync(It.IsAny<TimeSpan>()), Times.Never());
      Assert.That(_singleStepHandler, Is.Not.Null);
      _service.VerifyRemove(service => service.SingleStep -= _singleStepHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task StepOverAsync_CommandThrows_UnsubscribesHandler()
    {
      // Arrange
      _service.Setup(service => service.StepOver()).Throws(new InvalidOperationException("boom"));

      // Act
      Task<CONTEXT> pending = _executionControl.StepOverAsync();

      // Assert
      await Assert.ThrowsAsync<InvalidOperationException>(async () => await pending);
      Assert.That(_singleStepHandler, Is.Not.Null);
      _service.VerifyRemove(service => service.SingleStep -= _singleStepHandler, Times.AtLeastOnce());
    }

    [Test]
    public async Task StepOverAsync_NotRunning_ThrowsNotStartedText()
    {
      // Arrange
      _service.Setup(service => service.IsRunning()).Returns(false);

      // Act
      Task<CONTEXT> pending = _executionControl.StepOverAsync();

      // Assert
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await pending);
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
      Assert.That(_singleStepHandler, Is.Null);
      _service.Verify(service => service.StepOver(), Times.Never());
    }

    /// <summary>Makes the wait for the time limit end at once, as if the limit had already run out.</summary>
    private void LetTheTimeLimitRunOut()
    {
      _delay.Setup(delay => delay.WaitAsync(It.IsAny<TimeSpan>())).Returns(Task.CompletedTask);
    }

    /// <summary>Raises the entry-point event of the debugger service.</summary>
    /// <param name="context">The thread context of the stop.</param>
    private void RaiseEntryPointStop(CONTEXT context)
    {
      _service.Raise(service => service.EntryPointBreakpoint += null, _service.Object, StopWith(context));
    }

    /// <summary>Raises the breakpoint event of the debugger service.</summary>
    /// <param name="context">The thread context of the stop.</param>
    private void RaiseBreakpointStop(CONTEXT context)
    {
      _service.Raise(service => service.Breakpoint += null, _service.Object, StopWith(context));
    }

    /// <summary>Raises the single-step event of the debugger service.</summary>
    /// <param name="context">The thread context of the stop.</param>
    private void RaiseSingleStepStop(CONTEXT context)
    {
      _service.Raise(service => service.SingleStep += null, _service.Object, StopWith(context));
    }

    /// <summary>Creates the event data of a stop.</summary>
    /// <param name="context">The thread context of the stop.</param>
    /// <returns>The event data.</returns>
    private static BreakpointEventArgs StopWith(CONTEXT context)
    {
      return new BreakpointEventArgs(4711, 12, context);
    }

    /// <summary>
    /// Awaits a task the tested code is expected to complete. When it does not, the test fails after the
    /// safety net instead of waiting forever.
    /// </summary>
    /// <param name="pending">The task of a waiting operation.</param>
    /// <returns>The thread context the operation returned.</returns>
    private static Task<CONTEXT> Completed(Task<CONTEXT> pending)
    {
      return pending.WaitAsync(CompletionSafetyNet);
    }
  }
}
