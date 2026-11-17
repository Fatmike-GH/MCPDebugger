#pragma once

namespace MCP::Debugger
{
  /// <summary>
  /// Adds the services of the debugger assembly to a dependency-injection container. The composition root of an
  /// executable calls this instead of creating the service itself.
  /// </summary>
  [System::Runtime::CompilerServices::ExtensionAttribute]
  public ref class DebuggerServiceCollectionExtensions abstract sealed
  {
  public:
    /// <summary>
    /// Registers <see cref="IDebuggerService"/> as a singleton: one service, and with it one debugging session at
    /// a time, per container. The container creates the service on first use and disposes it together with the
    /// container, which ends a running session.
    /// </summary>
    /// <remarks>
    /// No dispatcher is registered: the service needs an <see cref="IDispatcher"/>, and which one fits depends on
    /// the executable (console or UI). The caller registers its <see cref="IDispatcher"/> implementation in the
    /// same collection.
    /// </remarks>
    /// <param name="services">The collection to add the registration to.</param>
    /// <returns><paramref name="services"/>, so that registrations can be chained.</returns>
    /// <exception cref="System::ArgumentNullException"><paramref name="services"/> is nullptr.</exception>
    [System::Runtime::CompilerServices::ExtensionAttribute]
    static Microsoft::Extensions::DependencyInjection::IServiceCollection^ AddDebuggerService(
      Microsoft::Extensions::DependencyInjection::IServiceCollection^ services);
  };
}
