#pragma once
#include <Windows.h>
#include <map>
#include <memory>
#include <optional>
#include ".\Interfaces\IBreakpoint.h"
#include ".\Interfaces\IBreakpointFactory.h"
#include ".\Interfaces\IBreakpointManager.h"

namespace MCP::Native::Debugger
{
  class BreakpointManager : public Interfaces::IBreakpointManager
  {
  public:
    /// <summary>
    /// The process handle is borrowed and must stay valid as long as the manager. The manager takes ownership
    /// of the factory and creates every breakpoint through it.
    /// </summary>
    BreakpointManager(HANDLE processHandle, std::unique_ptr<Interfaces::IBreakpointFactory> breakpointFactory);
    ~BreakpointManager() override;

    void SetHardwareBreakpoint(PCONTEXT& context, DWORD_PTR address);
    void RemoveHardwareBreakpoint(PCONTEXT& context);
    bool HasHardwareBreakpoint(PCONTEXT& context);

    bool SetBreakpoint(DWORD_PTR address) override;
    bool RemoveBreakpoint(DWORD_PTR address) override;
    bool HasBreakpoint(DWORD_PTR address) override;
    void HoldBreakpointAt(DWORD_PTR address) override;
    void ReleaseHeldBreakpoint() override;

    bool SetMemoryBreakpoint(DWORD_PTR address, DWORD_PTR size);
    bool IsMemoryBreakpointActive() { return _memoryBreakPointActive; }
    void NotifyMemoryBreakpointRemoved();

    // Used for single stepping
    void SetTrapFlag(HANDLE threadHandle, bool enable) override;

  private:
    std::unique_ptr<Interfaces::IBreakpointFactory> _breakpointFactory;
    // One entry per address a breakpoint is set at. Every entry is enabled, except the one at the held address.
    std::map<DWORD_PTR, std::unique_ptr<Interfaces::IBreakpoint>> _breakpoints;
    // The address the debuggee is stopped at, from the stop until it has left the address; empty otherwise.
    std::optional<DWORD_PTR> _heldAddress;
    bool _memoryBreakPointActive;
    HANDLE _processHandle;
  };
}
