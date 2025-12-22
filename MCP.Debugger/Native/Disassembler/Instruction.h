#pragma once
#include <Windows.h>
#include <string>

namespace MCP::Native::Disassembler
{
  struct Instruction
  {
    DWORD_PTR VA;
    DWORD_PTR InstructionLength;
    std::string Disassembly;
  };
}