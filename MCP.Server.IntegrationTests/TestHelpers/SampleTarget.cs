namespace MCP.Server.IntegrationTests.TestHelpers
{
  /// <summary>
  /// Locates the sample debuggee the tests let the server debug. The executable is used where it lies in the
  /// repository and is never copied: started from another path, Windows loads an additional module into the
  /// process and the module list differs.
  /// </summary>
  public static class SampleTarget
  {
#if X64
    /// <summary>Folder suffix of the sample target that has the bitness of this test assembly.</summary>
    private const string Bitness = "x64";
#else
    /// <summary>Folder suffix of the sample target that has the bitness of this test assembly.</summary>
    private const string Bitness = "x86";
#endif

    /// <summary>The file that marks the repository root.</summary>
    private const string SolutionFileName = "MCPDebugger.slnx";

    /// <summary>File name of the sample target; also its module name in a module list.</summary>
    public const string FileName = "Example.exe";

    /// <summary>
    /// Full path of the sample target that has the bitness of this test assembly - the only one a server of
    /// the same build can debug.
    /// </summary>
    public static string ExecutablePath => Path.Combine(FindRepositoryRoot(), "Examples", "Example_" + Bitness, FileName);

    /// <summary>
    /// Finds the repository root: the nearest parent folder of the test assembly that holds the solution file.
    /// </summary>
    /// <returns>The full path of the repository root.</returns>
    /// <exception cref="DirectoryNotFoundException">No parent folder holds the solution file.</exception>
    private static string FindRepositoryRoot()
    {
      string testDirectory = TestContext.CurrentContext.TestDirectory;
      for (var folder = new DirectoryInfo(testDirectory); folder != null; folder = folder.Parent)
      {
        if (File.Exists(Path.Combine(folder.FullName, SolutionFileName)))
        {
          return folder.FullName;
        }
      }

      throw new DirectoryNotFoundException($"No parent folder of '{testDirectory}' contains '{SolutionFileName}'.");
    }
  }
}
