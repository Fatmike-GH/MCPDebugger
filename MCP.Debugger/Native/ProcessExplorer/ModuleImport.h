#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "ModuleImportFunction.h"

namespace MCP::Native::ProcessExplorer
{
  struct ModuleImport
  {
    std::string ModuleName;
    std::vector<ModuleImportFunction> Functions;
  };
}