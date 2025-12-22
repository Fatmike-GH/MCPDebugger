using MCP.Server.McpTools;
using Microsoft.Extensions.DependencyInjection;
using ModelContextProtocol.Server;

namespace MCP.Server
{
  internal class Program
  {
    static void Main(string[] args)
    {
      Task.Run(async () => await new Program().Start()).Wait();
    }

    public async Task Start(System.IO.Stream inputStream = null, System.IO.Stream outputStream = null)
    {
      var serviceCollection = new ServiceCollection();

      var serverBuilder = serviceCollection.AddMcpServer(options => options.ServerInstructions = "The server provides tools for static and dynamic analysis (debugging) of windows executables. Very important: Always use HexToDecConverter before reading memory from addresses you see in disassembly!");

      if (inputStream == null || outputStream == null)
        serverBuilder.WithStdioServerTransport();
      else
        serverBuilder.WithStreamServerTransport(inputStream, outputStream);

      //serverBuilder.WithTools<DebuggerTools>(); // Transient
      serverBuilder.WithTools(new DebuggerTools()); // Singleton

      var servicesProvider = serviceCollection.BuildServiceProvider();
      var server = servicesProvider.GetRequiredService<McpServer>();

      await server.RunAsync();
    }
  }
}
