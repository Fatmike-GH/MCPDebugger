namespace MCP.Debugger.Types.EventArgs
{
  public class ExitProcessEventArgs : System.EventArgs
  {
    public uint ProcessId { get; }

    public ExitProcessEventArgs(uint processId)
    {
      this.ProcessId = processId;
    }
  }
}
