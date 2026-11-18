#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "..\ModuleInfo.h"

namespace MCP::Native::ProcessExplorer::Interfaces
{
  /// <summary>
  /// Answers questions about the modules of the debuggee processes: which are loaded, what they export and
  /// where an address belongs.
  /// </summary>
  /// <remarks>
  /// Threading: every query may be called from any thread; it reads bookkeeping that the debug thread fills
  /// and never interrupts the debuggee.
  /// Failures: a query returns false or null when the process, the module or the address is not known.
  /// Ownership: module information is handed out as shared pointers and stays valid after the module was
  /// unloaded; name parameters are borrowed for the duration of the call.
  /// </remarks>
  class IProcessExplorer
  {
  public:
    virtual ~IProcessExplorer() = default;

    virtual bool GetModules(DWORD processId, std::vector<ModuleInfoPtr>& modules) = 0;
    virtual bool GetModule(DWORD processId, HMODULE moduleHandle, ModuleInfoPtr& outModuleInfo) = 0;
    virtual bool GetModuleByVA(DWORD processId, DWORD_PTR va, ModuleInfoPtr& outModuleInfo) = 0;
    virtual bool GetFunctionByVA(DWORD processId, DWORD_PTR va, ModuleExport& outExport) = 0;

    /// <summary>Finds a loaded module by its file name; null when it is not loaded.</summary>
    virtual HMODULE GetRemoteModuleHandle(DWORD processId, const wchar_t* moduleName) = 0;

    /// <summary>
    /// Finds an export of a loaded module by its name, or by its ordinal when lpProcName carries an ordinal
    /// instead of a string pointer; null when the module or the export is not known.
    /// </summary>
    virtual FARPROC GetRemoteProcAddress(DWORD processId, HMODULE hModule, LPCSTR lpProcName) = 0;
  };
}
