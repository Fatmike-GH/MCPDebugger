#pragma once
#include <Windows.h>
#include <map>
#include <memory>

namespace MCP::Native::Debugger
{
  class Breakpoint;
}

namespace MCP::Native::Debugger
{
  class BreakpointManager
  {
  public:
    BreakpointManager(HANDLE processHandle);
    ~BreakpointManager();

    void SetHardwareBreakpoint(PCONTEXT& context, DWORD_PTR address);
    void RemoveHardwareBreakpoint(PCONTEXT& context);
    bool HasHardwareBreakpoint(PCONTEXT& context);

    bool SetBreakpoint(DWORD_PTR address);
    bool RemoveBreakpoint(DWORD_PTR address);
    Breakpoint* GetBreakpoint(DWORD_PTR address);
    bool HasBreakpoint(DWORD_PTR address);

    bool SetMemoryBreakpoint(DWORD_PTR address, DWORD_PTR size);
    bool IsMemoryBreakpointActive() { return _memoryBreakPointActive; }
    void NotifyMemoryBreakpointRemoved();

    // Used for single stepping
    void SetTrapFlag(HANDLE threadHandle, bool enable);

  private:
    std::map<DWORD_PTR, std::unique_ptr<Breakpoint>> _breakpoints;
    bool _memoryBreakPointActive;
    HANDLE _processHandle;
  };
}
