namespace MCP.Debugger.Types.Modules
{
  public class Module
  {
    public uint ProcessId { get; }
    public string ModuleName { get; }
    public IntPtr ModuleHandle { get; }
    public IntPtr BaseAddress { get; }
    public UInt32 SizeOfImage { get; }

    public Module(uint processId, string moduleName, IntPtr moduleHandle, IntPtr baseAddress, UInt32 sizeOfImage)
    {
      this.ProcessId = processId;
      this.ModuleName = moduleName;
      this.ModuleHandle = moduleHandle;
      this.BaseAddress = baseAddress;
      this.SizeOfImage = sizeOfImage;
    }
  }
}
