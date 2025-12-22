#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger::DebugEventHandler
{
  struct HandleResult
  {
    DWORD ContinueStatus{ DBG_CONTINUE };
    bool ProcessExited{ false };
  };
}