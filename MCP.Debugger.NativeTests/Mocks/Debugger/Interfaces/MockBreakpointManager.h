#pragma once
#include "Native/Debugger/Interfaces/IBreakpoint.h"
#include "Native/Debugger/Interfaces/IBreakpointManager.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the breakpoints of a debuggee process.
  /// </summary>
  class MockBreakpointManager : public MCP::Native::Debugger::Interfaces::IBreakpointManager
  {
  public:
    MOCK_METHOD(bool, SetBreakpoint, (DWORD_PTR address), (override));
    MOCK_METHOD(bool, RemoveBreakpoint, (DWORD_PTR address), (override));
    MOCK_METHOD(MCP::Native::Debugger::Interfaces::IBreakpoint*, GetBreakpoint, (DWORD_PTR address), (override));
    MOCK_METHOD(void, SetTrapFlag, (HANDLE threadHandle, bool enable), (override));
  };
}
