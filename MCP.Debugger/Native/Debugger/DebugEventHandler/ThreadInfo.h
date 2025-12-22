#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::DebugEventHandler
{
  struct ThreadInfo
  {
    DWORD ProcessId;
    DWORD ThreadId;
    CREATE_THREAD_DEBUG_INFO info;
  };
}