using System.Text.Json;
using ModelContextProtocol.Client;
using ModelContextProtocol.Protocol;

namespace MCP.Server.IntegrationTests.TestHelpers
{
  /// <summary>
  /// One running server process with an MCP client connected to it over stdio - the way a real client uses
  /// the server. Disposing ends the server process, and with it the debuggee of a session that is still open.
  /// </summary>
  public sealed class ServerSession : IAsyncDisposable
  {
    /// <summary>
    /// Upper bound of a test in milliseconds, for the cancellation of every client call. It lies below the
    /// time the server itself waits for a stop of the debuggee, so a hanging test ends by its own bound and
    /// never by the server giving up.
    /// </summary>
    public const int TimeoutMilliseconds = 30000;

    /// <summary>File name of the server executable.</summary>
    private const string ExecutableFileName = "MCP.Server.exe";

    /// <summary>The client connected to the server process.</summary>
    private readonly McpClient _client;

    /// <summary>Wraps a connected client.</summary>
    /// <param name="client">The client; owned by the new session.</param>
    private ServerSession(McpClient client)
    {
      _client = client;
    }

    /// <summary>
    /// Full path of the server under test. The server runs from the output folder of its build, where all
    /// its files lie; the test assembly lies in a subfolder of that folder.
    /// </summary>
    public static string ExecutablePath
    {
      get
      {
        string testDirectory = Path.TrimEndingDirectorySeparator(TestContext.CurrentContext.TestDirectory);
        return Path.Combine(Path.GetDirectoryName(testDirectory), ExecutableFileName);
      }
    }

    /// <summary>Starts the server and connects a client to it (the MCP initialize handshake).</summary>
    /// <param name="cancellationToken">Cancels the start.</param>
    /// <returns>The session with the running server.</returns>
    public static async Task<ServerSession> StartAsync(CancellationToken cancellationToken)
    {
      var transport = new StdioClientTransport(new StdioClientTransportOptions
      {
        Command = ExecutablePath,
        Name = "Integration tests",
      });
      McpClient client = await McpClient.CreateAsync(transport, cancellationToken: cancellationToken);
      return new ServerSession(client);
    }

    /// <summary>Asks the server for its tools.</summary>
    /// <param name="cancellationToken">Cancels the call.</param>
    /// <returns>The names of all tools the server offers.</returns>
    public async Task<IReadOnlyList<string>> ListToolNamesAsync(CancellationToken cancellationToken)
    {
      var tools = await _client.ListToolsAsync(cancellationToken: cancellationToken);
      return tools.Select(tool => tool.Name).ToList();
    }

    /// <summary>Calls a tool of the server.</summary>
    /// <param name="tool">Name of the tool.</param>
    /// <param name="arguments">Arguments by parameter name; empty for a tool without parameters.</param>
    /// <param name="cancellationToken">Cancels the call.</param>
    /// <returns>The result as the server sent it; a failed call is a result marked as error, not an exception.</returns>
    public async Task<CallToolResult> CallAsync(string tool, Dictionary<string, object> arguments, CancellationToken cancellationToken)
    {
      return await _client.CallToolAsync(tool, arguments, cancellationToken: cancellationToken);
    }

    /// <summary>
    /// Returns the text of a tool result. A tool that returns a value sends it as one text block: JSON for an
    /// object, the plain value otherwise, the error message for a failed call.
    /// </summary>
    /// <param name="result">The result of a tool call.</param>
    /// <returns>The text of its single content block.</returns>
    /// <exception cref="InvalidOperationException">The result does not hold exactly one text block.</exception>
    public static string Text(CallToolResult result)
    {
      if (result.Content.Count != 1 || result.Content[0] is not TextContentBlock textBlock)
      {
        throw new InvalidOperationException($"Expected a result with one text block, but it holds {result.Content.Count} block(s).");
      }

      return textBlock.Text;
    }

    /// <summary>Parses the text of a tool result as JSON.</summary>
    /// <param name="result">The result of a tool call that returns an object.</param>
    /// <returns>The root element; it stays valid without further disposal.</returns>
    /// <exception cref="InvalidOperationException">The result does not hold exactly one text block.</exception>
    /// <exception cref="JsonException">The text is not JSON.</exception>
    public static JsonElement ParseJson(CallToolResult result)
    {
      using JsonDocument document = JsonDocument.Parse(Text(result));
      return document.RootElement.Clone();
    }

    /// <summary>Disconnects the client, which ends the server process.</summary>
    /// <returns>A task that completes when the client is disposed.</returns>
    public ValueTask DisposeAsync()
    {
      return _client.DisposeAsync();
    }
  }
}
