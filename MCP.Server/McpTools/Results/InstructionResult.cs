using MCP.Debugger.Types.Disassembler;

namespace MCP.Server.McpTools.Results
{
  public class InstructionResult
  {
    public InstructionResult(Instruction instruction)
    {
      this.VirtualAdress = (ulong)instruction.VirtualAdress.ToInt64();
#if X64
      //this.FormattedVirtualAddress = $"0x{this.VirtualAdress:X16}";
#else
      //this.FormattedVirtualAddress = $"0x{this.VirtualAdress:X8}";
#endif
      //this.InstructionLength = instruction.InstructionLength;
      this.Disassembly = instruction.Disassembly;
    }

    public ulong VirtualAdress { get; private set; }
    //public string FormattedVirtualAddress { get; private set; }
    //public uint InstructionLength { get; private set; }
    public string Disassembly { get; private set; }
  }
}
