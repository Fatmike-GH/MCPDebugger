#pragma once
#include <Windows.h>
#include ".\Interfaces\IDebuggeeBreakIn.h"

namespace MCP::Native::Debugger
{
  /// <summary>
  /// Interrupts a running debuggee through the operating system. It holds no logic of its own: it exists so
  /// that the code deciding when to interrupt can be tested without a debuggee.
  /// </summary>
  class DebuggeeBreakIn : public Interfaces::IDebuggeeBreakIn
  {
  public:
    void RequestBreakIn(HANDLE processHandle) override;
  };
}
