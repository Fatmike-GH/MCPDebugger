using MCP.Server.Base;
using ModelContextProtocol;

namespace MCP.Server.Tests.Base
{
  /// <summary>
  /// How the base class of the tools passes on what a tool body does: its result, and its exceptions as
  /// errors an MCP client can show.
  /// </summary>
  public class McpToolBaseTests
  {
    /// <summary>The tool whose protected members are called.</summary>
    private ExposedTool _tool;

    /// <summary>Creates the tool for a test.</summary>
    [SetUp]
    public void CreateTool()
    {
      _tool = new ExposedTool();
    }

    [Test]
    public void Execute_ActionCompletes_RunsTheAction()
    {
      // Arrange
      int runs = 0;
      Action action = () => runs++;

      // Act
      _tool.Run(action);

      // Assert
      Assert.That(runs, Is.EqualTo(1));
    }

    [Test]
    public void Execute_ActionThrowsMcpException_ThrowsTheSameException()
    {
      // Arrange
      var thrown = new McpException("tool error");
      Action action = () => throw thrown;

      // Act
      McpException caught = Assert.Throws<McpException>(() => _tool.Run(action));

      // Assert
      Assert.That(caught, Is.SameAs(thrown));
    }

    [Test]
    public void Execute_ActionThrowsOtherException_ThrowsMcpExceptionWithItsMessage()
    {
      // Arrange
      Action action = () => throw new InvalidOperationException("boom");

      // Act
      McpException caught = Assert.Throws<McpException>(() => _tool.Run(action));

      // Assert
      Assert.That(caught.Message, Is.EqualTo("Unexpected internal MCP server error! Internal exception: boom"));
    }

    [Test]
    public void Execute_FunctionCompletes_ReturnsItsValue()
    {
      // Act
      int value = _tool.Run(() => 42);

      // Assert
      Assert.That(value, Is.EqualTo(42));
    }

    [Test]
    public void Execute_FunctionThrowsMcpException_ThrowsTheSameException()
    {
      // Arrange
      var thrown = new McpException("tool error");
      Func<int> function = () => throw thrown;

      // Act
      McpException caught = Assert.Throws<McpException>(() => _tool.Run(function));

      // Assert
      Assert.That(caught, Is.SameAs(thrown));
    }

    [Test]
    public void Execute_FunctionThrowsOtherException_ThrowsMcpExceptionWithItsMessage()
    {
      // Arrange
      Func<int> function = () => throw new InvalidOperationException("boom");

      // Act
      McpException caught = Assert.Throws<McpException>(() => _tool.Run(function));

      // Assert
      Assert.That(caught.Message, Is.EqualTo("Unexpected internal MCP server error! Internal exception: boom"));
    }

    /// <summary>
    /// A tool class without tools. The base class is abstract and its members are protected, so the tests
    /// reach them through this class.
    /// </summary>
    private sealed class ExposedTool : McpToolBase
    {
      /// <summary>Runs a tool body without a result the way a tool does.</summary>
      /// <param name="action">The tool body.</param>
      public void Run(Action action)
      {
        Execute(action);
      }

      /// <summary>Runs a tool body with a result the way a tool does.</summary>
      /// <typeparam name="T">Type of the result.</typeparam>
      /// <param name="function">The tool body.</param>
      /// <returns>The result of the tool body.</returns>
      public T Run<T>(Func<T> function)
      {
        return Execute(function);
      }
    }
  }
}
