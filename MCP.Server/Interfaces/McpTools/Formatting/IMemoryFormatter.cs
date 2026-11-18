namespace MCP.Server.Interfaces.McpTools.Formatting
{
  /// <summary>Turns memory content into the text a tool answers with.</summary>
  /// <remarks>Threading: thread-safe.</remarks>
  public interface IMemoryFormatter
  {
    /// <summary>
    /// Formats bytes as a JSON array of numbers on one line, for example <c>[77,90]</c>. Never throws.
    /// </summary>
    /// <param name="memory">The bytes; may be empty or null.</param>
    /// <returns>The text; <c>[]</c> for an empty array, <c>null</c> for null.</returns>
    string Format(byte[] memory);
  }
}
