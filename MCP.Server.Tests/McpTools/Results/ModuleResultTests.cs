using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;
using MCP.Server.Tests.TestHelpers;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>A loaded module as the tools report it.</summary>
  public class ModuleResultTests
  {
    [Test]
    public void Constructor_Module_MapsItsFields()
    {
      // Arrange
      var module = new Module(4711, "example.exe", new IntPtr(0x500000), new IntPtr(TestAddresses.Sample), 0x1000);

      // Act
      var result = new ModuleResult(module);

      // Assert
      Assert.That(result.ProcessId, Is.EqualTo(4711u));
      Assert.That(result.ModuleName, Is.EqualTo("example.exe"));
      Assert.That(result.ModuleHandle, Is.EqualTo(0x500000ul));
      Assert.That(result.BaseAddress, Is.EqualTo((ulong)TestAddresses.Sample));
    }

    [Test]
    public void Constructor_Module_FormatsBaseAddressForThisBitness()
    {
      // Arrange
      var module = new Module(4711, "example.exe", new IntPtr(0x500000), new IntPtr(TestAddresses.Sample), 0x1000);

      // Act
      var result = new ModuleResult(module);

      // Assert
      Assert.That(result.FormattedBaseAddress, Is.EqualTo(TestAddresses.SampleFormatted));
    }
  }
}
