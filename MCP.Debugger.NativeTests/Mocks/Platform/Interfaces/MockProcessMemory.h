#pragma once
#include "Native/Platform/Interfaces/IProcessMemory.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Platform::Interfaces
{
  /// <summary>
  /// gMock double of the reader of a process's memory.
  /// </summary>
  class MockProcessMemory : public MCP::Native::Platform::Interfaces::IProcessMemory
  {
  public:
    MOCK_METHOD(bool, Read, (HANDLE processHandle, DWORD_PTR address, BYTE* buffer, size_t size), (override));
  };
}
