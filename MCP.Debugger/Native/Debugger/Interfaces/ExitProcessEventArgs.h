#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::Interfaces
{
  struct ExitProcessEventArgs
  {
    DWORD ProcessId;
  };
}