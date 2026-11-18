#pragma once
#include ".\Interfaces\IProcessMemory.h"

namespace MCP::Native::Platform
{
  /// <summary>
  /// Reads process memory through the operating system. It forwards the call and holds no logic of its own,
  /// which is why it has no unit test: everything worth testing lives in the classes that use it.
  /// </summary>
  class ProcessMemory : public Interfaces::IProcessMemory
  {
  public:
    [[nodiscard]] bool Read(HANDLE processHandle, DWORD_PTR address, BYTE* buffer, size_t size) override;
  };
}
