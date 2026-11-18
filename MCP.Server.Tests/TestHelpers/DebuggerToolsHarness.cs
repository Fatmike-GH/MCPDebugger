using MCP.Debugger;
using MCP.Debugger.Types.EventArgs;
using MCP.Debugger.Types.Interop;
using MCP.Server.Interfaces.McpTools.Execution;
using MCP.Server.Interfaces.McpTools.Formatting;
using MCP.Server.McpTools;
using Moq;
using Moq.Language.Flow;

namespace MCP.Server.Tests.TestHelpers
{
  /// <summary>
  /// The debugger tools on three mocked collaborators, for one test. A session state is reached the way a client
  /// reaches it: through the tools. Nothing here reads or writes the tools' private state, and nothing here
  /// asserts.
  /// </summary>
  /// <remarks>
  /// All mocks are loose. The start of the debugger service is set up only on request, because a later set-up
  /// of the same member replaces the earlier one: a test that needs its own answer to the start must not lose it
  /// to this class.
  /// </remarks>
  internal sealed class DebuggerToolsHarness
  {
    /// <summary>The path a session is started with. It is only a text: no file is opened.</summary>
    public const string SamplePath = @"C:\samples\target.exe";

    /// <summary>The process id the mocked service reports for a started session.</summary>
    public const uint StartedProcessId = 4711;

    /// <summary>The id of the main thread the mocked service reports for a started session.</summary>
    public const uint StartedThreadId = 12;

    /// <summary>The image base the mocked service reports for a started session.</summary>
    public const nint StartedImageBase = TestAddresses.Sample;

    /// <summary>The entry point the mocked service reports for a started session.</summary>
    public const nint StartedEntryPoint = 0x401000;

    /// <summary>
    /// The flags register of the thread context the mocked service reports for a started session. The flags
    /// tell a context apart in both bitnesses, so no test needs a bitness-specific register for that.
    /// </summary>
    public const uint StartContextFlags = 0x246;

    /// <summary>Creates the three mocks and the tools on them. No session is running.</summary>
    public DebuggerToolsHarness()
    {
      Service = new Mock<IDebuggerService>();
      ExecutionControl = new Mock<IExecutionControl>();
      MemoryFormatter = new Mock<IMemoryFormatter>();
      Tools = new DebuggerTools(Service.Object, ExecutionControl.Object, MemoryFormatter.Object);
    }

    /// <summary>
    /// The answer of the mocked service to a start. The signature is the one of the service's start, so that the
    /// answer can write the values the start hands back through its <c>ref</c> parameters.
    /// </summary>
    /// <param name="path">Full path of the executable.</param>
    /// <param name="error">Receives the reason of a failed start.</param>
    /// <param name="processId">Receives the process id of the debuggee.</param>
    /// <param name="threadId">Receives the id of the debuggee's main thread.</param>
    /// <param name="imageBase">Receives the image base of the debuggee.</param>
    /// <param name="entryPoint">Receives the entry point of the debuggee.</param>
    /// <param name="context">Receives the main thread's context at the system breakpoint.</param>
    /// <returns>True when the debuggee runs under the debugger.</returns>
    private delegate bool StartAnswer(string path, ref string error, ref uint processId, ref uint threadId,
                                      ref nint imageBase, ref nint entryPoint, ref CONTEXT context);

    /// <summary>The debugger service the tools drive.</summary>
    public Mock<IDebuggerService> Service { get; }

    /// <summary>The execution control the waiting tools run the debuggee with.</summary>
    public Mock<IExecutionControl> ExecutionControl { get; }

    /// <summary>The formatter the read-memory tool answers with.</summary>
    public Mock<IMemoryFormatter> MemoryFormatter { get; }

    /// <summary>The tools under test.</summary>
    public DebuggerTools Tools { get; }

    /// <summary>
    /// Lets the next start of the mocked service succeed: it hands back the process id, thread id, image base
    /// and entry point of this class and a thread context with <see cref="StartContextFlags"/>.
    /// </summary>
    public void SetupStartSucceeds()
    {
      SetupStart().Returns(new StartAnswer(WriteStartedSession));
    }

    /// <summary>Lets the next start of the mocked service fail.</summary>
    /// <param name="error">The reason the service reports.</param>
    public void SetupStartFails(string error)
    {
      SetupStart().Returns(new StartAnswer(
        (string path, ref string reportedError, ref uint processId, ref uint threadId, ref nint imageBase,
         ref nint entryPoint, ref CONTEXT context) =>
        {
          reportedError = error;
          return false;
        }));
    }

    /// <summary>Lets the next start of the mocked service throw.</summary>
    /// <param name="exception">The exception the service throws.</param>
    public void SetupStartThrows(Exception exception)
    {
      SetupStart().Throws(exception);
    }

    /// <summary>Starts a session through the start tool. The session then stands at the system breakpoint.</summary>
    public void StartSession()
    {
      SetupStartSucceeds();
      Tools.Debugger_Start(SamplePath);
    }

    /// <summary>
    /// Starts a session and runs it to its entry point, both through the tools. The session is then ready for
    /// every tool. The wait for the entry point is a task that has completed already.
    /// </summary>
    /// <returns>A task that ends when the entry-point tool has returned.</returns>
    public async Task ReachEntryPointAsync()
    {
      StartSession();
      ExecutionControl.Setup(control => control.ContinueToEntryPointAsync()).ReturnsAsync(new CONTEXT());
      await Tools.Debugger_ContinueToEntryPoint();
    }

    /// <summary>Raises the process-exit event of the mocked service for the debuggee of the started session.</summary>
    public void RaiseProcessExit()
    {
      Service.Raise(service => service.ExitProcess += null, Service.Object, new ExitProcessEventArgs(StartedProcessId));
    }

    /// <summary>Begins the set-up of the service's start for any path and any content of the <c>ref</c> parameters.</summary>
    /// <returns>The set-up, to be completed with the answer of the service.</returns>
    private ISetup<IDebuggerService, bool> SetupStart()
    {
      return Service.Setup(service => service.Start(It.IsAny<string>(), ref It.Ref<string>.IsAny,
                                                    ref It.Ref<uint>.IsAny, ref It.Ref<uint>.IsAny,
                                                    ref It.Ref<nint>.IsAny, ref It.Ref<nint>.IsAny,
                                                    ref It.Ref<CONTEXT>.IsAny));
    }

    /// <summary>The answer of a successful start: writes the values of the started session.</summary>
    /// <param name="path">Full path of the executable; not used.</param>
    /// <param name="error">Left unchanged, as the service leaves it on success.</param>
    /// <param name="processId">Receives <see cref="StartedProcessId"/>.</param>
    /// <param name="threadId">Receives <see cref="StartedThreadId"/>.</param>
    /// <param name="imageBase">Receives <see cref="StartedImageBase"/>.</param>
    /// <param name="entryPoint">Receives <see cref="StartedEntryPoint"/>.</param>
    /// <param name="context">Receives a thread context with <see cref="StartContextFlags"/>.</param>
    /// <returns>True.</returns>
    private static bool WriteStartedSession(string path, ref string error, ref uint processId, ref uint threadId,
                                            ref nint imageBase, ref nint entryPoint, ref CONTEXT context)
    {
      processId = StartedProcessId;
      threadId = StartedThreadId;
      imageBase = StartedImageBase;
      entryPoint = StartedEntryPoint;
      context = new CONTEXT { EFlags = StartContextFlags };
      return true;
    }
  }
}
