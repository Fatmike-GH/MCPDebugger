#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include ".\Interfaces\ISymbolResolver.h"
#include "..\ProcessExplorer\Interfaces\IProcessExplorer.h"

namespace MCP::Native::Disassembler
{
  class SymbolResolver : public Interfaces::ISymbolResolver
  {
  public:
    /// <summary>The explorer is borrowed and must outlive the resolver.</summary>
    explicit SymbolResolver(MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer& explorer);

    std::string Resolve(HANDLE hProcess, DWORD processId, DWORD_PTR address) override;
    std::string ResolveString(HANDLE hProcess, DWORD_PTR address) override;

  private:
    std::string TryResolveExport(DWORD processId, DWORD_PTR address);
    std::string TryResolveIndirect(HANDLE hProcess, DWORD processId, DWORD_PTR address);
    std::string TryReadString(HANDLE hProcess, DWORD_PTR address);
    std::string WStringToString(const std::wstring& wstr);

  private:
    MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer& _processExplorer;
  };
}