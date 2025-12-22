namespace MCP.Debugger.Types.Modules
{
  public class ModuleExport
  {
    public string Name { get; set; }
    public string ForwardedName { get; set; }
    public IntPtr VirtualAddress { get; set; }
    public uint Ordinal { get; set; }
  }
}
