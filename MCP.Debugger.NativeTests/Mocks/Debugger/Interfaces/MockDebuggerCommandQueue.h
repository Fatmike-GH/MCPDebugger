#pragma once
#include "Native/Debugger/Interfaces/IDebuggerCommandQueue.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the hand-over of commands from client threads to the debug thread.
  /// </summary>
  class MockDebuggerCommandQueue : public MCP::Native::Debugger::Interfaces::IDebuggerCommandQueue
  {
  public:
    MOCK_METHOD(void, SetCurrentContext, (DWORD processId, DWORD threadId), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::CommandResult, Stop, (), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::CommandResult, Continue, (), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::CommandResult, StepInto, (), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::CommandResult, StepOver, (), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::CommandResult, SetBreakpoint, (DWORD_PTR va), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::CommandResult, RemoveBreakpoint, (DWORD_PTR va), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::ReadMemoryResult, ReadMemory, (DWORD_PTR va, size_t size), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::CommandResult, WriteMemory, (DWORD_PTR va, const BYTE* data, size_t size), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::DisassembleResult, Disassemble, (DWORD_PTR va, size_t length), (override));
    MOCK_METHOD(MCP::Native::Debugger::Commands::TryResolveStringResult, TryResolveString, (DWORD_PTR va), (override));
    MOCK_METHOD(MCP::Native::Debugger::Interfaces::StopOutcome, ProcessCommandsSync, (), (override));
    MOCK_METHOD(void, ProcessCommandsAsync, (), (override));
  };
}
