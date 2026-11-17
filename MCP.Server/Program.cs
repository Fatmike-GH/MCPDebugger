using System.Reflection;
using MCP.Debugger;
using MCP.Server.McpTools;
using Microsoft.Extensions.DependencyInjection;
using ModelContextProtocol.Server;

namespace MCP.Server
{
  /// <summary>
  /// Entry point of the MCP server process and its composition root: the only place that decides which
  /// implementations the server's classes get and that takes objects out of the container.
  /// </summary>
  internal class Program
  {
    static void Main(string[] args)
    {
      Task.Run(async () => await new Program().Start()).Wait();
    }

    /// <summary>
    /// Wires the server's objects, then serves MCP requests until the client closes the connection.
    /// </summary>
    /// <remarks>
    /// When one of the two streams is null, standard input and output are used instead of them. Nothing may be
    /// written to the console here: with that transport the standard output is the protocol channel.
    /// </remarks>
    /// <param name="inputStream">Stream the requests are read from.</param>
    /// <param name="outputStream">Stream the answers are written to.</param>
    /// <returns>A task that completes when the server stopped.</returns>
    public async Task Start(System.IO.Stream inputStream = null, System.IO.Stream outputStream = null)
    {
      var serviceCollection = new ServiceCollection();

      // One dispatcher, one debugger service and one tool object per process: the debugging session lives in the
      // service and the session flags in the tool object, and both must survive from one tool call to the next.
      serviceCollection.AddSingleton<IDispatcher, ConsoleDispatcher>();
      serviceCollection.AddDebuggerService();
      serviceCollection.AddSingleton<DebuggerTools>();

      var serverBuilder = serviceCollection.AddMcpServer(options => options.ServerInstructions = "The server provides tools for static and dynamic analysis (debugging) of windows executables. Very important: Always use HexToDecConverter before reading memory from addresses you see in disassembly!");

      if (inputStream == null || outputStream == null)
        serverBuilder.WithStdioServerTransport();
      else
        serverBuilder.WithStreamServerTransport(inputStream, outputStream);

      AddToolsOfSingleton<DebuggerTools>(serviceCollection);

      var servicesProvider = serviceCollection.BuildServiceProvider();

      // Creates dispatcher, debugger service and tool object before the first request is accepted: when one of
      // them cannot be created, the process ends at start instead of failing the first tool call.
      _ = servicesProvider.GetRequiredService<DebuggerTools>();

      var server = servicesProvider.GetRequiredService<McpServer>();

      await server.RunAsync();

      // The container is deliberately not disposed. The process ends right after this method returned, which
      // releases everything and also ends a debuggee that still runs. A dispose would only add risk: the
      // synchronous one throws because the MCP server object can only be disposed asynchronously, and disposing
      // the debugger service could wait for a service call that is still blocked and so delay the exit.
    }

    /// <summary>
    /// Registers one MCP tool for every method of <typeparamref name="TTools"/> that carries
    /// <see cref="McpServerToolAttribute"/>. An instance method is called on the one
    /// <typeparamref name="TTools"/> object of the container, so what that object keeps between two tool calls
    /// is preserved.
    /// </summary>
    /// <remarks>
    /// The SDK's own <c>WithTools&lt;T&gt;()</c> is not used because it creates a new tool object for every call.
    /// The SDK disposes the object a tool call ran on when it is disposable; <typeparamref name="TTools"/> must
    /// therefore not implement <see cref="IDisposable"/> or <see cref="IAsyncDisposable"/>.
    /// </remarks>
    /// <typeparam name="TTools">The tool class; registered in <paramref name="services"/> as a singleton.</typeparam>
    /// <param name="services">The collection the tools are added to.</param>
    /// <exception cref="InvalidOperationException"><typeparamref name="TTools"/> is disposable.</exception>
    private static void AddToolsOfSingleton<TTools>(IServiceCollection services)
      where TTools : class
    {
      if (typeof(IDisposable).IsAssignableFrom(typeof(TTools)) || typeof(IAsyncDisposable).IsAssignableFrom(typeof(TTools)))
      {
        throw new InvalidOperationException(
          $"{typeof(TTools).Name} is disposable: it would be disposed after its first tool call although the container keeps handing out the same object.");
      }

      // The same methods the SDK picks up for a tool class: public and non-public, instance and static.
      const BindingFlags ToolMethods = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static;

      foreach (MethodInfo method in typeof(TTools).GetMethods(ToolMethods))
      {
        if (method.GetCustomAttribute<McpServerToolAttribute>() == null)
        {
          continue;
        }

        if (method.IsStatic)
        {
          services.AddSingleton<McpServerTool>(provider => McpServerTool.Create(
            method,
            target: null,
            new McpServerToolCreateOptions { Services = provider }));
        }
        else
        {
          services.AddSingleton<McpServerTool>(provider => McpServerTool.Create(
            method,
            request => request.Services.GetRequiredService<TTools>(),
            new McpServerToolCreateOptions { Services = provider }));
        }
      }
    }
  }
}
