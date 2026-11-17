#include "DebuggerServiceCollectionExtensions.h"

// DebuggerService.h uses Win32 types (HANDLE, CONTEXT) without including their header.
#include <Windows.h>

#include "DebuggerService.h"
#include ".\Interfaces\IDebuggerService.h"
#include ".\Interfaces\IDispatcher.h"

using namespace Microsoft::Extensions::DependencyInjection;

namespace MCP::Debugger
{
  IServiceCollection^ DebuggerServiceCollectionExtensions::AddDebuggerService(IServiceCollection^ services)
  {
    if (services == nullptr)
    {
      throw gcnew System::ArgumentNullException("services");
    }

    // Registered by type: the container calls the constructor of DebuggerService and supplies the dispatcher.
    return ServiceCollectionServiceExtensions::AddSingleton<IDebuggerService^, DebuggerService^>(services);
  }
}
