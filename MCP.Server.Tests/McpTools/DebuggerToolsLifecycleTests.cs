using MCP.Debugger.Types.Interop;
using MCP.Server.McpTools.Results;
using MCP.Server.Tests.TestHelpers;
using ModelContextProtocol;
using Moq;

namespace MCP.Server.Tests.McpTools
{
  /// <summary>
  /// The tools that begin, move on and end a session - start, stop, continue, continue to the entry point - and
  /// the end of the debuggee process. What a tool hands to the debugger and gives back, and which session state
  /// follows. The state is observed the way a client observes it: by the text a following tool is refused with.
  /// </summary>
  public class DebuggerToolsLifecycleTests
  {
    /// <summary>Error text of a tool that needs a session when none is running.</summary>
    private const string NotStartedText = "Debugger not yet started.";

    /// <summary>Error text of the start tool while a session is running.</summary>
    private const string AlreadyStartedText = "Debugger already started.";

    /// <summary>Error text of a tool that needs the entry point while the session stands before it.</summary>
    private const string EntryPointNotReachedText = "Entry point not yet reached.";

    /// <summary>Start of the error text of a failed start; the reason the service reports follows.</summary>
    private const string StartFailedPrefix = "Debugger_Start failed. Error: ";

    /// <summary>Start of the error text for an unexpected exception; the message of the exception follows.</summary>
    private const string InternalErrorPrefix = "Unexpected internal MCP server error! Internal exception: ";

    /// <summary>Message of the exception a mock throws as an unexpected failure.</summary>
    private const string FailureMessage = "boom";

    /// <summary>A flags value that marks the thread context of the entry-point stop.</summary>
    private const uint EntryPointFlags = 0x202;

    /// <summary>The tools on mocked collaborators.</summary>
    private DebuggerToolsHarness _harness;

    /// <summary>Creates the tools for a test. No session is running.</summary>
    [SetUp]
    public void CreateTools()
    {
      _harness = new DebuggerToolsHarness();
    }

    [Test]
    public void Debugger_Start_ServiceStarts_ReturnsItsSessionValues()
    {
      // Arrange
      _harness.SetupStartSucceeds();

      // Act
      DebuggerStartResult result = _harness.Tools.Debugger_Start(DebuggerToolsHarness.SamplePath);

      // Assert
      Assert.That(result.ProcessId, Is.EqualTo(DebuggerToolsHarness.StartedProcessId));
      Assert.That(result.ThreadId, Is.EqualTo(DebuggerToolsHarness.StartedThreadId));
      Assert.That(result.ImageBase, Is.EqualTo((ulong)DebuggerToolsHarness.StartedImageBase));
      Assert.That(result.EntryPoint, Is.EqualTo((ulong)DebuggerToolsHarness.StartedEntryPoint));
      Assert.That(result.Context.EFlags, Is.EqualTo(DebuggerToolsHarness.StartContextFlags));
    }

    [Test]
    public void Debugger_Start_Path_PassesItToTheService()
    {
      // Arrange
      _harness.SetupStartSucceeds();

      // Act
      _harness.Tools.Debugger_Start(DebuggerToolsHarness.SamplePath);

      // Assert
      _harness.Service.Verify(
        service => service.Start(DebuggerToolsHarness.SamplePath, ref It.Ref<string>.IsAny, ref It.Ref<uint>.IsAny,
                                 ref It.Ref<uint>.IsAny, ref It.Ref<nint>.IsAny, ref It.Ref<nint>.IsAny,
                                 ref It.Ref<CONTEXT>.IsAny),
        Times.Once());
    }

    [Test]
    public void Debugger_Start_ServiceStarts_NextStartIsRefused()
    {
      // Arrange
      _harness.SetupStartSucceeds();

      // Act
      _harness.Tools.Debugger_Start(DebuggerToolsHarness.SamplePath);

      // Assert
      McpException caught = Assert.Throws<McpException>(
        () => _harness.Tools.Debugger_Start(DebuggerToolsHarness.SamplePath));
      Assert.That(caught.Message, Is.EqualTo(AlreadyStartedText));
    }

    [Test]
    public void Debugger_Start_ServiceFails_ThrowsStartFailureText()
    {
      // Arrange
      _harness.SetupStartFails("The debugger could not be started.");

      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _harness.Tools.Debugger_Start(DebuggerToolsHarness.SamplePath));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(StartFailedPrefix + "The debugger could not be started."));
    }

    [Test]
    public void Debugger_Start_ServiceFails_SessionStaysNotStarted()
    {
      // Arrange
      _harness.SetupStartFails("The debugger could not be started.");

      // Act
      Assert.Throws<McpException>(() => _harness.Tools.Debugger_Start(DebuggerToolsHarness.SamplePath));

      // Assert
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Stop());
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
    }

    [Test]
    public void Debugger_Start_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.SetupStartThrows(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _harness.Tools.Debugger_Start(DebuggerToolsHarness.SamplePath));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    [Test]
    public void Debugger_Stop_SessionAtSystemBreakpoint_StopsTheService()
    {
      // Arrange
      _harness.StartSession();

      // Act
      _harness.Tools.Debugger_Stop();

      // Assert
      _harness.Service.Verify(service => service.Stop(), Times.Once());
    }

    [Test]
    public void Debugger_Stop_SessionAtSystemBreakpoint_NextGatedCallThrowsNotStartedText()
    {
      // Arrange
      _harness.StartSession();

      // Act
      _harness.Tools.Debugger_Stop();

      // Assert
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Continue());
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
    }

    [Test]
    public async Task Debugger_Stop_ReadySession_NextGatedCallThrowsNotStartedText()
    {
      // Arrange
      await _harness.ReachEntryPointAsync();

      // Act
      _harness.Tools.Debugger_Stop();

      // Assert
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Continue());
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
    }

    [Test]
    public async Task Debugger_Stop_ReadySession_NewSessionStartsBeforeItsEntryPoint()
    {
      // Arrange
      await _harness.ReachEntryPointAsync();

      // Act
      _harness.Tools.Debugger_Stop();
      _harness.StartSession();

      // Assert
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Continue());
      Assert.That(caught.Message, Is.EqualTo(EntryPointNotReachedText));
    }

    [Test]
    public void Debugger_Stop_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.StartSession();
      _harness.Service.Setup(service => service.Stop()).Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Stop());

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    [Test]
    public async Task Debugger_Continue_ReadySession_ContinuesTheService()
    {
      // Arrange
      await _harness.ReachEntryPointAsync();

      // Act
      _harness.Tools.Debugger_Continue();

      // Assert
      _harness.Service.Verify(service => service.Continue(), Times.Once());
    }

    [Test]
    public async Task Debugger_Continue_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      await _harness.ReachEntryPointAsync();
      _harness.Service.Setup(service => service.Continue()).Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Continue());

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    [Test]
    public async Task Debugger_ContinueToEntryPoint_StopArrives_ReturnsItsContext()
    {
      // Arrange
      _harness.StartSession();
      _harness.ExecutionControl.Setup(control => control.ContinueToEntryPointAsync())
                               .ReturnsAsync(new CONTEXT { EFlags = EntryPointFlags });

      // Act
      BreakpointResult result = await _harness.Tools.Debugger_ContinueToEntryPoint();

      // Assert
      Assert.That(result.Context.EFlags, Is.EqualTo(EntryPointFlags));
    }

    [Test]
    public async Task Debugger_ContinueToEntryPoint_StopArrives_SessionIsReady()
    {
      // Arrange
      _harness.StartSession();
      _harness.ExecutionControl.Setup(control => control.ContinueToEntryPointAsync()).ReturnsAsync(new CONTEXT());

      // Act
      await _harness.Tools.Debugger_ContinueToEntryPoint();

      // Assert
      Assert.DoesNotThrow(() => _harness.Tools.Debugger_Continue());
    }

    [Test]
    public async Task Debugger_ContinueToEntryPoint_ExecutionControlThrowsAtTheCall_ThrowsTheSameException()
    {
      // Arrange
      _harness.StartSession();
      var failure = new McpException("Debugger has not been started yet.");
      _harness.ExecutionControl.Setup(control => control.ContinueToEntryPointAsync()).Throws(failure);

      // Act
      McpException caught = await Assert.ThrowsAsync<McpException>(
        async () => await _harness.Tools.Debugger_ContinueToEntryPoint());

      // Assert
      Assert.That(caught, Is.SameAs(failure));
    }

    [Test]
    public async Task Debugger_ContinueToEntryPoint_ExecutionControlFailsItsTask_ThrowsTheSameException()
    {
      // Arrange
      _harness.StartSession();
      var failure = new McpException("A timeout occurred before the entry point was reached.");
      _harness.ExecutionControl.Setup(control => control.ContinueToEntryPointAsync()).ThrowsAsync(failure);

      // Act
      McpException caught = await Assert.ThrowsAsync<McpException>(
        async () => await _harness.Tools.Debugger_ContinueToEntryPoint());

      // Assert
      Assert.That(caught, Is.SameAs(failure));
    }

    [Test]
    public void ProcessExit_SessionAtSystemBreakpoint_NextGatedCallThrowsNotStartedText()
    {
      // Arrange
      _harness.StartSession();

      // Act
      _harness.RaiseProcessExit();

      // Assert
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Continue());
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
    }

    [Test]
    public async Task ProcessExit_ReadySession_NextGatedCallThrowsNotStartedText()
    {
      // Arrange
      await _harness.ReachEntryPointAsync();

      // Act
      _harness.RaiseProcessExit();

      // Assert
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Continue());
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
    }

    [Test]
    public async Task ProcessExit_ReadySession_NewSessionStartsBeforeItsEntryPoint()
    {
      // Arrange
      await _harness.ReachEntryPointAsync();

      // Act
      _harness.RaiseProcessExit();
      _harness.StartSession();

      // Assert
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Continue());
      Assert.That(caught.Message, Is.EqualTo(EntryPointNotReachedText));
    }
  }
}
