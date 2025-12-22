using MCP.Debugger.Types.Modules;

namespace MCP.Server.McpTools.Results
{
  public class ExportResult
  {
    public ExportResult(ModuleExport moduleExport)
    {
      this.Name = moduleExport.Name;
      this.ForwardedName = moduleExport.ForwardedName;
      this.VirtualAddress = (ulong)moduleExport.VirtualAddress.ToInt64();
#if X64
      this.FormattedVirtualAddress = $"0x{this.VirtualAddress:X16}";
#else
      this.FormattedVirtualAddress = $"0x{this.VirtualAddress:X8}";
#endif
      this.Ordinal = moduleExport.Ordinal;
    }

    public string Name { get; set; }
    public string ForwardedName { get; set; }
    public ulong VirtualAddress { get; set; }
    public string FormattedVirtualAddress { get; set; }
    public uint Ordinal { get; set; }
  }
}
