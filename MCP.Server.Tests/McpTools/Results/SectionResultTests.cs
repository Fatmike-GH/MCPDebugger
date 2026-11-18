using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;
using MCP.Server.Tests.TestHelpers;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>A section of a module as the tools report it.</summary>
  public class SectionResultTests
  {
    /// <summary>Creates the section the tests map.</summary>
    /// <returns>A section with a different value in every field.</returns>
    private static ModuleSection CreateSection()
    {
      return new ModuleSection
      {
        Name = ".text",
        BaseAddress = new IntPtr(TestAddresses.Sample),
        VirtualSize = 0x2000,
        Characteristics = 0x60000020
      };
    }

    [Test]
    public void Constructor_Section_MapsItsFields()
    {
      // Arrange
      ModuleSection section = CreateSection();

      // Act
      var result = new SectionResult(section);

      // Assert
      Assert.That(result.Name, Is.EqualTo(".text"));
      Assert.That(result.BaseAddress, Is.EqualTo((ulong)TestAddresses.Sample));
      Assert.That(result.VirtualSize, Is.EqualTo(0x2000u));
      Assert.That(result.Characteristics, Is.EqualTo(0x60000020u));
    }

    [Test]
    public void Constructor_Section_FormatsBaseAddressForThisBitness()
    {
      // Arrange
      ModuleSection section = CreateSection();

      // Act
      var result = new SectionResult(section);

      // Assert
      Assert.That(result.FormattedBaseAddress, Is.EqualTo(TestAddresses.SampleFormatted));
    }
  }
}
