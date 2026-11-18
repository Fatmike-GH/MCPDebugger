#pragma once
#include <Windows.h>
#include <memory>
#include <vector>
#include "..\Instruction.h"

namespace MCP::Native::Disassembler::Interfaces
{
  /// <summary>
  /// Decodes machine code of a debuggee process into instructions with their text.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only - decoding reads the debuggee's memory.
  /// Ownership: handles are borrowed for the call; the instruction list is handed out as a shared pointer.
  /// </remarks>
  class IDisassembler
  {
  public:
    virtual ~IDisassembler() = default;

    /// <summary>
    /// Decodes the instructions of an address range. The list stays empty or unset when the memory cannot be
    /// read.
    /// </summary>
    virtual void Disassemble(HANDLE processHandle,
                             DWORD processId,
                             DWORD_PTR startAddress,
                             SSIZE_T length,
                             std::shared_ptr<std::vector<Instruction>>& outInstructions) = 0;

    /// <summary>
    /// Tells whether the instruction at the address is one a step over must run past instead of into, and how
    /// long it is.
    /// </summary>
    virtual bool IsCallInstruction(HANDLE processHandle, DWORD_PTR address, size_t& instructionLength) = 0;
  };
}
