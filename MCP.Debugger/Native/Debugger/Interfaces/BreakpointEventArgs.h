#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::Interfaces
{
  struct BreakpointEventArgs
  {
    DWORD ProcessId;
    DWORD ThreadId;
    CONTEXT Context;
  };
}