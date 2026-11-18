#pragma once
#include "Native/Debugger/Interfaces/IBreakpointFactory.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the factory a breakpoint manager creates its breakpoints with.
  /// </summary>
  class MockBreakpointFactory : public MCP::Native::Debugger::Interfaces::IBreakpointFactory
  {
  public:
    MOCK_METHOD(std::unique_ptr<MCP::Native::Debugger::Interfaces::IBreakpoint>, Create, (DWORD_PTR va), (override));
  };
}
