#pragma once
#include "Native/Debugger/Interfaces/IResumePlan.h"
#include "Native/Debugger/Interfaces/IStopSite.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the plan a resuming command leaves for the debug thread.
  /// </summary>
  class MockResumePlan : public MCP::Native::Debugger::Interfaces::IResumePlan
  {
  public:
    MOCK_METHOD(void, Begin, (MCP::Native::Debugger::Interfaces::IStopSite& stopSite), (override));
    MOCK_METHOD(bool, HaltsAfterFirstInstruction, (), (const, override));
    MOCK_METHOD(bool, OwnsBreakpointAt, (DWORD_PTR address), (const, override));
    MOCK_METHOD(void, End, (MCP::Native::Debugger::Interfaces::IStopSite& stopSite), (override));
  };
}
