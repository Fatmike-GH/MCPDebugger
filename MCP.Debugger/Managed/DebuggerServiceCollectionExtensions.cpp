// Included first: DebuggerService.h uses Win32 types (HANDLE, CONTEXT) without including their header, and
// the Win32 headers rename identifiers through their A/W macros, so all source files must see the member
// names of a managed type the same way.
#include <Windows.h>

#include "DebuggerServiceCollectionExtensions.h"

#include "DebuggerEventRelay.h"
#include "DebuggerService.h"
#include "NativeDebuggerHost.h"
#include ".\Interfaces\IDebuggerService.h"
#include ".\Interfaces\IDispatcher.h"
#include "..\Adapters\DebuggerEventHandlerAdapter.h"
#include "..\Native\Composition\NativeComposition.h"

using namespace Microsoft::Extensions::DependencyInjection;

namespace MCP::Debugger
{
  IServiceCollection^ DebuggerServiceCollectionExtensions::AddDebuggerService(IServiceCollection^ services)
  {
    if (services == nullptr)
    {
      throw gcnew System::ArgumentNullException("services");
    }

    // Registered with factory methods: the constructors take types the container must not create by itself
    // (native objects, non-public classes). Both type arguments are given, because the overloads that take a
    // factory cannot be told apart otherwise.
    ServiceCollectionServiceExtensions::AddSingleton<DebuggerEventRelay^, DebuggerEventRelay^>(
      services, gcnew System::Func<System::IServiceProvider^, DebuggerEventRelay^>(&DebuggerServiceCollectionExtensions::CreateDebuggerEventRelay));
    ServiceCollectionServiceExtensions::AddSingleton<INativeDebuggerHost^, NativeDebuggerHost^>(
      services, gcnew System::Func<System::IServiceProvider^, NativeDebuggerHost^>(&DebuggerServiceCollectionExtensions::CreateNativeDebuggerHost));
    return ServiceCollectionServiceExtensions::AddSingleton<IDebuggerService^, DebuggerService^>(
      services, gcnew System::Func<System::IServiceProvider^, DebuggerService^>(&DebuggerServiceCollectionExtensions::CreateDebuggerService));
  }

  DebuggerEventRelay^ DebuggerServiceCollectionExtensions::CreateDebuggerEventRelay(System::IServiceProvider^ provider)
  {
    return gcnew DebuggerEventRelay();
  }

  NativeDebuggerHost^ DebuggerServiceCollectionExtensions::CreateNativeDebuggerHost(System::IServiceProvider^ provider)
  {
    // The adapter is created here and not by the native composition function: it uses managed types, which
    // the native core must stay free of. A managed handle cannot be passed to std::make_unique as a
    // temporary, so the sink gets a local variable.
    IDebuggerEventSink^ sink = ServiceProviderServiceExtensions::GetRequiredService<DebuggerEventRelay^>(provider);
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerEventHandler> eventHandler =
      std::make_unique<MCP::Adapters::DebuggerEventHandlerAdapter>(sink);
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebugger> debugger =
      MCP::Native::Composition::CreateDebugger(*eventHandler);

    return gcnew NativeDebuggerHost(std::move(eventHandler), std::move(debugger));
  }

  DebuggerService^ DebuggerServiceCollectionExtensions::CreateDebuggerService(System::IServiceProvider^ provider)
  {
    // The container disposes its singletons in reverse order of creation. The service is created after the
    // collaborators it asks for here, so it is disposed before the owner of the native debugger.
    IDispatcher^ dispatcher = ServiceProviderServiceExtensions::GetRequiredService<IDispatcher^>(provider);
    INativeDebuggerHost^ host = ServiceProviderServiceExtensions::GetRequiredService<INativeDebuggerHost^>(provider);
    IDebuggerEventSource^ events = ServiceProviderServiceExtensions::GetRequiredService<DebuggerEventRelay^>(provider);

    return gcnew DebuggerService(dispatcher, host, events);
  }
}
