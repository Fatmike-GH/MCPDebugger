#include "BreakpointManager.h"
#include <utility>

namespace MCP::Native::Debugger
{
  BreakpointManager::BreakpointManager(HANDLE processHandle, std::unique_ptr<Interfaces::IBreakpointFactory> breakpointFactory)
    : _breakpointFactory(std::move(breakpointFactory))
  {
    _processHandle = processHandle;
    _memoryBreakPointActive = false;
  }

  BreakpointManager::~BreakpointManager()
  {
    _breakpoints.clear();
  }

  void BreakpointManager::SetHardwareBreakpoint(PCONTEXT& context, DWORD_PTR address)
  {
    context->Dr0 = address;
    context->Dr7 = 1 << 0; // Enable breakpoint
    context->EFlags &= ~(1 << 8); // Remove trap flag
  }

  void BreakpointManager::RemoveHardwareBreakpoint(PCONTEXT& context)
  {
    context->Dr0 = 0;
    context->Dr7 = 0; // Disable breakpoint
    context->EFlags &= ~(1 << 8); // Remove trap flag
  }

  bool BreakpointManager::HasHardwareBreakpoint(PCONTEXT& context)
  {
    return (context->Dr7 & 0x1) != 0;
  }

  bool BreakpointManager::SetBreakpoint(DWORD_PTR address)
  {
    auto iter = _breakpoints.find(address);
    if (iter == _breakpoints.end())
    {
      iter = _breakpoints.emplace(address, _breakpointFactory->Create(address)).first;
    }

    // The debuggee is stopped at the held address: its breakpoint instruction is written at the release.
    if (_heldAddress != address)
    {
      iter->second->Enable();
    }
    return true;
  }

  bool BreakpointManager::RemoveBreakpoint(DWORD_PTR address)
  {
    auto iter = _breakpoints.find(address);
    if (iter == _breakpoints.end()) return false;

    iter->second->Disable();
    _breakpoints.erase(iter);

    return true;
  }

  bool BreakpointManager::HasBreakpoint(DWORD_PTR address)
  {
    auto iter = _breakpoints.find(address);
    return (iter != _breakpoints.end());
  }

  void BreakpointManager::HoldBreakpointAt(DWORD_PTR address)
  {
    if (_heldAddress == address) return;

    // Only one address is held: a breakpoint at the address held so far must not stay out of memory.
    ReleaseHeldBreakpoint();

    _heldAddress = address;
    auto iter = _breakpoints.find(address);
    if (iter != _breakpoints.end())
    {
      iter->second->Disable();
    }
  }

  void BreakpointManager::ReleaseHeldBreakpoint()
  {
    if (!_heldAddress.has_value()) return;

    auto iter = _breakpoints.find(*_heldAddress);
    _heldAddress.reset();
    if (iter != _breakpoints.end())
    {
      iter->second->Enable();
    }
  }

  bool BreakpointManager::SetMemoryBreakpoint(DWORD_PTR address, DWORD_PTR size)
  {
    if (_memoryBreakPointActive) return false;
    _memoryBreakPointActive = true;
    DWORD oldProtect = 0;
    VirtualProtectEx(_processHandle, (LPVOID)address, size, PAGE_GUARD | PAGE_EXECUTE_READWRITE, &oldProtect);
    return true;
  }

  void BreakpointManager::NotifyMemoryBreakpointRemoved()
  {
    _memoryBreakPointActive = false;
  }

  void BreakpointManager::SetTrapFlag(HANDLE threadHandle, bool enable)
  {
    CONTEXT context = { 0 };
    context.ContextFlags = CONTEXT_FULL; // Need CONTEXT_FULL for EFlags
    GetThreadContext(threadHandle, &context);

    if (enable)
    {
      context.EFlags |= (1 << 8); // Set trap flag
    }
    else
    {
      context.EFlags &= ~(1 << 8); // Clear Trap Flag
    }

    SetThreadContext(threadHandle, &context);
  }
}
