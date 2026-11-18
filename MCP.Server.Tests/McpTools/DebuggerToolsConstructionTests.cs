using MCP.Debugger;
using MCP.Debugger.Types.EventArgs;
using MCP.Server.Interfaces.McpTools.Execution;
using MCP.Server.Interfaces.McpTools.Formatting;
using MCP.Server.McpTools;
using Moq;

namespace MCP.Server.Tests.McpTools
{
  /// <summary>Creating the tool class: it needs nothing but its three collaborators, and it needs all of them.</summary>
  public class DebuggerToolsConstructionTests
  {
    /// <summary>The debugger service the tools are created on.</summary>
    private Mock<IDebuggerService> _debuggerService;

    /// <summary>The execution control the tools are created with.</summary>
    private Mock<IExecutionControl> _executionControl;

    /// <summary>The memory formatter the tools are created with.</summary>
    private Mock<IMemoryFormatter> _memoryFormatter;

    /// <summary>Creates the collaborators for a test.</summary>
    [SetUp]
    public void CreateCollaborators()
    {
      _debuggerService = new Mock<IDebuggerService>();
      _executionControl = new Mock<IExecutionControl>();
      _memoryFormatter = new Mock<IMemoryFormatter>();
    }

    [Test]
    public void Constructor_MockedCollaborators_CreatesTheToolsAndListensForTheProcessExit()
    {
      // Act
      var tools = new DebuggerTools(_debuggerService.Object, _executionControl.Object, _memoryFormatter.Object);

      // Assert
      Assert.That(tools, Is.Not.Null);
      _debuggerService.VerifyAdd(service => service.ExitProcess += It.IsAny<EventHandler<ExitProcessEventArgs>>(), Times.Once());
    }

    [Test]
    public void Constructor_NullDebuggerService_ThrowsArgumentNullException()
    {
      // Act
      ArgumentNullException caught = Assert.Throws<ArgumentNullException>(
        () => new DebuggerTools(null, _executionControl.Object, _memoryFormatter.Object));

      // Assert
      Assert.That(caught.ParamName, Is.EqualTo("debuggerService"));
    }

    [Test]
    public void Constructor_NullExecutionControl_ThrowsArgumentNullException()
    {
      // Act
      ArgumentNullException caught = Assert.Throws<ArgumentNullException>(
        () => new DebuggerTools(_debuggerService.Object, null, _memoryFormatter.Object));

      // Assert
      Assert.That(caught.ParamName, Is.EqualTo("executionControl"));
    }

    [Test]
    public void Constructor_NullMemoryFormatter_ThrowsArgumentNullException()
    {
      // Act
      ArgumentNullException caught = Assert.Throws<ArgumentNullException>(
        () => new DebuggerTools(_debuggerService.Object, _executionControl.Object, null));

      // Assert
      Assert.That(caught.ParamName, Is.EqualTo("memoryFormatter"));
    }
  }
}
