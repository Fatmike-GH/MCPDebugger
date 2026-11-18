#pragma once
#include <Windows.h>
#include <string>

namespace MCP::Native::Disassembler::Interfaces
{
  /// <summary>
  /// Gives addresses of a debuggee process a readable meaning: the exported function they belong to, or the
  /// text they point at.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only - resolving may read the debuggee's memory.
  /// Ownership: the process handle is borrowed for the call; the text is returned by value.
  /// Failures: an address without a meaning yields an empty text.
  /// </remarks>
  class ISymbolResolver
  {
  public:
    virtual ~ISymbolResolver() = default;

    /// <summary>The symbol or text an operand address stands for; empty when it has none.</summary>
    virtual std::string Resolve(HANDLE hProcess, DWORD processId, DWORD_PTR address) = 0;

    /// <summary>The text stored at the address; empty when there is none.</summary>
    virtual std::string ResolveString(HANDLE hProcess, DWORD_PTR address) = 0;
  };
}
