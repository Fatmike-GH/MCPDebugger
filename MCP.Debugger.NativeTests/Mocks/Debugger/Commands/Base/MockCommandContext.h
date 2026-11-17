#pragma once
#include "Native/Debugger/Commands/Base/ICommandContext.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Debugger::Commands::Base
{
  /// <summary>
  /// gMock double of the context a command executes against.
  /// </summary>
  class MockCommandContext : public MCP::Native::Debugger::Commands::Base::ICommandContext
  {
  public:
    MOCK_METHOD(MCP::Native::Debugger::BreakpointManager*, GetBreakpointManager, (), (override));
    MOCK_METHOD(void, ExecuteClearCommandQueue, (), (override));
    MOCK_METHOD(bool, ExecuteReadMemory, (DWORD_PTR address, size_t size, std::shared_ptr<std::vector<BYTE>>& outDataBuffer), (override));
    MOCK_METHOD(bool, ExecuteWriteMemory, (DWORD_PTR address, const BYTE* data, size_t size), (override));
    MOCK_METHOD(bool, ExecuteDisassemble, (DWORD_PTR address, size_t length, std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>>& outInstructions), (override));
    MOCK_METHOD(std::string, ExecuteTryResolveString, (DWORD_PTR address), (override));
  };
}
