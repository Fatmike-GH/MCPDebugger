namespace MCP.Server.McpTools.Results
{
  public class ContextResult
  {
    public ContextResult(MCP.Debugger.Types.Interop.CONTEXT context)
    {
#if X64
      this.SegCs = context.SegCs;
      this.SegDs = context.SegDs;
      this.SegEs = context.SegEs;
      this.SegFs = context.SegFs;
      this.SegGs = context.SegGs;
      this.SegSs = context.SegSs;
      this.EFlags = context.EFlags;
      this.Rax = context.Rax;
      this.Rcx = context.Rcx;
      this.Rdx = context.Rdx;
      this.Rbx = context.Rbx;
      this.Rsp = context.Rsp;
      this.Rbp = context.Rbp;
      this.Rsi = context.Rsi;
      this.Rdi = context.Rdi;
      this.R8 = context.R8;
      this.R9 = context.R9;
      this.R10 = context.R10;
      this.R11 = context.R11;
      this.R12 = context.R12;
      this.R13 = context.R13;
      this.R14 = context.R14;
      this.R15 = context.R15;
      this.Rip = context.Rip;
#else
      this.SegGs = context.SegGs;
      this.SegFs = context.SegFs;
      this.SegEs = context.SegEs;
      this.SegDs = context.SegDs;
      this.Edi = context.Edi;
      this.Esi = context.Esi;
      this.Ebx = context.Ebx;
      this.Edx = context.Edx;
      this.Ecx = context.Ecx;
      this.Eax = context.Eax;
      this.Ebp = context.Ebp;
      this.Eip = context.Eip;
      this.SegCs = context.SegCs;
      this.EFlags = context.EFlags;
      this.Esp = context.Esp;
      this.SegSs = context.SegSs;
#endif

    }
#if X64
    public ushort SegCs { get; private set; }
    public ushort SegDs { get; private set; }
    public ushort SegEs { get; private set; }
    public ushort SegFs { get; private set; }
    public ushort SegGs { get; private set; }
    public ushort SegSs { get; private set; }
    public uint EFlags { get; private set; }
    public ulong Rax { get; private set; }
    public ulong Rcx { get; private set; }
    public ulong Rdx { get; private set; }
    public ulong Rbx { get; private set; }
    public ulong Rsp { get; private set; }
    public ulong Rbp { get; private set; }
    public ulong Rsi { get; private set; }
    public ulong Rdi { get; private set; }
    public ulong R8 { get; private set; }
    public ulong R9 { get; private set; }
    public ulong R10 { get; private set; }
    public ulong R11 { get; private set; }
    public ulong R12 { get; private set; }
    public ulong R13 { get; private set; }
    public ulong R14 { get; private set; }
    public ulong R15 { get; private set; }
    public ulong Rip { get; private set; }
#else
    public uint SegGs { get; private set; }
    public uint SegFs { get; private set; }
    public uint SegEs { get; private set; }
    public uint SegDs { get; private set; }
    public uint Edi { get; private set; }
    public uint Esi { get; private set; }
    public uint Ebx { get; private set; }
    public uint Edx { get; private set; }
    public uint Ecx { get; private set; }
    public uint Eax { get; private set; }
    public uint Ebp { get; private set; }
    public uint Eip { get; private set; }
    public uint SegCs { get; private set; }
    public uint EFlags { get; private set; }
    public uint Esp { get; private set; }
    public uint SegSs { get; private set; }
#endif
  }
}
