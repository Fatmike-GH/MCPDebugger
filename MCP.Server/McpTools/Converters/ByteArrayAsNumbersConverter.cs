using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace MCP.Server.McpTools.Converters
{
  public class ByteArrayAsNumbersConverter : JsonConverter<byte[]>
  {
    public override byte[] Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
      if (reader.TokenType == JsonTokenType.Null)
        return null;

      if (reader.TokenType != JsonTokenType.StartArray)
        throw new JsonException("Expected array.");

      var list = new List<byte>();

      while (reader.Read())
      {
        if (reader.TokenType == JsonTokenType.EndArray)
          return list.ToArray();

        if (!reader.TryGetByte(out var b))
          throw new JsonException("Expected byte.");

        list.Add(b);
      }

      throw new JsonException("Unexpected end of JSON.");
    }

    public override void Write(Utf8JsonWriter writer, byte[] value, JsonSerializerOptions options)
    {
      if (value == null)
      {
        writer.WriteNullValue();
        return;
      }

      // Use CompactWriter to write array in a single row
      using var buffer = new MemoryStream();
      using (var compactWriter = new Utf8JsonWriter(buffer, new JsonWriterOptions { Indented = false }))
      {
        compactWriter.WriteStartArray();
        foreach (var b in value)
          compactWriter.WriteNumberValue(b);
        compactWriter.WriteEndArray();
      }

      writer.WriteRawValue(Encoding.UTF8.GetString(buffer.ToArray()));
    }
  }
}
