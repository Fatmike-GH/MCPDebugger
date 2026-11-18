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

namespace MCP::Native::Platform::Interfaces
{
  class IProcessMemory;
}

namespace MCP::Native::Disassembler
{
  class Disassembler : public Interfaces::IDisassembler
  {
  public:
    /// <summary>
    /// Both collaborators are borrowed and must outlive the disassembler. The resolver names the addresses
    /// the operands of an instruction refer to; the machine code to decode is read through the process
    /// memory.
    /// </summary>
    Disassembler(Interfaces::ISymbolResolver& symbolResolver,
                 MCP::Native::Platform::Interfaces::IProcessMemory& processMemory);
    ~Disassembler() override;

    void Disassemble(HANDLE processHandle,
                     DWORD processId,
                     DWORD_PTR startAddress,
                     SSIZE_T length,
                     std::shared_ptr<std::vector<Instruction>>& outInstructions) override;

    bool IsCallInstruction(HANDLE processHandle, DWORD_PTR address, size_t& instructionLength) override;

  private:
    /// <summary>
    /// What decoding the bytes at one position gave.
    /// </summary>
    enum class DecodeOutcome
    {
      /// <summary>The bytes begin with a complete instruction.</summary>
      Decoded,
      /// <summary>The bytes end inside an instruction: they may be the beginning of a valid one.</summary>
      CutOff,
      /// <summary>The bytes are no instruction, however many bytes would follow.</summary>
      Undecodable
    };

    /// <summary>
    /// Decodes one instruction from the bytes, which are located at the runtime address in the debuggee. The
    /// instruction is filled in - and may be read - only when the outcome is Decoded.
    /// </summary>
    static DecodeOutcome Decode(const BYTE* bytes, size_t size, DWORD_PTR runtimeAddress, ZydisDisassembledInstruction& instruction);

    /// <summary>
    /// Appends the symbol of every address a visible operand of the instruction refers to. The runtime
    /// address is where the instruction is located in the debuggee: relative operands are resolved from it.
    /// </summary>
    void AppendOperandSymbols(const ZydisDisassembledInstruction& info,
                              DWORD_PTR runtimeAddress,
                              Interfaces::ISymbolResolver& resolver,
                              HANDLE processHandle,
                              DWORD processId,
                              std::string& disasmText);

  private:
    Interfaces::ISymbolResolver& _symbolResolver;
    MCP::Native::Platform::Interfaces::IProcessMemory& _processMemory;
  };
}
