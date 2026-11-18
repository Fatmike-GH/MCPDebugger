using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;
using MCP.Server.Tests.TestHelpers;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>An exported function of a module as the tools report it.</summary>
  public class ExportResultTests
  {
    /// <summary>Creates the export the tests map.</summary>
    /// <returns>An export with a different value in every field.</returns>
    private static ModuleExport CreateExport()
    {
      return new ModuleExport
      {
        Name = "CreateFileW",
        ForwardedName = "api-ms-win-core-file.CreateFileW",
        VirtualAddress = new IntPtr(TestAddresses.Sample),
        Ordinal = 7
      };
    }

    [Test]
    public void Constructor_Export_MapsItsFields()
    {
      // Arrange
      ModuleExport export = CreateExport();

      // Act
      var result = new ExportResult(export);

      // Assert
      Assert.That(result.Name, Is.EqualTo("CreateFileW"));
      Assert.That(result.ForwardedName, Is.EqualTo("api-ms-win-core-file.CreateFileW"));
      Assert.That(result.VirtualAddress, Is.EqualTo((ulong)TestAddresses.Sample));
      Assert.That(result.Ordinal, Is.EqualTo(7u));
    }

    [Test]
    public void Constructor_Export_FormatsVirtualAddressForThisBitness()
    {
      // Arrange
      ModuleExport export = CreateExport();

      // Act
      var result = new ExportResult(export);

      // Assert
      Assert.That(result.FormattedVirtualAddress, Is.EqualTo(TestAddresses.SampleFormatted));
    }
  }
}
