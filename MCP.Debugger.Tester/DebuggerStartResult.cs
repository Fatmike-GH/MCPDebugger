namespace MCP.Debugger.Tester
{
  public class DebuggerStartResult
  {
    public DebuggerStartResult(uint processId, uint threadId, nint imageBase, nint entryPoint, MCP.Debugger.Types.Interop.CONTEXT context)
    {
      ProcessId = processId;
      ThreadId = threadId;
      ImageBase = imageBase;
      EntryPoint = entryPoint;
      Context = context;
    }

    public uint ProcessId { get; private set; }
    public uint ThreadId { get; private set; }
    public nint ImageBase { get; private set; }
    public nint EntryPoint { get; private set; }
    public MCP.Debugger.Types.Interop.CONTEXT Context { get; private set; }
  }
}
