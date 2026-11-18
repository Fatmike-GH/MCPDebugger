#pragma once
#include "Native/Disassembler/Interfaces/IDisassembler.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Disassembler::Interfaces
{
  /// <summary>
  /// gMock double of the decoder of a debuggee's machine code.
  /// </summary>
  class MockDisassembler : public MCP::Native::Disassembler::Interfaces::IDisassembler
  {
  public:
    MOCK_METHOD(void, Disassemble, (HANDLE processHandle, DWORD processId, DWORD_PTR startAddress, SSIZE_T length, std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>>& outInstructions), (override));
    MOCK_METHOD(bool, IsCallInstruction, (HANDLE processHandle, DWORD_PTR address, size_t& instructionLength), (override));
  };
}
