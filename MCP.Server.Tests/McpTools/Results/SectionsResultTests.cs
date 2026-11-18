using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>The list of sections of a module as the tools report it.</summary>
  public class SectionsResultTests
  {
    [Test]
    public void Constructor_NullList_HasNoSections()
    {
      // Act
      var result = new SectionsResult(null);

      // Assert
      Assert.That(result.Sections, Is.Empty);
    }

    [Test]
    public void Constructor_Sections_MapsEverySectionInOrder()
    {
      // Arrange
      var sections = new List<ModuleSection>
      {
        new ModuleSection { Name = ".text" },
        new ModuleSection { Name = ".data" }
      };

      // Act
      var result = new SectionsResult(sections);

      // Assert
      Assert.That(result.Sections.Select(section => section.Name), Is.EqualTo(new[] { ".text", ".data" }));
    }
  }
}
