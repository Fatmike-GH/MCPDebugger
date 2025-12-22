#pragma once
#include <Windows.h>
#include <string>
#include <vector>

namespace MCP::Native::ProcessExplorer
{
  struct ModuleImportFunction
  {
    std::string Name;
    WORD Ordinal = 0;
    bool ImportByName = false;
    void* IATEntryAddress = nullptr;
  };
}