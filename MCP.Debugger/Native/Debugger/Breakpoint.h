#pragma once
#include <Windows.h>

namespace MCP::Native::Debugger
{
  class Breakpoint
  {
  public:
    Breakpoint(HANDLE process, DWORD_PTR va);
    ~Breakpoint();

    DWORD_PTR GetVa() { return _va; }

    void Enable();
    void Disable();
    bool IsEnabled() { return _enabled; }

  private:
    HANDLE _process;
    DWORD_PTR _va;
    bool _enabled;
    const BYTE _breakpointCode[1] = { 0xCC }; // INT 3
    BYTE _originalCode[1];
  };
}

