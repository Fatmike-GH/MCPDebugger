namespace MCP.Debugger.Types.Modules
{
  public class ModuleImportFunction
  {
    public string Name { get; set; }
    public ushort Ordinal { get; set; }
    public bool ImportByName { get; set; }
    public IntPtr IATEntryAddress { get; set; }
  }
}
