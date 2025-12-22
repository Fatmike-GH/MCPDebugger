#pragma once
#include <Windows.h>

namespace MCP::Native::ProcessExplorer
{
  struct ModuleSection
  {
    char Name[IMAGE_SIZEOF_SHORT_NAME + 1] = { 0 };
    void* BaseAddress = nullptr;
    DWORD VirtualSize = 0;
    DWORD Characteristics = 0;
  };
}