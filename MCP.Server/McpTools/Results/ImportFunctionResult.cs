using MCP.Debugger.Types.Modules;

namespace MCP.Server.McpTools.Results
{
  public class ImportFunctionResult
  {
    public ImportFunctionResult(ModuleImportFunction moduleImportFunction)
    {
      this.Name = moduleImportFunction.Name;
      this.Ordinal = moduleImportFunction.Ordinal;
      this.ImportByName = moduleImportFunction.ImportByName;
      this.IATEntryAddress = (ulong)moduleImportFunction.IATEntryAddress.ToInt64();
#if X64
      this.FormattedIATEntryAddress = $"0x{this.IATEntryAddress:X16}";
#else
      this.FormattedIATEntryAddress = $"0x{this.IATEntryAddress:X8}";
#endif
    }

    public string Name { get; set; }
    public ushort Ordinal { get; set; }
    public bool ImportByName { get; set; }
    public ulong IATEntryAddress { get; set; }
    public string FormattedIATEntryAddress { get; set; }
  }
}
