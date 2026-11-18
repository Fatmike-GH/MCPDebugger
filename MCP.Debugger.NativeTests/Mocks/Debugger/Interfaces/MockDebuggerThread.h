#pragma once
#include "Native/Debugger/Interfaces/IDebuggerThread.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Interfaces
{
  /// <summary>
  /// gMock double of the debug thread of a session.
  /// </summary>
  class MockDebuggerThread : public MCP::Native::Debugger::Interfaces::IDebuggerThread
  {
  public:
    MOCK_METHOD(bool, Start, (const std::string& path), (override));
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
    MOCK_METHOD(PROCESS_INFORMATION, GetProcessInformation, (), (override));
    MOCK_METHOD(bool, IsRunning, (), (const, override));
  };
}
