using MCP.Debugger.Types.Modules;

namespace MCP.Server.McpTools.Results
{
  public class ModuleResult
  {
    public ModuleResult(Module module)
    {
      this.ProcessId = module.ProcessId;
      this.ModuleName = module.ModuleName;
      this.ModuleHandle = (ulong)module.ModuleHandle.ToInt64();
      this.BaseAddress = (ulong)module.BaseAddress.ToInt64();
#if X64
      this.FormattedBaseAddress = $"0x{this.BaseAddress:X16}";
#else
      this.FormattedBaseAddress = $"0x{this.BaseAddress:X8}";
#endif
      this.SizeOfImage = SizeOfImage;
    }

    public uint ProcessId { get; private set; }
    public string ModuleName { get; private set; }
    public ulong ModuleHandle { get; private set; }
    public ulong BaseAddress { get; private set; }
    public string FormattedBaseAddress { get; private set; }
    public uint SizeOfImage { get; private set; }
  }
}
