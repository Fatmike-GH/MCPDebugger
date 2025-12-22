using MCP.Debugger.Types.Modules;

namespace MCP.Server.McpTools.Results
{
  public class ExportsResult
  {
    public ExportsResult(IEnumerable<ModuleExport> moduleExports)
    {
      if (moduleExports == null)
      {
        Exports = new List<ExportResult>();
      }
      else
      {
        Exports = moduleExports
                  .Select(e => new ExportResult(e))
                  .ToList();
      }
    }

    public IEnumerable<ExportResult> Exports { get; private set; }
  }
}
