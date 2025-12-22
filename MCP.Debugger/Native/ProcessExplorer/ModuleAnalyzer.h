#pragma once
#include <Windows.h>
#include <string>
#include <memory>
#include "PEImage.h"
#include "ModuleInfo.h"

namespace MCP::Native::ProcessExplorer
{
  class ModuleAnalyzer
  {
  public:
    static ModuleInfoPtr AnalyzeModule(HANDLE hProcess, HANDLE hFile, void* baseAddress);

  private:
    static ModuleInfoPtr CreateModuleInfo(PEImage& peImage, HANDLE hProcess, HANDLE hFile, void* baseAddress);
    static std::wstring GetPathFromHandle(HANDLE hFile);
    static std::wstring GetPathFromProcess(HANDLE hProcess);
    static std::wstring GetPathFromBaseAddress(HANDLE hProcess, void* baseAddress);
  };
}