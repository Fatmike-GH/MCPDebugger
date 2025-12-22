#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::DebugEventHandler
{
  struct ProcessInfo
  {
    DWORD ProcessId;
    CREATE_PROCESS_DEBUG_INFO info;
  };
}