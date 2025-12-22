namespace MCP.Debugger.Types.Disassembler
{
  public class Instruction
  {
    public IntPtr VirtualAdress { get; private set; }
    public UInt32 InstructionLength { get; private set; }
    public string Disassembly { get; private set; }

    public Instruction(IntPtr virtualAdress, UInt32 instructionLength, string disassembly)
    {
      this.VirtualAdress = virtualAdress;
      this.InstructionLength = instructionLength;
      this.Disassembly = disassembly;
    }
  }
}
