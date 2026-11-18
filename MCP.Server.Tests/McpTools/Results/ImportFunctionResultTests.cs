using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;
using MCP.Server.Tests.TestHelpers;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>An imported function of a module as the tools report it.</summary>
  public class ImportFunctionResultTests
  {
    /// <summary>Creates the imported function the tests map.</summary>
    /// <returns>An imported function with a different value in every field.</returns>
    private static ModuleImportFunction CreateImportFunction()
    {
      return new ModuleImportFunction
      {
        Name = "ExitProcess",
        Ordinal = 3,
        ImportByName = true,
        IATEntryAddress = new IntPtr(TestAddresses.Sample)
      };
    }

    [Test]
    public void Constructor_ImportFunction_MapsItsFields()
    {
      // Arrange
      ModuleImportFunction importFunction = CreateImportFunction();

      // Act
      var result = new ImportFunctionResult(importFunction);

      // Assert
      Assert.That(result.Name, Is.EqualTo("ExitProcess"));
      Assert.That(result.Ordinal, Is.EqualTo((ushort)3));
      Assert.That(result.ImportByName, Is.True);
      Assert.That(result.IATEntryAddress, Is.EqualTo((ulong)TestAddresses.Sample));
    }

    [Test]
    public void Constructor_ImportFunction_FormatsEntryAddressForThisBitness()
    {
      // Arrange
      ModuleImportFunction importFunction = CreateImportFunction();

      // Act
      var result = new ImportFunctionResult(importFunction);

      // Assert
      Assert.That(result.FormattedIATEntryAddress, Is.EqualTo(TestAddresses.SampleFormatted));
    }
  }
}
