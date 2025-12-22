#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <string>
#include "Instruction.h"
#include "SymbolResolver.h"
#include "..\Zydis\include\Zydis.h"

namespace MCP::Native::ProcessExplorer
{
  class ProcessExplorer;
}

namespace MCP::Native::Disassembler
{
  class Disassembler
  {
  public:
    Disassembler(MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer);
    ~Disassembler();

    void Disassemble(HANDLE processHandle,
                     DWORD processId,
                     DWORD_PTR startAddress,
                     SSIZE_T length,
                     std::shared_ptr<std::vector<Instruction>>& outInstructions);

    void Disassemble(HANDLE processHandle,
                     DWORD processId,
                     DWORD_PTR address,
                     Instruction& outInstruction);

    bool IsCallInstruction(HANDLE processHandle, DWORD_PTR address, size_t& instructionLength);

  private:
    void GetInstruction(BYTE* address, SSIZE_T length, DWORD_PTR runtimeAddress, ZydisDisassembledInstruction& instructionInfo);

    void AppendOperandSymbols(const ZydisDisassembledInstruction& info,
                              SymbolResolver& resolver,
                              HANDLE processHandle,
                              DWORD processId,
                              std::string& disasmText);

  private:
    MCP::Native::ProcessExplorer::ProcessExplorer* _processExplorer;
  };
}