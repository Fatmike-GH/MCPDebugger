#pragma once
#include "Native/Debugger/Interfaces/IDebuggerEventHandler.h"
#include "Native/Debugger/Interfaces/IDebuggerSessionFactory.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the factory the debugger asks for a new session on every start.
  /// </summary>
  class MockDebuggerSessionFactory : public MCP::Native::Debugger::Interfaces::IDebuggerSessionFactory
  {
  public:
    MOCK_METHOD(std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerSession>, CreateSession,
                (MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler), (override));
  };
}
