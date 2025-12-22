using MCP.Debugger.Types.Disassembler;

namespace MCP.Server.McpTools.Results
{
  public class BreakpointResult
  {
    public BreakpointResult(MCP.Debugger.Types.Interop.CONTEXT context,
                            Instruction instruction)
    {
      this.Context = new ContextResult(context);
      //this.CurrrentInstruction = new InstructionResult(instruction);
    }

    public ContextResult Context { get; private set; }
    //public InstructionResult CurrrentInstruction { get; private set; }
  }
}
