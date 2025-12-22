using MCP.Debugger.Types.Modules;

namespace MCP.Server.McpTools.Results
{
  public class SectionResult
  {
    public SectionResult(ModuleSection moduleSection)
    {
      this.Name = moduleSection.Name;
      this.BaseAddress = (ulong)moduleSection.BaseAddress.ToInt64();
#if X64
      this.FormattedBaseAddress = $"0x{this.BaseAddress:X16}";
#else
      this.FormattedBaseAddress = $"0x{this.BaseAddress:X8}";
#endif
      this.VirtualSize = moduleSection.VirtualSize;
      this.Characteristics = moduleSection.Characteristics;
    }

    public string Name { get; set; }
    public ulong BaseAddress { get; set; }
    public string FormattedBaseAddress { get; set; }
    public uint VirtualSize { get; set; }
    public uint Characteristics { get; set; }
  }
}
