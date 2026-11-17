namespace MCP.Debugger.IntegrationTests.TestHelpers
{
  /// <summary>
  /// Locates the sample debuggee the tests run against. The executable is used where it lies in the repository
  /// and is never copied: started from another path, Windows loads an additional module into the process and
  /// the module list differs.
  /// </summary>
  public static class SampleTarget
  {
#if X64
    /// <summary>Folder suffix of the sample target that has the bitness of this test assembly.</summary>
    private const string Bitness = "x64";

    /// <summary>Folder suffix of the sample target that has the other bitness.</summary>
    private const string OtherBitness = "x86";
#else
    /// <summary>Folder suffix of the sample target that has the bitness of this test assembly.</summary>
    private const string Bitness = "x86";

    /// <summary>Folder suffix of the sample target that has the other bitness.</summary>
    private const string OtherBitness = "x64";
#endif

    /// <summary>The file that marks the repository root.</summary>
    private const string SolutionFileName = "MCPDebugger.slnx";

    /// <summary>Folder of the sample targets, relative to the repository root.</summary>
    private const string ExamplesFolder = "Examples";

    /// <summary>File name of the sample target; also its module name in a module list.</summary>
    public const string FileName = "Example.exe";

    /// <summary>Full path of the sample target that has the bitness of this test assembly.</summary>
    public static string ExecutablePath => PathFor(Bitness);

    /// <summary>Full path of the sample target of the other bitness, which the debugger must refuse.</summary>
    public static string OtherBitnessExecutablePath => PathFor(OtherBitness);

    /// <summary>Full path of an executable that does not exist.</summary>
    public static string MissingExecutablePath => Path.Combine(FindRepositoryRoot(), ExamplesFolder, "DoesNotExist.exe");

    /// <summary>Builds the path of the sample target in the folder with the given bitness suffix.</summary>
    /// <param name="bitness">Folder suffix, <c>x64</c> or <c>x86</c>.</param>
    /// <returns>The full path of the executable.</returns>
    private static string PathFor(string bitness)
    {
      return Path.Combine(FindRepositoryRoot(), ExamplesFolder, "Example_" + bitness, FileName);
    }

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
