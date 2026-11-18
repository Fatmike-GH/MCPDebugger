#pragma once
#include <Windows.h>
#include "..\ModuleInfo.h"

namespace MCP::Native::ProcessExplorer::Interfaces
{
  /// <summary>
  /// Describes a module that is loaded into a debuggee process: where it lies, which file it comes from and
  /// what it exports and imports.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only - the analysis reads the debuggee's memory.
  /// Ownership: the handles are borrowed for the duration of the call; the result is handed out as a shared
  /// pointer.
  /// </remarks>
  class IModuleAnalyzer
  {
  public:
    virtual ~IModuleAnalyzer() = default;

    /// <summary>
    /// Reads the image at the base address. Null when the process handle or the base address is null or the
    /// image cannot be read. The file handle may be null; it only helps to find the module's path.
    /// </summary>
    virtual ModuleInfoPtr AnalyzeModule(HANDLE hProcess, HANDLE hFile, void* baseAddress) = 0;
  };
}
