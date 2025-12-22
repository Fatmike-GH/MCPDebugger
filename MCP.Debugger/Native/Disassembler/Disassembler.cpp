#define ZYDIS_STATIC_BUILD
#include "Disassembler.h"
#include <cstdio> // For sprintf_s

namespace MCP::Native::Disassembler
{
  Disassembler::Disassembler(MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer)
  {
    _processExplorer = processExplorer;
  }

  Disassembler::~Disassembler()
  {
  }

  void Disassembler::Disassemble(HANDLE processHandle,
                                 DWORD processId,
                                 DWORD_PTR startAddress,
                                 SSIZE_T length,
                                 std::shared_ptr<std::vector<Instruction>>& outInstructions)
  {
    if (length <= 0) return;

    std::vector<BYTE> buffer(length);
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(processHandle, (LPVOID)startAddress, buffer.data(), static_cast<size_t>(length), &bytesRead) || bytesRead == 0)
    {
      return;
    }

    outInstructions = std::make_shared<std::vector<Instruction>>();
    SSIZE_T remainingLength = static_cast<SSIZE_T>(bytesRead);
    BYTE* currentBufferPtr = buffer.data();
    DWORD_PTR currentRuntimeAddr = startAddress;

    SymbolResolver resolver(_processExplorer);

    while (remainingLength > 0)
    {
      ZydisDisassembledInstruction instructionInfo;
      GetInstruction(currentBufferPtr, remainingLength, currentRuntimeAddr, instructionInfo);

      std::string finalDisasm = instructionInfo.text;

      AppendOperandSymbols(instructionInfo, resolver, processHandle, processId, finalDisasm);

      Instruction instruction;
      instruction.VA = currentRuntimeAddr;
      instruction.InstructionLength = instructionInfo.info.length;
      instruction.Disassembly = finalDisasm;
      outInstructions->push_back(instruction);

      currentBufferPtr += instructionInfo.info.length;
      currentRuntimeAddr += instructionInfo.info.length;
      remainingLength -= instructionInfo.info.length;
    }
  }

  void Disassembler::Disassemble(HANDLE processHandle,
                                 DWORD processId,
                                 DWORD_PTR address,
                                 Instruction& outInstruction)
  {
    BYTE buffer[16];
    SIZE_T bytesRead = 0;

    if (!ReadProcessMemory(processHandle, (LPCVOID)address, buffer, sizeof(buffer), &bytesRead) || bytesRead == 0)
      return;

    ZydisDisassembledInstruction instructionInfo;
    GetInstruction(buffer, bytesRead, address, instructionInfo);

    SymbolResolver resolver(_processExplorer);
    std::string finalDisasm = instructionInfo.text;

    AppendOperandSymbols(instructionInfo, resolver, processHandle, processId, finalDisasm);

    outInstruction.VA = address;
    outInstruction.InstructionLength = instructionInfo.info.length;
    outInstruction.Disassembly = finalDisasm;
  }

  void Disassembler::AppendOperandSymbols(const ZydisDisassembledInstruction& info,
                                          SymbolResolver& symbolResolver,
                                          HANDLE processHandle,
                                          DWORD processId,
                                          std::string& disasmText)
  {
    for (int i = 0; i < info.info.operand_count_visible; ++i)
    {
      const auto& op = info.operands[i];
      ZyanU64 targetAddress = 0;
      bool hasAddress = false;

      // Try Official Zydis Calculation
      // Handles: [RIP + Disp], JMP/CALL Relatives
      if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&info.info, info.operands, i, &targetAddress)))
      {
        hasAddress = true;
      }

      // Fallback for Absolute Memory (e.g., PUSH [0x403228])
      else if (op.type == ZYDIS_OPERAND_TYPE_MEMORY &&
               op.mem.base == ZYDIS_REGISTER_NONE &&
               op.mem.index == ZYDIS_REGISTER_NONE)
      {
        if (op.mem.disp.value != 0)
        {
          targetAddress = (ZyanU64)op.mem.disp.value;
          hasAddress = true;
        }
      }
      // Fallback for Immediates (e.g., PUSH 0x403228)
      else if (op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
      {
        // Heuristic: Value > 64KB likely a pointer
        if (op.imm.is_signed && op.imm.value.s > 0x10000)
        {
          targetAddress = (ZyanU64)op.imm.value.s;
          hasAddress = true;
        }
        else if (!op.imm.is_signed && op.imm.value.u > 0x10000)
        {
          targetAddress = op.imm.value.u;
          hasAddress = true;
        }
      }

      if (hasAddress)
      {
        std::string sym = symbolResolver.Resolve(processHandle, processId, (DWORD_PTR)targetAddress);

        if (!sym.empty())
        {
          // Prevent duplicates
          if (disasmText.find(sym) == std::string::npos)
          {
            disasmText += " " + sym;
          }
        }
      }
    }
  }

  bool Disassembler::IsCallInstruction(HANDLE processHandle, DWORD_PTR address, size_t& instructionLength)
  {
    BYTE buffer[16];
    SIZE_T bytesRead = 0;

    if (!ReadProcessMemory(processHandle, (LPCVOID)address, buffer, sizeof(buffer), &bytesRead) || bytesRead == 0)
    {
      return false;
    }

    ZydisDisassembledInstruction instructionInfo;
    GetInstruction(buffer, bytesRead, address, instructionInfo);

    instructionLength = instructionInfo.info.length;

    if (instructionInfo.info.mnemonic == ZYDIS_MNEMONIC_CALL)
    {
      return true;
    }

    if (instructionInfo.info.attributes & ZYDIS_ATTRIB_HAS_REP ||
      instructionInfo.info.attributes & ZYDIS_ATTRIB_HAS_REPE ||
      instructionInfo.info.attributes & ZYDIS_ATTRIB_HAS_REPNE)
    {
      return true;
    }

    return false;
  }

  void Disassembler::GetInstruction(BYTE* address, SSIZE_T length, DWORD_PTR runtimeAddress, ZydisDisassembledInstruction& instructionInfo)
  {
#ifdef _WIN64
    ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, runtimeAddress, address, length, &instructionInfo);
#else
    ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_COMPAT_32, runtimeAddress, address, length, &instructionInfo);
#endif
  }
}