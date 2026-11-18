using System.Text.Json;
using MCP.Server.McpTools.Converters;

namespace MCP.Server.Tests.McpTools.Converters
{
  /// <summary>Byte arrays as JSON arrays of numbers, in both directions.</summary>
  public class ByteArrayAsNumbersConverterTests
  {
    /// <summary>Serializer settings with the converter under test as the only converter.</summary>
    private JsonSerializerOptions _options;

    /// <summary>Creates the serializer settings for a test.</summary>
    [SetUp]
    public void CreateOptions()
    {
      _options = new JsonSerializerOptions();
      _options.Converters.Add(new ByteArrayAsNumbersConverter());
    }

    [Test]
    public void Write_Bytes_WritesArrayOfNumbersOnOneLine()
    {
      // Arrange
      byte[] bytes = { 1, 2, 255 };

      // Act
      string json = JsonSerializer.Serialize(bytes, _options);

      // Assert
      Assert.That(json, Is.EqualTo("[1,2,255]"));
    }

    [Test]
    public void Write_EmptyArray_WritesEmptyArray()
    {
      // Act
      string json = JsonSerializer.Serialize(Array.Empty<byte>(), _options);

      // Assert
      Assert.That(json, Is.EqualTo("[]"));
    }

    [Test]
    public void Write_Null_WritesNull()
    {
      // Act
      string json = JsonSerializer.Serialize<byte[]>(null, _options);

      // Assert
      Assert.That(json, Is.EqualTo("null"));
    }

    [Test]
    public void Read_ArrayOfNumbers_ReturnsTheBytes()
    {
      // Act
      byte[] bytes = JsonSerializer.Deserialize<byte[]>("[0,7,255]", _options);

      // Assert
      Assert.That(bytes, Is.EqualTo(new byte[] { 0, 7, 255 }));
    }

    [Test]
    public void Read_Null_ReturnsNull()
    {
      // Act
      byte[] bytes = JsonSerializer.Deserialize<byte[]>("null", _options);

      // Assert
      Assert.That(bytes, Is.Null);
    }

    [Test]
    public void Read_String_ThrowsExpectedArray()
    {
      // Act
      JsonException caught = Assert.Throws<JsonException>(
        () => JsonSerializer.Deserialize<byte[]>("\"AQI=\"", _options));

      // Assert
      Assert.That(caught.Message, Is.EqualTo("Expected array."));
    }

    [TestCase("[256]")]
    [TestCase("[-1]")]
    public void Read_NumberOutsideByteRange_ThrowsExpectedByte(string json)
    {
      // Act
      JsonException caught = Assert.Throws<JsonException>(
        () => JsonSerializer.Deserialize<byte[]>(json, _options));

      // Assert
      Assert.That(caught.Message, Is.EqualTo("Expected byte."));
    }
  }
}
