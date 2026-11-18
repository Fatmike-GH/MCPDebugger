using System.Linq.Expressions;
using MCP.Debugger.Types.Interop;
using MCP.Server.Interfaces.McpTools.Execution;
using MCP.Server.McpTools;
using MCP.Server.McpTools.Results;
using MCP.Server.Tests.TestHelpers;
using ModelContextProtocol;
using Moq;

namespace MCP.Server.Tests.McpTools
{
  /// <summary>
  /// The tools that let a ready session run and wait for its next stop: continue to a breakpoint, continue to a
  /// breakpoint on an exported function, step into and step over. What they hand to the execution control, that
  /// they answer with the thread context of the stop, and that an error of the execution control reaches the
  /// caller untouched. No test waits: every mocked wait has ended when it is returned.
  /// </summary>
  public class DebuggerToolsWaitingTests
  {
    /// <summary>The address the breakpoint tests run to.</summary>
    private const nint BreakpointAddress = 0x401000;

    /// <summary>The handle the mocked service reports for the module of the API-breakpoint tests.</summary>
    private const nint ModuleHandle = 0x10000000;

    /// <summary>The address the mocked service reports for the function of the API-breakpoint tests.</summary>
    private const nint FunctionAddress = 0x10001234;

    /// <summary>The module of the API-breakpoint tests.</summary>
    private const string ModuleName = "kernel32.dll";

    /// <summary>The function of the API-breakpoint tests.</summary>
    private const string FunctionName = "CreateFileW";

    /// <summary>A flags value that marks the thread context of the stop a test lets arrive.</summary>
    private const uint StopFlags = 0x206;

    /// <summary>The tools on mocked collaborators.</summary>
    private DebuggerToolsHarness _harness;

    /// <summary>Creates the tools for a test and a session that has reached its entry point.</summary>
    /// <returns>A task that ends when the session is ready.</returns>
    [SetUp]
    public async Task CreateToolsWithReadySessionAsync()
    {
      _harness = new DebuggerToolsHarness();
      await _harness.ReachEntryPointAsync();
    }

    [Test]
    public async Task Debugger_ContinueToBreakpoint_StopArrives_ReturnsItsContext()
    {
      // Arrange
      _harness.ExecutionControl.Setup(control => control.ContinueToBreakpointAsync(BreakpointAddress))
                               .ReturnsAsync(new CONTEXT { EFlags = StopFlags });

      // Act
      BreakpointResult result = await _harness.Tools.Debugger_ContinueToBreakpoint((ulong)BreakpointAddress);

      // Assert
      Assert.That(result.Context.EFlags, Is.EqualTo(StopFlags));
    }

    [Test]
    public async Task Debugger_ContinueToBreakpoint_ZeroAddress_PassesItOn()
    {
      // Act
      await _harness.Tools.Debugger_ContinueToBreakpoint(0);

      // Assert
      _harness.ExecutionControl.Verify(control => control.ContinueToBreakpointAsync(nint.Zero), Times.Once());
    }

    [Test]
    public async Task Debugger_ContinueToApiBreakpoint_FunctionFound_RunsToItsAddressAndReturnsTheContext()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetRemoteModuleHandle(DebuggerToolsHarness.StartedProcessId, ModuleName))
                      .Returns(ModuleHandle);
      _harness.Service.Setup(service => service.GetRemoteProcAddress(DebuggerToolsHarness.StartedProcessId, ModuleHandle, FunctionName))
                      .Returns(FunctionAddress);
      _harness.ExecutionControl.Setup(control => control.ContinueToBreakpointAsync(FunctionAddress))
                               .ReturnsAsync(new CONTEXT { EFlags = StopFlags });

      // Act
      BreakpointResult result = await _harness.Tools.Debugger_ContinueToApiBreakpoint(
        DebuggerToolsHarness.StartedProcessId, ModuleName, FunctionName);

      // Assert
      Assert.That(result.Context.EFlags, Is.EqualTo(StopFlags));
    }

    [Test]
    public async Task Debugger_ContinueToApiBreakpoint_ModuleNotFound_AsksForTheFunctionWithHandleZero()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetRemoteModuleHandle(DebuggerToolsHarness.StartedProcessId, ModuleName))
                      .Returns(nint.Zero);

      // Act
      await _harness.Tools.Debugger_ContinueToApiBreakpoint(DebuggerToolsHarness.StartedProcessId, ModuleName, FunctionName);

      // Assert
      _harness.Service.Verify(
        service => service.GetRemoteProcAddress(DebuggerToolsHarness.StartedProcessId, nint.Zero, FunctionName),
        Times.Once());
    }

    [Test]
    public async Task Debugger_ContinueToApiBreakpoint_FunctionNotFound_PassesAddressZeroOn()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetRemoteModuleHandle(DebuggerToolsHarness.StartedProcessId, ModuleName))
                      .Returns(ModuleHandle);
      _harness.Service.Setup(service => service.GetRemoteProcAddress(DebuggerToolsHarness.StartedProcessId, ModuleHandle, FunctionName))
                      .Returns(nint.Zero);

      // Act
      await _harness.Tools.Debugger_ContinueToApiBreakpoint(DebuggerToolsHarness.StartedProcessId, ModuleName, FunctionName);

      // Assert
      _harness.ExecutionControl.Verify(control => control.ContinueToBreakpointAsync(nint.Zero), Times.Once());
    }

    [Test]
    public async Task Debugger_StepInto_StopArrives_ReturnsItsContext()
    {
      // Arrange
      _harness.ExecutionControl.Setup(control => control.StepIntoAsync())
                               .ReturnsAsync(new CONTEXT { EFlags = StopFlags });

      // Act
      BreakpointResult result = await _harness.Tools.Debugger_StepInto();

      // Assert
      Assert.That(result.Context.EFlags, Is.EqualTo(StopFlags));
      _harness.ExecutionControl.Verify(control => control.StepOverAsync(), Times.Never());
    }

    [Test]
    public async Task Debugger_StepOver_StopArrives_ReturnsItsContext()
    {
      // Arrange
      _harness.ExecutionControl.Setup(control => control.StepOverAsync())
                               .ReturnsAsync(new CONTEXT { EFlags = StopFlags });

      // Act
      BreakpointResult result = await _harness.Tools.Debugger_StepOver();

      // Assert
      Assert.That(result.Context.EFlags, Is.EqualTo(StopFlags));
      _harness.ExecutionControl.Verify(control => control.StepIntoAsync(), Times.Never());
    }

    [TestCaseSource(nameof(WaitingTools))]
    public async Task WaitingTool_ExecutionControlThrowsAtTheCall_ThrowsTheSameException(
      Expression<Func<IExecutionControl, Task<CONTEXT>>> wait, Func<DebuggerTools, Task> call)
    {
      // Arrange
      var failure = new McpException("The given absolute virtual address is zero.");
      _harness.ExecutionControl.Setup(wait).Throws(failure);

      // Act
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await call(_harness.Tools));

      // Assert
      Assert.That(caught, Is.SameAs(failure));
    }

    [TestCaseSource(nameof(WaitingTools))]
    public async Task WaitingTool_ExecutionControlFailsItsTask_ThrowsTheSameException(
      Expression<Func<IExecutionControl, Task<CONTEXT>>> wait, Func<DebuggerTools, Task> call)
    {
      // Arrange
      var failure = new McpException("A timeout occurred before the stop arrived.");
      _harness.ExecutionControl.Setup(wait).ThrowsAsync(failure);

      // Act
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await call(_harness.Tools));

      // Assert
      Assert.That(caught, Is.SameAs(failure));
    }

    /// <summary>
    /// The waiting tools of a ready session, each with the wait of the execution control it relies on and a call
    /// with valid arguments.
    /// </summary>
    /// <returns>One test case per tool, named by the tool.</returns>
    private static IEnumerable<TestCaseData> WaitingTools()
    {
      yield return WaitingTool(nameof(DebuggerTools.Debugger_ContinueToBreakpoint),
                               control => control.ContinueToBreakpointAsync(It.IsAny<nint>()),
                               tools => tools.Debugger_ContinueToBreakpoint((ulong)BreakpointAddress));
      yield return WaitingTool(nameof(DebuggerTools.Debugger_ContinueToApiBreakpoint),
                               control => control.ContinueToBreakpointAsync(It.IsAny<nint>()),
                               tools => tools.Debugger_ContinueToApiBreakpoint(DebuggerToolsHarness.StartedProcessId, ModuleName, FunctionName));
      yield return WaitingTool(nameof(DebuggerTools.Debugger_StepInto),
                               control => control.StepIntoAsync(),
                               tools => tools.Debugger_StepInto());
      yield return WaitingTool(nameof(DebuggerTools.Debugger_StepOver),
                               control => control.StepOverAsync(),
                               tools => tools.Debugger_StepOver());
    }

    /// <summary>Creates the test case of a waiting tool.</summary>
    /// <param name="name">Name of the tool; the name of the test case.</param>
    /// <param name="wait">The wait of the execution control the tool relies on.</param>
    /// <param name="call">Calls the tool with valid arguments.</param>
    /// <returns>The test case.</returns>
    private static TestCaseData WaitingTool(string name,
                                            Expression<Func<IExecutionControl, Task<CONTEXT>>> wait,
                                            Func<DebuggerTools, Task> call)
    {
      return new TestCaseData(wait, call).SetArgDisplayNames(name);
    }
  }
}
