#pragma once
#include "Native/Debugger/Interfaces/BreakpointEventArgs.h"
#include "Native/Debugger/Interfaces/ExitProcessEventArgs.h"
#include "Native/Debugger/Interfaces/IDebuggerEventHandler.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the client the debugger reports its events to.
  /// </summary>
  class MockDebuggerEventHandler : public MCP::Native::Debugger::Interfaces::IDebuggerEventHandler
  {
  public:
    MOCK_METHOD(void, OnExitProcess, (std::shared_ptr<MCP::Native::Debugger::Interfaces::ExitProcessEventArgs> args), (override));
    MOCK_METHOD(void, OnSystemBreakpoint, (std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args), (override));
    MOCK_METHOD(void, OnEntryPointBreakpoint, (std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args), (override));
    MOCK_METHOD(void, OnBreakpoint, (std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args), (override));
    MOCK_METHOD(void, OnSingleStep, (std::shared_ptr<MCP::Native::Debugger::Interfaces::BreakpointEventArgs> args), (override));
  };
}
