#pragma once
#include "Native/Debugger/Interfaces/IStopSiteFactory.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the factory the handler of the stops creates its stop sites with.
  /// </summary>
  class MockStopSiteFactory : public MCP::Native::Debugger::Interfaces::IStopSiteFactory
  {
  public:
    MOCK_METHOD(std::unique_ptr<MCP::Native::Debugger::Interfaces::IStopSite>, Create, (HANDLE processHandle, DWORD_PTR stopAddress), (override));
  };
}
