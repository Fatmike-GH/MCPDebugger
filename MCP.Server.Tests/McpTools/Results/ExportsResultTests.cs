using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>The list of exports of a module as the tools report it.</summary>
  public class ExportsResultTests
  {
    [Test]
    public void Constructor_NullList_HasNoExports()
    {
      // Act
      var result = new ExportsResult(null);

      // Assert
      Assert.That(result.Exports, Is.Empty);
    }

    [Test]
    public void Constructor_Exports_MapsEveryExportInOrder()
    {
      // Arrange
      var exports = new List<ModuleExport>
      {
        new ModuleExport { Name = "First" },
        new ModuleExport { Name = "Second" }
      };

      // Act
      var result = new ExportsResult(exports);

      // Assert
      Assert.That(result.Exports.Select(export => export.Name), Is.EqualTo(new[] { "First", "Second" }));
    }
  }
}
