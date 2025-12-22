#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "..\ProcessExplorer\ProcessExplorer.h"

namespace MCP::Native::Disassembler
{
  class SymbolResolver
  {
  public:
    SymbolResolver(MCP::Native::ProcessExplorer::ProcessExplorer* explorer);

    std::string Resolve(HANDLE hProcess, DWORD processId, DWORD_PTR address);
    std::string ResolveString(HANDLE hProcess, DWORD_PTR address);

  private:
    std::string TryResolveExport(DWORD processId, DWORD_PTR address);
    std::string TryResolveIndirect(HANDLE hProcess, DWORD processId, DWORD_PTR address);
    std::string TryReadString(HANDLE hProcess, DWORD_PTR address);
    std::string WStringToString(const std::wstring& wstr);

  private:
    MCP::Native::ProcessExplorer::ProcessExplorer* _processExplorer;
  };
}