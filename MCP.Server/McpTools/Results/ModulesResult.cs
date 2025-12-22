using MCP.Debugger.Types.Modules;

namespace MCP.Server.McpTools.Results
{
  public class ModulesResult
  {
    public ModulesResult(IEnumerable<Module> modules)
    {
      if (modules == null)
      {
        Modules = new List<ModuleResult>();
      }
      else
      {
        Modules = modules
                  .Select(e => new ModuleResult(e))
                  .ToList();
      }
    }

    public IEnumerable<ModuleResult> Modules { get; private set; }
  }
}
