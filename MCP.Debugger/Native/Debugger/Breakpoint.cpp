#include "Breakpoint.h"

namespace MCP::Native::Debugger
{
  Breakpoint::Breakpoint(HANDLE process, DWORD_PTR va)
  {
    _process = process;
    _va = va;
    _enabled = false;
    memset(_originalCode, 0, sizeof(_originalCode));
  }

  Breakpoint::~Breakpoint()
  {
  }

  void Breakpoint::Enable()
  {
    if (_enabled) return;

    // TO DO: Check if _originalCode is already a 0xCC
    ReadProcessMemory(_process, (LPVOID)_va, _originalCode, sizeof(_originalCode), NULL);
    WriteProcessMemory(_process, (LPVOID)_va, &_breakpointCode, sizeof(_breakpointCode), NULL);
    FlushInstructionCache(_process, (LPVOID)_va, sizeof(_breakpointCode));

    _enabled = true;
  }

  void Breakpoint::Disable()
  {
    if (!_enabled) return;

    WriteProcessMemory(_process, (LPVOID)_va, &_originalCode, sizeof(_originalCode), NULL);
    FlushInstructionCache(_process, (LPVOID)_va, sizeof(_originalCode));

    _enabled = false;
  }
}