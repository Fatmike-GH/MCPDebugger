#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <string>
#include "Instruction.h"
#include ".\Interfaces\IDisassembler.h"
#include "..\Zydis\include\Zydis.h"

namespace MCP::Native::Disassembler::Interfaces
{
  class ISymbolResolver;
}

namespace MCP::Native::Disassembler
{
  class Disassembler : public Interfaces::IDisassembler
  {
  public:
    /// <summary>
    /// The resolver is borrowed and must outlive the disassembler; it names the addresses the operands of
    /// an instruction refer to.
    /// </summary>
    explicit Disassembler(Interfaces::ISymbolResolver& symbolResolver);
    ~Disassembler() override;

    void Disassemble(HANDLE processHandle,
                     DWORD processId,
                     DWORD_PTR startAddress,
                     SSIZE_T length,
                     std::shared_ptr<std::vector<Instruction>>& outInstructions) override;

    void Disassemble(HANDLE processHandle,
                     DWORD processId,
                     DWORD_PTR address,
                     Instruction& outInstruction);

    bool IsCallInstruction(HANDLE processHandle, DWORD_PTR address, size_t& instructionLength) override;

  private:
    void GetInstruction(BYTE* address, SSIZE_T length, DWORD_PTR runtimeAddress, ZydisDisassembledInstruction& instructionInfo);

    void AppendOperandSymbols(const ZydisDisassembledInstruction& info,
                              Interfaces::ISymbolResolver& resolver,
                              HANDLE processHandle,
                              DWORD processId,
                              std::string& disasmText);

  private:
    Interfaces::ISymbolResolver& _symbolResolver;
  };
}