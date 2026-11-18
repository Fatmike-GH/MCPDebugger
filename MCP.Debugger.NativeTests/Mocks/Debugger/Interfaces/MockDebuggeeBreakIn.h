#pragma once
#include "Native/Debugger/Interfaces/IDebuggeeBreakIn.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the break-in into a running debuggee.
  /// </summary>
  class MockDebuggeeBreakIn : public MCP::Native::Debugger::Interfaces::IDebuggeeBreakIn
  {
  public:
    MOCK_METHOD(void, RequestBreakIn, (HANDLE processHandle), (override));
  };
}
