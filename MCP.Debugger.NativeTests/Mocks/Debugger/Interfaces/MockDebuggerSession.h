#pragma once
#include "Native/Debugger/DebugEventHandler/ProcessInfo.h"
#include "Native/Debugger/Interfaces/IDebuggerSession.h"
#include "Native/Debugger/Interfaces/IDebuggerThread.h"
#include "Native/ProcessExplorer/Interfaces/IProcessExplorer.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the objects of one debugging session.
  /// </summary>
  class MockDebuggerSession : public MCP::Native::Debugger::Interfaces::IDebuggerSession
  {
  public:
    MOCK_METHOD(MCP::Native::Debugger::Interfaces::IDebuggerThread&, GetDebuggerThread, (), (override));
    MOCK_METHOD(MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer&, GetProcessExplorer, (), (override));
    MOCK_METHOD(bool, GetProcessById, (DWORD processId, MCP::Native::Debugger::DebugEventHandler::ProcessInfo& outInfo), (override));
  };
}
