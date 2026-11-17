using System.ComponentModel;
using System.Diagnostics;
using MCP.Debugger.IntegrationTests.TestHelpers;

namespace MCP.Debugger.IntegrationTests
{
  /// <summary>
  /// Run-wide check of this test assembly: no test may leave a debuggee process behind. A failure here makes
  /// the whole run fail although every single test is listed as passed.
  /// </summary>
  /// <remarks>
  /// A process counts as started by this run when it runs the sample target and did not exist before the
  /// first test. Another test assembly that debugs the same sample target at the same time would therefore be
  /// reported here: the integration test assemblies must run one after the other, not in parallel.
  /// </remarks>
  [SetUpFixture]
  public class RunFixture
  {
    /// <summary>
    /// Time a process that is already ending gets to disappear before it counts as left behind.
    /// </summary>
    private static readonly TimeSpan ExitGracePeriod = TimeSpan.FromSeconds(5);

    /// <summary>Ids of the watched processes that already ran before the first test.</summary>
    private HashSet<int> _processIdsBeforeRun;

    /// <summary>Executables whose processes the tests of this assembly start.</summary>
    private static IReadOnlyList<string> WatchedExecutables => new[] { SampleTarget.ExecutablePath };

    /// <summary>Remembers the processes that are not started by this run.</summary>
    [OneTimeSetUp]
    public void RecordRunningProcesses()
    {
      _processIdsBeforeRun = new HashSet<int>();
      foreach (Process process in FindProcessesNamedLikeWatchedExecutables())
      {
        using (process)
        {
          _processIdsBeforeRun.Add(process.Id);
        }
      }
    }

    /// <summary>Fails the run when a process started by the tests still exists.</summary>
    [OneTimeTearDown]
    public void AssertNoProcessIsLeftBehind()
    {
      var leftBehind = new List<string>();
      foreach (Process process in FindProcessesNamedLikeWatchedExecutables())
      {
        using (process)
        {
          if (!_processIdsBeforeRun.Contains(process.Id) && RunsWatchedExecutable(process) && !process.WaitForExit(ExitGracePeriod))
          {
            leftBehind.Add($"{process.ProcessName} (process id {process.Id})");
          }
        }
      }

      Assert.That(leftBehind, Is.Empty, "Processes started by the tests are still running.");
    }

    /// <summary>Finds all running processes that have the name of a watched executable.</summary>
    /// <returns>The processes; the caller disposes them.</returns>
    private static List<Process> FindProcessesNamedLikeWatchedExecutables()
    {
      return WatchedExecutables
        .SelectMany(path => Process.GetProcessesByName(Path.GetFileNameWithoutExtension(path)))
        .ToList();
    }

    /// <summary>
    /// Tells whether a process runs one of the watched executables. The name alone is not enough: other
    /// copies of the same program may run on the machine, so the executable path decides.
    /// </summary>
    /// <param name="process">A process with the name of a watched executable.</param>
    /// <returns>True when the process was started from a watched executable.</returns>
    private static bool RunsWatchedExecutable(Process process)
    {
      string path;
      try
      {
        path = process.MainModule?.FileName;
      }
      catch (Win32Exception)
      {
        // The path cannot be read: the process has another bitness than this test host (a 32-bit process
        // cannot inspect a 64-bit one) or is just ending. Neither is a process this run has to account for.
        return false;
      }
      catch (InvalidOperationException)
      {
        // The process has ended in the meantime.
        return false;
      }

      return WatchedExecutables.Any(watched => string.Equals(watched, path, StringComparison.OrdinalIgnoreCase));
    }
  }
}
