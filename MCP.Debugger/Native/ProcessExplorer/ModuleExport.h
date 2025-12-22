#pragma once
#include <Windows.h>

namespace MCP::Native::ProcessExplorer
{
  struct ModuleExport
  {
    std::string Name;
    std::string ForwardedName;
    void* VirtualAddress = nullptr; // Absolute VA
    DWORD Ordinal = 0;
  };
}