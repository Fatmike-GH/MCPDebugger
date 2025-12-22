#include "BreakpointManager.h"
#include "Breakpoint.h"

namespace MCP::Native::Debugger
{
  BreakpointManager::BreakpointManager(HANDLE processHandle)
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
    Breakpoint* existing = GetBreakpoint(address);
    if (existing != nullptr)
    {
      existing->Enable();
    }
    else
    {
      auto breakpoint = std::make_unique<Breakpoint>(_processHandle, address);
      breakpoint->Enable();
      _breakpoints[address] = std::move(breakpoint);
    }
    return true;    
  }

  bool BreakpointManager::RemoveBreakpoint(DWORD_PTR address)
  {
    auto iter = _breakpoints.find(address);
    if (iter == _breakpoints.end()) return false;

    iter->second->Disable();

    return true;
  }

  Breakpoint* BreakpointManager::GetBreakpoint(DWORD_PTR address)
  {
    auto iter = _breakpoints.find(address);
    if (iter == _breakpoints.end()) return nullptr;
    return iter->second.get();    
  }

  bool BreakpointManager::HasBreakpoint(DWORD_PTR address)
  {
    auto iter = _breakpoints.find(address);
    return (iter != _breakpoints.end());
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
