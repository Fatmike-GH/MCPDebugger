#pragma once
#include "Native/Debugger/DebugEventHandler/Interfaces/IDebugEventHandler.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::DebugEventHandler::Interfaces
{
  /// <summary>
  /// gMock double of the handler of one kind of debug event.
  /// </summary>
  class MockDebugEventHandler : public MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler
  {
  public:
    MOCK_METHOD(MCP::Native::Debugger::DebugEventHandler::HandleResult, HandleEvent, (DEBUG_EVENT& debugEvent), (override));
  };
}
