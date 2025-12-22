namespace MCP.Debugger.Types.Modules
{
  public class ModuleImport
  {
    public string ModuleName { get; set; }
    public List<ModuleImportFunction> Functions { get; set; } = new List<ModuleImportFunction>();
  }
}
