using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>The list of imports of a module as the tools report it.</summary>
  public class ImportsResultTests
  {
    [Test]
    public void Constructor_NullList_HasNoImports()
    {
      // Act
      var result = new ImportsResult(null);

      // Assert
      Assert.That(result.Imports, Is.Empty);
    }

    [Test]
    public void Constructor_Imports_MapsEveryImportInOrder()
    {
      // Arrange
      var imports = new List<ModuleImport>
      {
        new ModuleImport { ModuleName = "kernel32.dll" },
        new ModuleImport { ModuleName = "user32.dll" }
      };

      // Act
      var result = new ImportsResult(imports);

      // Assert
      Assert.That(result.Imports.Select(import => import.ModuleName), Is.EqualTo(new[] { "kernel32.dll", "user32.dll" }));
    }
  }
}
