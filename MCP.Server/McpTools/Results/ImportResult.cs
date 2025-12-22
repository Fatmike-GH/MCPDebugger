using MCP.Debugger.Types.Modules;

namespace MCP.Server.McpTools.Results
{
  public class ImportResult
  {
    public ImportResult(ModuleImport moduleImport)
    {
      this.ModuleName = moduleImport.ModuleName;
      if (moduleImport.Functions == null)
      {
        Functions = new List<ImportFunctionResult>();
      }
      else
      {
        Functions = moduleImport.Functions
                    .Select(e => new ImportFunctionResult(e))
                    .ToList();
      }
    }

    public string ModuleName { get; private set; }
    public List<ImportFunctionResult> Functions { get; private set; }
  }
}
