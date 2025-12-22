using ModelContextProtocol;

namespace MCP.Server.Base
{
  public abstract class McpToolBase
  {
    public McpToolBase()
    {
    }

    protected void Execute(Action action)
    {
      try
      {
        action();
      }
      catch (Exception e)
      {
        throw HandleException(e);
      }
    }

    protected T Execute<T>(Func<T> action)
    {
      try
      {
        return action();
      }
      catch (Exception e)
      {
        throw HandleException(e);
      }
    }

    protected Exception HandleException(Exception e)
    {
      if (e is McpException)
        return e;
      else
        return new McpException($"Unexpected internal MCP server error! Internal exception: {e.Message}");
    }
  }
}
