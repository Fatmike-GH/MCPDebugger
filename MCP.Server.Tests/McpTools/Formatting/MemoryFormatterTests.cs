using MCP.Server.McpTools.Formatting;

namespace MCP.Server.Tests.McpTools.Formatting
{
  /// <summary>The text the server answers with for memory content.</summary>
  public class MemoryFormatterTests
  {
    /// <summary>The formatter under test.</summary>
    private MemoryFormatter _formatter;

    /// <summary>Creates the formatter for a test.</summary>
    [SetUp]
    public void CreateFormatter()
    {
      _formatter = new MemoryFormatter();
    }

    [Test]
    public void Format_Bytes_ReturnsJsonArrayOfNumbersOnOneLine()
    {
      // Arrange
      byte[] memory = { 1, 2, 255 };

      // Act
      string text = _formatter.Format(memory);

      // Assert
      Assert.That(text, Is.EqualTo("[1,2,255]"));
    }

    [Test]
    public void Format_EmptyArray_ReturnsEmptyJsonArray()
    {
      // Act
      string text = _formatter.Format(Array.Empty<byte>());

      // Assert
      Assert.That(text, Is.EqualTo("[]"));
    }

    [Test]
    public void Format_Null_ReturnsJsonNull()
    {
      // Act
      string text = _formatter.Format(null);

      // Assert
      Assert.That(text, Is.EqualTo("null"));
    }
  }
}
