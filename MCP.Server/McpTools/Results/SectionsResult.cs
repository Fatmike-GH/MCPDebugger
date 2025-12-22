using MCP.Debugger.Types.Modules;

namespace MCP.Server.McpTools.Results
{
  public class SectionsResult
  {
    public SectionsResult(IEnumerable<ModuleSection> sections)
    {
      if (sections == null)
      {
        Sections = new List<SectionResult>();
      }
      else
      {
        Sections = sections
                  .Select(e => new SectionResult(e))
                  .ToList();
      }
    }

    public IEnumerable<SectionResult> Sections { get; private set; }
  }
}
