using System.Text.Encodings.Web;
using System.Text.Json;
using MCP.Server.Interfaces.McpTools.Formatting;
using MCP.Server.McpTools.Converters;

namespace MCP.Server.McpTools.Formatting
{
  /// <summary>Formats memory content with the JSON serializer and the converters of the server.</summary>
  public class MemoryFormatter : IMemoryFormatter
  {
    /// <summary>The serializer settings; set up once and only read afterwards.</summary>
    private readonly JsonSerializerOptions _jsonSerializerOptions;

    /// <summary>Creates the formatter with its serializer settings and converters.</summary>
    public MemoryFormatter()
    {
      _jsonSerializerOptions = new JsonSerializerOptions()
      {
        IncludeFields = true,
        WriteIndented = false,
        Encoder = JavaScriptEncoder.UnsafeRelaxedJsonEscaping
      };
      _jsonSerializerOptions.Converters.Add(new IntPtrConverter());
      _jsonSerializerOptions.Converters.Add(new ByteArrayAsNumbersConverter());
    }

    /// <inheritdoc/>
    public string Format(byte[] memory)
    {
      return JsonSerializer.Serialize(memory, _jsonSerializerOptions);
    }
  }
}
