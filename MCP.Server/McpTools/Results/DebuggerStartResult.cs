using MCP.Debugger.Types.Disassembler;

namespace MCP.Server.McpTools.Results
{
  public class DebuggerStartResult
  {
    public DebuggerStartResult(uint processId,
                               uint threadId,
                               ulong imageBase,
                               ulong entryPoint,
                               MCP.Debugger.Types.Interop.CONTEXT context,
                               Instruction instruction)
    {
      ProcessId = processId;
      ThreadId = threadId;
      ImageBase = imageBase;
      EntryPoint = entryPoint;
      Context = new ContextResult(context);
      //CurrrentInstruction = new InstructionResult(instruction);
    }

    public uint ProcessId { get; private set; }
    public uint ThreadId { get; private set; }
    public ulong ImageBase { get; private set; }
    public ulong EntryPoint { get; private set; }
    public ContextResult Context { get; private set; }
    //public InstructionResult CurrrentInstruction { get; private set; }
  }
}
