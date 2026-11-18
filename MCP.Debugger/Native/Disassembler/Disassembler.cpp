#define ZYDIS_STATIC_BUILD
#include "Disassembler.h"
#include ".\Interfaces\ISymbolResolver.h"
#include "..\Platform\Interfaces\IProcessMemory.h"
#include <array>
#include <cstdio>

namespace MCP::Native::Disassembler
{
  namespace
  {
    // Number of bytes the call check reads. The longest instruction of the architecture has 15 bytes, so a
    // buffer of this size never ends inside the instruction at its start.
    constexpr size_t CallCheckReadSize = 16;

    // Text of the entry that stands for one byte that is no instruction: the notation disassemblers use for
    // a data byte, with the value written like the numbers in the instruction texts.
    std::string FormatDataByte(BYTE value)
    {
      std::array<char, sizeof("db 0xNN")> text{};
      std::snprintf(text.data(), text.size(), "db 0x%02X", static_cast<unsigned int>(value));
      return std::string(text.data());
    }
  }

  Disassembler::Disassembler(Interfaces::ISymbolResolver& symbolResolver,
                             MCP::Native::Platform::Interfaces::IProcessMemory& processMemory)
    : _symbolResolver(symbolResolver),
      _processMemory(processMemory)
  {
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

    std::vector<BYTE> buffer(static_cast<size_t>(length));
    if (!_processMemory.Read(processHandle, startAddress, buffer.data(), buffer.size()))
    {
      return;
    }

    outInstructions = std::make_shared<std::vector<Instruction>>();
    size_t offset = 0;

    // Every pass reports an entry of at least one byte or ends the loop, so the loop ends and no entry
    // reaches beyond the range
    while (offset < buffer.size())
    {
      const DWORD_PTR address = startAddress + offset;
      ZydisDisassembledInstruction decoded{};
      const DecodeOutcome outcome = Decode(buffer.data() + offset, buffer.size() - offset, address, decoded);

      if (outcome == DecodeOutcome::CutOff)
      {
        // The range ends inside an instruction. Nothing is reported for its bytes: they are neither a
        // complete instruction nor known to be data, and only the bytes behind the range could tell
        break;
      }

      Instruction instruction;
      instruction.VA = address;
      if (outcome == DecodeOutcome::Decoded)
      {
        instruction.InstructionLength = decoded.info.length;
        instruction.Disassembly = decoded.text;
        AppendOperandSymbols(decoded, address, _symbolResolver, processHandle, processId, instruction.Disassembly);
      }
      else
      {
        // No instruction starts here. Only this one byte is reported as data, because an instruction may
        // start at the very next byte
        instruction.InstructionLength = 1;
        instruction.Disassembly = FormatDataByte(buffer[offset]);
      }

      offset += instruction.InstructionLength;
      outInstructions->push_back(std::move(instruction));
    }
  }

  void Disassembler::AppendOperandSymbols(const ZydisDisassembledInstruction& info,
                                          DWORD_PTR runtimeAddress,
                                          Interfaces::ISymbolResolver& symbolResolver,
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
      // Handles: [RIP + Disp], JMP/CALL Relatives, absolute memory ([Disp])
      // It works on one operand - the one this pass looks at - and needs the address the instruction runs
      // at: a relative target is that address plus the instruction length plus the displacement
      if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&info.info, &op, runtimeAddress, &targetAddress)))
      {
        hasAddress = true;
      }

      // Fallback for Absolute Memory (e.g., PUSH [0x403228]). The calculation above accepts such an operand
      // itself, so this only takes over should it ever decline one
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
    // Without a decoded instruction there is no length. The caller runs to a breakpoint behind the
    // instruction only for a call, so answering "no call" keeps it from placing one at a guessed address
    instructionLength = 0;

    std::array<BYTE, CallCheckReadSize> buffer{};
    if (!_processMemory.Read(processHandle, address, buffer.data(), buffer.size()))
    {
      return false;
    }

    ZydisDisassembledInstruction decoded{};
    if (Decode(buffer.data(), buffer.size(), address, decoded) != DecodeOutcome::Decoded)
    {
      return false;
    }

    instructionLength = decoded.info.length;

    if (decoded.info.mnemonic == ZYDIS_MNEMONIC_CALL)
    {
      return true;
    }

    // A repeated string instruction is run past like a call: stepping into it would stop once per repetition
    const ZydisInstructionAttributes repeatPrefixes = ZYDIS_ATTRIB_HAS_REP | ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE;
    return (decoded.info.attributes & repeatPrefixes) != 0;
  }

  Disassembler::DecodeOutcome Disassembler::Decode(const BYTE* bytes, size_t size, DWORD_PTR runtimeAddress, ZydisDisassembledInstruction& instruction)
  {
#ifdef _WIN64
    const ZydisMachineMode machineMode = ZYDIS_MACHINE_MODE_LONG_64;
#else
    const ZydisMachineMode machineMode = ZYDIS_MACHINE_MODE_LONG_COMPAT_32;
#endif

    const ZyanStatus status = ZydisDisassembleIntel(machineMode, runtimeAddress, bytes, size, &instruction);
    if (status == ZYDIS_STATUS_NO_MORE_DATA)
    {
      return DecodeOutcome::CutOff;
    }

    if (!ZYAN_SUCCESS(status))
    {
      return DecodeOutcome::Undecodable;
    }

    // Callers advance by the length of a decoded instruction. A length of zero would never advance and one
    // beyond the given bytes would leave them, so such a result is not passed on as an instruction, whatever
    // the decoder reported
    if (instruction.info.length == 0 || instruction.info.length > size)
    {
      return DecodeOutcome::Undecodable;
    }

    return DecodeOutcome::Decoded;
  }
}