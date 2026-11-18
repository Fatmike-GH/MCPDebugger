using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>The list of loaded modules as the tools report it.</summary>
  public class ModulesResultTests
  {
    [Test]
    public void Constructor_NullList_HasNoModules()
    {
      // Act
      var result = new ModulesResult(null);

      // Assert
      Assert.That(result.Modules, Is.Empty);
    }

    [Test]
    public void Constructor_Modules_MapsEveryModuleInOrder()
    {
      // Arrange
      var modules = new List<Module>
      {
        new Module(1, "first.exe", new IntPtr(0x1000), new IntPtr(0x1000), 0),
        new Module(1, "second.dll", new IntPtr(0x2000), new IntPtr(0x2000), 0)
      };

      // Act
      var result = new ModulesResult(modules);

      // Assert
      Assert.That(result.Modules.Select(module => module.ModuleName), Is.EqualTo(new[] { "first.exe", "second.dll" }));
    }
  }
}
