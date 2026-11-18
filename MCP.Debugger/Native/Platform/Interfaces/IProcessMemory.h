#pragma once
#include <Windows.h>
#include <cstddef>

namespace MCP::Native::Platform::Interfaces
{
  /// <summary>
  /// Reads the memory of a process. Classes with logic reach process memory through this contract instead of
  /// calling the operating system themselves, so their logic can be tested with a double of it.
  /// </summary>
  /// <remarks>
  /// Threading: stateless, callable from any thread.
  /// Ownership: the process handle and the buffer are borrowed for the call.
  /// </remarks>
  class IProcessMemory
  {
  public:
    virtual ~IProcessMemory() = default;

    /// <summary>
    /// Copies the given number of bytes at the address into the buffer, which must have room for them. True
    /// when all of them were copied. False when the range cannot be read as a whole: the content of the buffer
    /// is then unspecified. A part of the range is never reported as a success.
    /// </summary>
    [[nodiscard]] virtual bool Read(HANDLE processHandle, DWORD_PTR address, BYTE* buffer, size_t size) = 0;
  };
}
