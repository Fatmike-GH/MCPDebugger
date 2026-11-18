using MCP.Debugger.Types.Interop;
using MCP.Server.McpTools.Results;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>A started debugging session as the tools report it.</summary>
  public class DebuggerStartResultTests
  {
    [Test]
    public void Constructor_SessionData_MapsItsFieldsAndTheContext()
    {
      // Arrange
      var context = new CONTEXT { EFlags = 0x246 };

      // Act
      var result = new DebuggerStartResult(4711, 12, 0x400000, 0x401000, context, null);

      // Assert
      Assert.That(result.ProcessId, Is.EqualTo(4711u));
      Assert.That(result.ThreadId, Is.EqualTo(12u));
      Assert.That(result.ImageBase, Is.EqualTo(0x400000ul));
      Assert.That(result.EntryPoint, Is.EqualTo(0x401000ul));
      Assert.That(result.Context.EFlags, Is.EqualTo(0x246u));
    }
  }
}
