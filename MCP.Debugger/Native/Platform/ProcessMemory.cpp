#include "ProcessMemory.h"

namespace MCP::Native::Platform
{
  bool ProcessMemory::Read(HANDLE processHandle, DWORD_PTR address, BYTE* buffer, size_t size)
  {
    // The system call takes the address in the other process as a pointer. It copies the whole range or
    // fails, so its success alone answers the contract and the number of bytes read is not asked for.
    return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), buffer, size, nullptr) != FALSE;
  }
}
