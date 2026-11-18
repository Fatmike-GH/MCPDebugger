#pragma once
#include "Native/Debugger/Interfaces/IBreakpoint.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of one software breakpoint.
  /// </summary>
  class MockBreakpoint : public MCP::Native::Debugger::Interfaces::IBreakpoint
  {
  public:
    MOCK_METHOD(DWORD_PTR, GetVa, (), (const, override));
    MOCK_METHOD(void, Enable, (), (override));
    MOCK_METHOD(void, Disable, (), (override));
    MOCK_METHOD(bool, IsEnabled, (), (const, override));
  };
}
