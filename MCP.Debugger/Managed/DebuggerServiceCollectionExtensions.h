#pragma once

namespace MCP::Debugger
{
  ref class DebuggerEventRelay;
  ref class DebuggerService;
  ref class NativeDebuggerHost;

  /// <summary>
  /// Adds the services of the debugger assembly to a dependency-injection container. The composition root of an
  /// executable calls this instead of creating the service itself.
  /// </summary>
  /// <remarks>
  /// This class is the composition code of the assembly's managed part: the only place that creates the
  /// service and its collaborators and that calls the composition function of the native debugger core.
  /// </remarks>
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
    /// <para>
    /// No dispatcher is registered: the service needs an <see cref="IDispatcher"/>, and which one fits depends on
    /// the executable (console or UI). The caller registers its <see cref="IDispatcher"/> implementation in the
    /// same collection.
    /// </para>
    /// <para>
    /// Two more singletons are registered that only the service uses and that a consumer cannot see: the relay
    /// for the debugger events and the owner of the native debugger. When the container is disposed, it
    /// disposes the service first - no event handler runs after that - and then the owner of the native
    /// debugger, which ends the session.
    /// </para>
    /// </remarks>
    /// <param name="services">The collection to add the registration to.</param>
    /// <returns><paramref name="services"/>, so that registrations can be chained.</returns>
    /// <exception cref="System::ArgumentNullException"><paramref name="services"/> is nullptr.</exception>
    [System::Runtime::CompilerServices::ExtensionAttribute]
    static Microsoft::Extensions::DependencyInjection::IServiceCollection^ AddDebuggerService(
      Microsoft::Extensions::DependencyInjection::IServiceCollection^ services);

  private:
    // Factory methods the container calls on first use. System::IServiceProvider is written in full: with the
    // Win32 headers included, the unqualified name is the COM interface of the same name.

    /// <summary>Creates the relay between the native event handler adapter and the service.</summary>
    static DebuggerEventRelay^ CreateDebuggerEventRelay(System::IServiceProvider^ provider);

    /// <summary>Creates the native debugger with its event handler adapter and the object that owns both.</summary>
    static NativeDebuggerHost^ CreateNativeDebuggerHost(System::IServiceProvider^ provider);

    /// <summary>Creates the service with the dispatcher the executable registered.</summary>
    static DebuggerService^ CreateDebuggerService(System::IServiceProvider^ provider);
  };
}
