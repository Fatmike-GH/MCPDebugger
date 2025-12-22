namespace MCP.Debugger.Types.Modules
{
  public class ModuleSection
  {
    public string Name { get; set; }
    public IntPtr BaseAddress { get; set; }
    public uint VirtualSize { get; set; }
    public uint Characteristics { get; set; }
  }
}
