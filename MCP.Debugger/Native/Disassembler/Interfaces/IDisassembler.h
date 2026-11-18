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
    /// Decodes the instructions of an address range. The list is left as it was when the length is not
    /// positive or the memory of the range cannot be read as a whole; otherwise it is replaced by a new list:
    /// - Every entry has a text and a length of at least one byte. The entries follow each other without a
    ///   gap from the start address, and none reaches beyond the range.
    /// - A byte at which no instruction starts is reported as an entry of its own with the length 1 and the
    ///   text "db 0xNN" (NN = the byte, two upper-case hex digits), without a symbol; decoding goes on at
    ///   the next byte. The entries right behind such a byte may be wrong instructions until the decoder is
    ///   back in step with the code.
    /// - An instruction the end of the range cuts off is not reported: the list ends with the entry in front
    ///   of it and is empty when the range holds nothing but the beginning of one instruction.
    /// </summary>
    virtual void Disassemble(HANDLE processHandle,
                             DWORD processId,
                             DWORD_PTR startAddress,
                             SSIZE_T length,
                             std::shared_ptr<std::vector<Instruction>>& outInstructions) = 0;

    /// <summary>
    /// Tells whether the instruction at the address is one a step over must run past instead of into - a call
    /// or an instruction with a repeat prefix - and how long it is. The length is written on every path.
    /// When the memory at the address cannot be read or holds no instruction, the answer is false and the
    /// length is 0: a caller must not place a breakpoint behind an instruction whose length is unknown.
    /// </summary>
    virtual bool IsCallInstruction(HANDLE processHandle, DWORD_PTR address, size_t& instructionLength) = 0;
  };
}
