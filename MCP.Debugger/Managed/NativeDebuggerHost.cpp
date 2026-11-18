// Included before any header of a managed type: the Win32 headers rename identifiers through their A/W
// macros, and every source file that sees those headers must see the member names of a managed type the
// same way as the others.
#include <Windows.h>

#include "NativeDebuggerHost.h"
#include "..\Native\Debugger\Interfaces\IDebugger.h"
#include "..\Native\Debugger\Interfaces\IDebuggerEventHandler.h"

namespace MCP::Debugger
{
  // Members are destroyed in reverse order of declaration: the debugger first, then the event handler it
  // reports to.
  struct NativeDebuggerObjects
  {
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerEventHandler> EventHandler;
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebugger> Debugger;
  };

  NativeDebuggerHost::NativeDebuggerHost(std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerEventHandler> eventHandler,
                                         std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebugger> debugger)
  {
    auto nativeObjects = std::make_unique<NativeDebuggerObjects>();
    nativeObjects->EventHandler = std::move(eventHandler);
    nativeObjects->Debugger = std::move(debugger);
    _nativeObjects = nativeObjects.release();
  }

  NativeDebuggerHost::~NativeDebuggerHost()
  {
    this->!NativeDebuggerHost();
  }

  NativeDebuggerHost::!NativeDebuggerHost()
  {
    if (_nativeObjects != nullptr)
    {
      delete _nativeObjects;
      _nativeObjects = nullptr;
    }
  }

  MCP::Native::Debugger::Interfaces::IDebugger* NativeDebuggerHost::Debugger::get()
  {
    if (_nativeObjects == nullptr)
    {
      throw gcnew System::ObjectDisposedException("NativeDebuggerHost");
    }
    return _nativeObjects->Debugger.get();
  }
}
