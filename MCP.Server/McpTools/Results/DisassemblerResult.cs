using MCP.Debugger.Types.Disassembler;

namespace MCP.Server.McpTools.Results
{
  public class DisassemblerResult
  {
    public DisassemblerResult(IEnumerable<Instruction> instructions)
    {
      if (instructions == null)
      {
        Instructions = new List<InstructionResult>();
      }
      else
      {
        Instructions = instructions
             .Select(e => new InstructionResult(e))
             .ToList();
      }
    }

    public IEnumerable<InstructionResult> Instructions { get; private set; }
  }
}
