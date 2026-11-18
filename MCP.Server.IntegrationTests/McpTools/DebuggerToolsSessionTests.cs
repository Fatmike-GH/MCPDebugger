using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using MCP.Server.IntegrationTests.TestHelpers;
using ModelContextProtocol.Protocol;

namespace MCP.Server.IntegrationTests.McpTools
{
  /// <summary>
  /// Debugging sessions through the tools of the server. Every test owns its server process because a
  /// session changes the state of the server. The tests stay within the first instructions behind the entry
  /// point, long before the sample target waits for console input - except the one about tools called while
  /// the debuggee runs, which lets it run free. Addresses are never fixed values: they are taken from the
  /// results of the server.
  /// </summary>
  public class DebuggerToolsSessionTests
  {
#if X64
    /// <summary>Name of the instruction pointer in a thread context of this build's bitness.</summary>
    private const string InstructionPointerName = "rip";
#else
    /// <summary>Name of the instruction pointer in a thread context of this build's bitness.</summary>
    private const string InstructionPointerName = "eip";
#endif

    /// <summary>Number of bytes disassembled at the entry point; holds at least two instructions.</summary>
    private const int DisassemblyLength = 32;

    /// <summary>
    /// Error text of a tool that needs a stopped debuggee, called while the debuggee runs; the tool name goes
    /// into the placeholder.
    /// </summary>
    private const string DebuggeeRunningErrorFormat =
      "An error occurred invoking '{0}': The debuggee is running. This command is only possible while it is stopped.";

    /// <summary>
    /// Upper bound for the answer of a tool that is refused because the debuggee runs. The refusal needs one
    /// round trip to the debugger (some milliseconds); the bound tells it from a tool that waits for a stop
    /// until its own time limit of 50 seconds.
    /// </summary>
    private static readonly TimeSpan RefusalTimeLimit = TimeSpan.FromSeconds(10);

    /// <summary>A call of a tool without parameters.</summary>
    private static Dictionary<string, object> NoArguments => new Dictionary<string, object>();

    [Test, CancelAfter(ServerSession.TimeoutMilliseconds)]
    public async Task Session_StartToStop_ReturnsDocumentedResults(CancellationToken cancellationToken)
    {
      // Arrange
      await using ServerSession server = await ServerSession.StartAsync(cancellationToken);

      // Act: start
      JsonElement start = await StartSampleTargetAsync(server, cancellationToken);

      // Assert
      uint processId = start.GetProperty("processId").GetUInt32();
      ulong imageBase = start.GetProperty("imageBase").GetUInt64();
      ulong entryPoint = start.GetProperty("entryPoint").GetUInt64();
      Assert.That(processId, Is.Not.Zero);
      Assert.That(start.GetProperty("threadId").GetUInt32(), Is.Not.Zero);
      Assert.That(imageBase, Is.Not.Zero);
      Assert.That(entryPoint, Is.Not.Zero);
      Assert.That(InstructionPointer(start), Is.Not.Zero);

      // Act: continue to the entry point
      JsonElement atEntryPoint = Success(await server.CallAsync(ToolNames.ContinueToEntryPoint, NoArguments, cancellationToken));

      // Assert
      Assert.That(InstructionPointer(atEntryPoint), Is.EqualTo(entryPoint));

      // Act: modules
      JsonElement modules = Success(await server.CallAsync(ToolNames.GetModules, new() { ["processId"] = processId }, cancellationToken));

      // Assert
      List<string> moduleNames = modules.GetProperty("modules").EnumerateArray()
        .Select(module => module.GetProperty("moduleName").GetString())
        .ToList();
      Assert.That(moduleNames, Has.Some.EqualTo(SampleTarget.FileName).IgnoreCase);

      // Act: disassemble at the entry point
      JsonElement disassembly = Success(await server.CallAsync(ToolNames.Disassemble, new() { ["va"] = entryPoint, ["length"] = DisassemblyLength }, cancellationToken));

      // Assert
      List<ulong> instructionAddresses = disassembly.GetProperty("instructions").EnumerateArray()
        .Select(instruction => instruction.GetProperty("virtualAdress").GetUInt64())
        .ToList();
      Assert.That(instructionAddresses, Has.Count.GreaterThan(1));
      Assert.That(instructionAddresses[0], Is.EqualTo(entryPoint));

      // Act: read memory at the image base
      JsonElement memory = Success(await server.CallAsync(ToolNames.ReadMemory, new() { ["va"] = imageBase, ["size"] = 2 }, cancellationToken));

      // Assert
      // The bytes come as a JSON array inside a string; an image starts with the letters "MZ" (77, 90).
      Assert.That(memory.GetProperty("memory").GetString(), Is.EqualTo("[77,90]"));

      // Act: step over
      JsonElement afterStepOver = Success(await server.CallAsync(ToolNames.StepOver, NoArguments, cancellationToken));

      // Assert
      // Stepping over the first instruction at the entry point - a call in the 32-bit sample target, a plain
      // instruction in the 64-bit one - ends at the second instruction of the disassembly in both cases.
      Assert.That(InstructionPointer(afterStepOver), Is.EqualTo(instructionAddresses[1]));

      // Act: stop
      CallToolResult stop = await server.CallAsync(ToolNames.Stop, NoArguments, cancellationToken);

      // Assert
      Assert.That(stop.IsError, Is.Not.True);
      Assert.That(stop.Content, Is.Empty);
    }

    [Test, CancelAfter(ServerSession.TimeoutMilliseconds)]
    public async Task GoTools_WhileTheDebuggeeRuns_ReturnRunningErrorAtOnce(CancellationToken cancellationToken)
    {
      // Arrange
      await using ServerSession server = await ServerSession.StartAsync(cancellationToken);
      await StartSampleTargetAsync(server, cancellationToken);
      Success(await server.CallAsync(ToolNames.ContinueToEntryPoint, NoArguments, cancellationToken));

      // Act: resume from the entry point; no breakpoint is set, so nothing stops the debuggee again
      CallToolResult resumed = await server.CallAsync(ToolNames.Continue, NoArguments, cancellationToken);

      // Assert
      Assert.That(resumed.IsError, Is.Not.True, () => ServerSession.Text(resumed));

      // Act: a step while the debuggee runs
      var stepWatch = Stopwatch.StartNew();
      CallToolResult step = await server.CallAsync(ToolNames.StepInto, NoArguments, cancellationToken);
      stepWatch.Stop();

      // Assert
      Assert.That(step.IsError, Is.True);
      Assert.That(ServerSession.Text(step), Is.EqualTo(string.Format(CultureInfo.InvariantCulture, DebuggeeRunningErrorFormat, ToolNames.StepInto)));
      Assert.That(stepWatch.Elapsed, Is.LessThan(RefusalTimeLimit));

      // Act: a continue while the debuggee runs
      var continueWatch = Stopwatch.StartNew();
      CallToolResult resumedAgain = await server.CallAsync(ToolNames.Continue, NoArguments, cancellationToken);
      continueWatch.Stop();

      // Assert
      Assert.That(resumedAgain.IsError, Is.True);
      Assert.That(ServerSession.Text(resumedAgain), Is.EqualTo(string.Format(CultureInfo.InvariantCulture, DebuggeeRunningErrorFormat, ToolNames.Continue)));
      Assert.That(continueWatch.Elapsed, Is.LessThan(RefusalTimeLimit));

      // Act: stop
      CallToolResult stop = await server.CallAsync(ToolNames.Stop, NoArguments, cancellationToken);

      // Assert
      // A session whose debuggee had ended meanwhile would refuse the stop: the errors above came from a
      // debuggee that ran.
      Assert.That(stop.IsError, Is.Not.True, () => ServerSession.Text(stop));
    }

    [Test, CancelAfter(ServerSession.TimeoutMilliseconds)]
    public async Task GetModules_BeforeEntryPoint_ReturnsGateError(CancellationToken cancellationToken)
    {
      // Arrange
      await using ServerSession server = await ServerSession.StartAsync(cancellationToken);
      JsonElement start = await StartSampleTargetAsync(server, cancellationToken);
      uint processId = start.GetProperty("processId").GetUInt32();

      // Act
      CallToolResult result = await server.CallAsync(ToolNames.GetModules, new() { ["processId"] = processId }, cancellationToken);

      // Assert
      Assert.That(result.IsError, Is.True);
      Assert.That(ServerSession.Text(result), Does.EndWith("Entry point not yet reached."));
    }

    [Test, CancelAfter(ServerSession.TimeoutMilliseconds)]
    public async Task Start_SecondTime_ReturnsAlreadyStartedError(CancellationToken cancellationToken)
    {
      // Arrange
      await using ServerSession server = await ServerSession.StartAsync(cancellationToken);
      await StartSampleTargetAsync(server, cancellationToken);

      // Act
      CallToolResult result = await server.CallAsync(ToolNames.Start, new() { ["path"] = SampleTarget.ExecutablePath }, cancellationToken);

      // Assert
      Assert.That(result.IsError, Is.True);
      Assert.That(ServerSession.Text(result), Does.EndWith("Debugger already started."));
    }

    /// <summary>Lets the server start a debugging session of the sample target.</summary>
    /// <param name="server">A server without a session.</param>
    /// <param name="cancellationToken">Cancels the call.</param>
    /// <returns>The result object of the start.</returns>
    private static async Task<JsonElement> StartSampleTargetAsync(ServerSession server, CancellationToken cancellationToken)
    {
      return Success(await server.CallAsync(ToolNames.Start, new() { ["path"] = SampleTarget.ExecutablePath }, cancellationToken));
    }

    /// <summary>
    /// Asserts that a tool call succeeded and returns its result object. A successful result carries no error
    /// mark at all, so the mark is compared against true and not against false.
    /// </summary>
    /// <param name="result">The result of a tool call that returns an object.</param>
    /// <returns>The result object.</returns>
    private static JsonElement Success(CallToolResult result)
    {
      Assert.That(result.IsError, Is.Not.True, () => ServerSession.Text(result));
      return ServerSession.ParseJson(result);
    }

    /// <summary>Reads the instruction pointer from a result that carries a thread context.</summary>
    /// <param name="result">The result object of a start, continue or step tool.</param>
    /// <returns>The address of the next instruction to execute.</returns>
    private static ulong InstructionPointer(JsonElement result)
    {
      return result.GetProperty("context").GetProperty(InstructionPointerName).GetUInt64();
    }
  }
}
