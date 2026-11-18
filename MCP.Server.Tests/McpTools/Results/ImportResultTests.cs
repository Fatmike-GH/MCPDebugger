using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>The functions a module imports from one other module, as the tools report them.</summary>
  public class ImportResultTests
  {
    [Test]
    public void Constructor_Import_MapsModuleNameAndEveryFunctionInOrder()
    {
      // Arrange
      var import = new ModuleImport
      {
        ModuleName = "kernel32.dll",
        Functions = new List<ModuleImportFunction>
        {
          new ModuleImportFunction { Name = "ExitProcess" },
          new ModuleImportFunction { Name = "GetStdHandle" }
        }
      };

      // Act
      var result = new ImportResult(import);

      // Assert
      Assert.That(result.ModuleName, Is.EqualTo("kernel32.dll"));
      Assert.That(result.Functions.Select(function => function.Name), Is.EqualTo(new[] { "ExitProcess", "GetStdHandle" }));
    }

    [Test]
    public void Constructor_NullFunctionList_HasNoFunctions()
    {
      // Arrange
      var import = new ModuleImport { ModuleName = "kernel32.dll", Functions = null };

      // Act
      var result = new ImportResult(import);

      // Assert
      Assert.That(result.Functions, Is.Empty);
    }
  }
}
