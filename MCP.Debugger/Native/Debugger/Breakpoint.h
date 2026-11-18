#pragma once
#include <Windows.h>
#include ".\Interfaces\IBreakpoint.h"

namespace MCP::Native::Debugger
{
  class Breakpoint : public Interfaces::IBreakpoint
  {
  public:
    Breakpoint(HANDLE process, DWORD_PTR va);
    ~Breakpoint() override;

    DWORD_PTR GetVa() const override { return _va; }

    void Enable() override;
    void Disable() override;
    bool IsEnabled() const override { return _enabled; }

  private:
    HANDLE _process;
    DWORD_PTR _va;
    bool _enabled;
    const BYTE _breakpointCode[1] = { 0xCC }; // INT 3
    BYTE _originalCode[1];
  };
}

