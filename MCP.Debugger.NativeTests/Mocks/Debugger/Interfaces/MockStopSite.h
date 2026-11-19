#pragma once
#include "Native/Debugger/Interfaces/IStopSite.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of a stop of the debuggee as a resume plan sees it.
  /// </summary>
  class MockStopSite : public MCP::Native::Debugger::Interfaces::IStopSite
  {
  public:
    MOCK_METHOD(DWORD_PTR, GetStopAddress, (), (const, override));
    MOCK_METHOD(std::optional<size_t>, TryGetCallLength, (), (override));
    MOCK_METHOD(void, SetBreakpoint, (DWORD_PTR address), (override));
    MOCK_METHOD(void, RemoveBreakpoint, (DWORD_PTR address), (override));
  };
}
