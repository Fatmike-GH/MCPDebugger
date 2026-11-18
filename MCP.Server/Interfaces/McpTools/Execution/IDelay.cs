namespace MCP.Server.Interfaces.McpTools.Execution
{
  /// <summary>
  /// Waits for a time span. A time limit is built on this interface instead of on a timer of the runtime, so
  /// that a test can let the limit run out without waiting.
  /// </summary>
  /// <remarks>Threading: thread-safe. Every call returns a task of its own.</remarks>
  public interface IDelay
  {
    /// <summary>Waits until a time span has elapsed.</summary>
    /// <param name="duration">The time to wait.</param>
    /// <returns>A task that completes when the time has elapsed. It never faults.</returns>
    Task WaitAsync(TimeSpan duration);
  }
}
