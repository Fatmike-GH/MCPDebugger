using MCP.Debugger.Types.Interop;
using MCP.Server.McpTools.Results;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>A stop of the debuggee as the tools report it.</summary>
  public class BreakpointResultTests
  {
    [Test]
    public void Constructor_Context_ReportsTheContextOfTheStop()
    {
      // Arrange
      var context = new CONTEXT { EFlags = 0x246 };

      // Act
      var result = new BreakpointResult(context, null);

      // Assert
      Assert.That(result.Context.EFlags, Is.EqualTo(0x246u));
    }
  }
}
