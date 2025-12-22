#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::DebugEventHandler
{
  struct DllInfo
  {
    DWORD ProcessId;
    LOAD_DLL_DEBUG_INFO Info;
  };
}