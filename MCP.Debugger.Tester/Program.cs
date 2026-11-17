using Microsoft.Extensions.DependencyInjection;

namespace MCP.Debugger.Tester
{
  /// <summary>
  /// Entry point of the tester and its composition root: the only place that decides which implementations the
  /// form gets and that takes objects out of the container.
  /// </summary>
  internal static class Program
  {
    /// <summary>Wires the tester's objects and runs the main form on this thread, the UI thread.</summary>
    [STAThread]
    static void Main()
    {
      ApplicationConfiguration.Initialize();

      // The container creates the dispatcher before the form, i.e. before the first control exists, and the
      // dispatcher keeps the synchronization context it finds in its constructor. Windows Forms installs its
      // context only with the first control, so it is installed here: without it the dispatcher would raise the
      // debugger events on the debugger's thread instead of the UI thread.
      SynchronizationContext.SetSynchronizationContext(new WindowsFormsSynchronizationContext());

      var serviceCollection = new ServiceCollection();
      serviceCollection.AddSingleton<IDispatcher, UiDispatcher>();
      serviceCollection.AddDebuggerService();
      serviceCollection.AddSingleton<Main>();

      var serviceProvider = serviceCollection.BuildServiceProvider();

      Application.Run(serviceProvider.GetRequiredService<Main>());

      // The container is deliberately not disposed. The process ends right after this method returned, which
      // releases everything and also ends a debuggee that still runs; disposing the debugger service could wait
      // for a service call that is still blocked and so delay the exit.
    }
  }
}
