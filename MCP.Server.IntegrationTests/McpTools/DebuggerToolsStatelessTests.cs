using MCP.Server.IntegrationTests.TestHelpers;
using ModelContextProtocol.Protocol;

namespace MCP.Server.IntegrationTests.McpTools
{
  /// <summary>
  /// What the server answers without a debugging session: its tool list and the number converter. Neither
  /// test changes the state of the server, so they share one server process. The fixture set-up arranges that
  /// server, so a test that needs nothing else has no arrange step of its own.
  /// </summary>
  public class DebuggerToolsStatelessTests
  {
    /// <summary>The shared server.</summary>
    private ServerSession _server;

    /// <summary>Starts the shared server, once for all tests of the fixture.</summary>
    /// <returns>A task that completes when the client is connected to the server.</returns>
    [OneTimeSetUp]
    public async Task StartSharedServerAsync()
    {
      using var timeout = new CancellationTokenSource(ServerSession.TimeoutMilliseconds);
      _server = await ServerSession.StartAsync(timeout.Token);
    }

    /// <summary>Ends the shared server after the last test of the fixture.</summary>
    /// <returns>A task that completes when the server process has been ended.</returns>
    [OneTimeTearDown]
    public async Task EndSharedServerAsync()
    {
      if (_server != null)
      {
        await _server.DisposeAsync();
      }
    }

    [Test, CancelAfter(ServerSession.TimeoutMilliseconds)]
    public async Task ListTools_ReturnsTheSeventeenDebuggerTools(CancellationToken cancellationToken)
    {
      // Act
      IReadOnlyList<string> toolNames = await _server.ListToolNamesAsync(cancellationToken);

      // Assert
      Assert.That(toolNames, Is.EquivalentTo(ToolNames.All));
      Assert.That(toolNames, Has.Count.EqualTo(17));
    }

    [Test, CancelAfter(ServerSession.TimeoutMilliseconds)]
    public async Task HexToDec_WithoutSession_ReturnsDecimalValue(CancellationToken cancellationToken)
    {
      // Arrange
      var arguments = new Dictionary<string, object> { ["hexNumber"] = "FF" };

      // Act
      CallToolResult result = await _server.CallAsync(ToolNames.HexToDecConverter, arguments, cancellationToken);

      // Assert
      Assert.That(result.IsError, Is.Not.True);
      Assert.That(ServerSession.Text(result), Is.EqualTo("255"));
    }
  }
}
