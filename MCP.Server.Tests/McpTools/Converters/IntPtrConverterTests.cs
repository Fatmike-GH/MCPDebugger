using System.Text.Json;
using MCP.Server.McpTools.Converters;

namespace MCP.Server.Tests.McpTools.Converters
{
  /// <summary>
  /// Pointer-sized integers as JSON numbers, in both directions. The values stay within 32 bits, where both
  /// bitnesses behave the same.
  /// </summary>
  public class IntPtrConverterTests
  {
    /// <summary>Serializer settings with the converter under test as the only converter.</summary>
    private JsonSerializerOptions _options;

    /// <summary>Creates the serializer settings for a test.</summary>
    [SetUp]
    public void CreateOptions()
    {
      _options = new JsonSerializerOptions();
      _options.Converters.Add(new IntPtrConverter());
    }

    [TestCase(42, "42")]
    [TestCase(-1, "-1")]
    public void Write_Value_WritesItAsNumber(int value, string expectedJson)
    {
      // Act
      string json = JsonSerializer.Serialize(new IntPtr(value), _options);

      // Assert
      Assert.That(json, Is.EqualTo(expectedJson));
    }

    [Test]
    public void Read_Number_ReturnsItsValue()
    {
      // Act
      IntPtr value = JsonSerializer.Deserialize<IntPtr>("42", _options);

      // Assert
      Assert.That(value, Is.EqualTo(new IntPtr(42)));
    }
  }
}
