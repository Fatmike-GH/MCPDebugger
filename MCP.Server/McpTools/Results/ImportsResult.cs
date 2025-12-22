using MCP.Debugger.Types.Modules;

namespace MCP.Server.McpTools.Results
{
  public class ImportsResult
  {
    public ImportsResult(IEnumerable<ModuleImport> moduleImports)
    {
      if (moduleImports == null)
      {
        Imports = new List<ImportResult>();
      }
      else
      {
        Imports = moduleImports
                  .Select(e => new ImportResult(e))
                  .ToList();
      }
    }

    public IEnumerable<ImportResult> Imports { get; private set; }
  }
}
