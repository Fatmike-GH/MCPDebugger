using MCP.Server.Interfaces.McpTools.Execution;

namespace MCP.Server.McpTools.Execution
{
  /// <summary>The wait of the running server: a timer of the runtime.</summary>
  public class TaskDelay : IDelay
  {
    /// <inheritdoc/>
    public Task WaitAsync(TimeSpan duration)
    {
      return Task.Delay(duration);
    }
  }
}
