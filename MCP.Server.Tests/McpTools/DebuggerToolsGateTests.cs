using System.Text.Json;
using MCP.Server.McpTools;
using MCP.Server.Tests.TestHelpers;
using ModelContextProtocol;

namespace MCP.Server.Tests.McpTools
{
  /// <summary>
  /// Which tool is refused in which session state, and with which text. The states are "no session", "session at
  /// the system breakpoint" (started, entry point not reached) and "ready" (entry point reached). A refused tool
  /// must not touch the debugger.
  /// </summary>
  public class DebuggerToolsGateTests
  {
    /// <summary>Error text of a tool that needs a session when none is running.</summary>
    private const string NotStartedText = "Debugger not yet started.";

    /// <summary>Error text of the start tool while a session is running.</summary>
    private const string AlreadyStartedText = "Debugger already started.";

    /// <summary>Error text of a tool that needs the entry point while the session stands before it.</summary>
    private const string EntryPointNotReachedText = "Entry point not yet reached.";

    /// <summary>Error text of the entry-point tool when the session has passed its entry point.</summary>
    private const string EntryPointAlreadyReachedText = "Entry point already reached.";

    /// <summary>An address the address tools are called with.</summary>
    private const ulong Address = TestAddresses.Sample;

    /// <summary>A number of bytes the disassemble and read-memory tools are called with.</summary>
    private const uint ByteCount = 16;

    /// <summary>A module name the module tools and the API-breakpoint tool are called with.</summary>
    private const string ModuleName = "kernel32.dll";

    /// <summary>A function name the API-breakpoint tool is called with.</summary>
    private const string FunctionName = "CreateFileW";

    /// <summary>The tools on mocked collaborators.</summary>
    private DebuggerToolsHarness _harness;

    /// <summary>Creates the tools for a test. No session is running.</summary>
    [SetUp]
    public void CreateTools()
    {
      _harness = new DebuggerToolsHarness();
    }

    [TestCaseSource(nameof(ReadyGatedTools))]
    public async Task ReadyGatedTool_NoSession_ThrowsNotStartedText(Func<DebuggerTools, Task> call)
    {
      // Arrange
      ForgetCallsSoFar();

      // Act
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await call(_harness.Tools));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
      _harness.Service.VerifyNoOtherCalls();
      _harness.ExecutionControl.VerifyNoOtherCalls();
    }

    [TestCaseSource(nameof(ReadyGatedTools))]
    public async Task ReadyGatedTool_SessionAtSystemBreakpoint_ThrowsEntryPointNotReachedText(Func<DebuggerTools, Task> call)
    {
      // Arrange
      _harness.StartSession();
      ForgetCallsSoFar();

      // Act
      McpException caught = await Assert.ThrowsAsync<McpException>(async () => await call(_harness.Tools));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(EntryPointNotReachedText));
      _harness.Service.VerifyNoOtherCalls();
      _harness.ExecutionControl.VerifyNoOtherCalls();
    }

    [Test]
    public void Debugger_Start_SessionAtSystemBreakpoint_ThrowsAlreadyStartedText()
    {
      // Arrange
      _harness.StartSession();

      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _harness.Tools.Debugger_Start(DebuggerToolsHarness.SamplePath));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(AlreadyStartedText));
    }

    [Test]
    public async Task Debugger_Start_ReadySession_ThrowsAlreadyStartedText()
    {
      // Arrange
      await _harness.ReachEntryPointAsync();

      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _harness.Tools.Debugger_Start(DebuggerToolsHarness.SamplePath));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(AlreadyStartedText));
    }

    [Test]
    public void Debugger_Stop_NoSession_ThrowsNotStartedText()
    {
      // Arrange
      ForgetCallsSoFar();

      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Stop());

      // Assert
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
      _harness.Service.VerifyNoOtherCalls();
    }

    [Test]
    public async Task Debugger_ContinueToEntryPoint_NoSession_ThrowsNotStartedText()
    {
      // Arrange
      ForgetCallsSoFar();

      // Act
      McpException caught = await Assert.ThrowsAsync<McpException>(
        async () => await _harness.Tools.Debugger_ContinueToEntryPoint());

      // Assert
      Assert.That(caught.Message, Is.EqualTo(NotStartedText));
      _harness.ExecutionControl.VerifyNoOtherCalls();
    }

    [Test]
    public async Task Debugger_ContinueToEntryPoint_EntryPointReached_ThrowsAlreadyReachedText()
    {
      // Arrange
      await _harness.ReachEntryPointAsync();
      ForgetCallsSoFar();

      // Act
      McpException caught = await Assert.ThrowsAsync<McpException>(
        async () => await _harness.Tools.Debugger_ContinueToEntryPoint());

      // Assert
      Assert.That(caught.Message, Is.EqualTo(EntryPointAlreadyReachedText));
      _harness.ExecutionControl.VerifyNoOtherCalls();
    }

    [Test]
    public void Debugger_HexToDecConverter_SessionAtSystemBreakpoint_Converts()
    {
      // Arrange
      _harness.StartSession();

      // Act
      ulong value = _harness.Tools.Debugger_HexToDecConverter("FF");

      // Assert
      Assert.That(value, Is.EqualTo(255ul));
    }

    [Test]
    public async Task Debugger_HexToDecConverter_ReadySession_Converts()
    {
      // Arrange
      await _harness.ReachEntryPointAsync();

      // Act
      ulong value = _harness.Tools.Debugger_HexToDecConverter("FF");

      // Assert
      Assert.That(value, Is.EqualTo(255ul));
    }

    /// <summary>
    /// The tools that need a session that has reached its entry point, each as a call with valid arguments. A new
    /// tool behind that gate is added here.
    /// </summary>
    /// <returns>One test case per tool, named by the tool.</returns>
    private static IEnumerable<TestCaseData> ReadyGatedTools()
    {
      JsonElement bytes = JsonSerializer.SerializeToElement(new[] { 1, 2 });

      yield return Tool(nameof(DebuggerTools.Debugger_Continue), tools => tools.Debugger_Continue());
      yield return WaitingTool(nameof(DebuggerTools.Debugger_ContinueToBreakpoint),
                               tools => tools.Debugger_ContinueToBreakpoint(Address));
      yield return WaitingTool(nameof(DebuggerTools.Debugger_ContinueToApiBreakpoint),
                               tools => tools.Debugger_ContinueToApiBreakpoint(DebuggerToolsHarness.StartedProcessId, ModuleName, FunctionName));
      yield return WaitingTool(nameof(DebuggerTools.Debugger_StepInto), tools => tools.Debugger_StepInto());
      yield return WaitingTool(nameof(DebuggerTools.Debugger_StepOver), tools => tools.Debugger_StepOver());
      yield return Tool(nameof(DebuggerTools.Debugger_Disassemble), tools => tools.Debugger_Disassemble(Address, ByteCount));
      yield return Tool(nameof(DebuggerTools.Debugger_TryResolveString), tools => tools.Debugger_TryResolveString(Address));
      yield return Tool(nameof(DebuggerTools.Debugger_ReadMemory), tools => tools.Debugger_ReadMemory(Address, ByteCount));
      yield return Tool(nameof(DebuggerTools.Debugger_WriteMemory), tools => tools.Debugger_WriteMemory(Address, bytes));
      yield return Tool(nameof(DebuggerTools.Debugger_GetModules),
                        tools => tools.Debugger_GetModules(DebuggerToolsHarness.StartedProcessId));
      yield return Tool(nameof(DebuggerTools.Debugger_GetModuleImports),
                        tools => tools.Debugger_GetModuleImports(DebuggerToolsHarness.StartedProcessId, ModuleName));
      yield return Tool(nameof(DebuggerTools.Debugger_GetModuleExports),
                        tools => tools.Debugger_GetModuleExports(DebuggerToolsHarness.StartedProcessId, ModuleName));
      yield return Tool(nameof(DebuggerTools.Debugger_GetModuleSections),
                        tools => tools.Debugger_GetModuleSections(DebuggerToolsHarness.StartedProcessId, ModuleName));
    }

    /// <summary>
    /// Creates the test case of a tool that answers at once. Its call is wrapped into a completed task, so that
    /// one test awaits tools of both kinds; an exception of the tool leaves the wrapper as it left the tool.
    /// </summary>
    /// <param name="name">Name of the tool; the name of the test case.</param>
    /// <param name="call">Calls the tool with valid arguments.</param>
    /// <returns>The test case.</returns>
    private static TestCaseData Tool(string name, Action<DebuggerTools> call)
    {
      return WaitingTool(name, tools =>
      {
        call(tools);
        return Task.CompletedTask;
      });
    }

    /// <summary>Creates the test case of a tool that waits for a stop of the debuggee.</summary>
    /// <param name="name">Name of the tool; the name of the test case.</param>
    /// <param name="call">Calls the tool with valid arguments.</param>
    /// <returns>The test case.</returns>
    private static TestCaseData WaitingTool(string name, Func<DebuggerTools, Task> call)
    {
      return new TestCaseData(call).SetArgDisplayNames(name);
    }

    /// <summary>
    /// Forgets the calls the collaborators have received up to now (the event subscription of the tools, the
    /// start of a session), so that a test can state that a refused tool called nothing.
    /// </summary>
    private void ForgetCallsSoFar()
    {
      _harness.Service.Invocations.Clear();
      _harness.ExecutionControl.Invocations.Clear();
    }
  }
}
