namespace MCP.Debugger.Types.EventArgs
{
  public class BreakpointEventArgs : System.EventArgs
  {
    public uint ProcessId { get; }
    public uint ThreadId { get; }
    public Interop.CONTEXT Context { get; }
    public BreakpointEventArgs(uint processId, uint threadId, Interop.CONTEXT context)
    {
      this.ProcessId = processId;
      this.ThreadId = threadId;
      this.Context = context;
    }
  }
}
