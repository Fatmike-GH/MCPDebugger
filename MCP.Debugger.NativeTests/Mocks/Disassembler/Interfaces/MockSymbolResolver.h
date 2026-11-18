#pragma once
#include "Native/Disassembler/Interfaces/ISymbolResolver.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::Disassembler::Interfaces
{
  /// <summary>
  /// gMock double of the resolver that gives debuggee addresses a readable meaning.
  /// </summary>
  class MockSymbolResolver : public MCP::Native::Disassembler::Interfaces::ISymbolResolver
  {
  public:
    MOCK_METHOD(std::string, Resolve, (HANDLE hProcess, DWORD processId, DWORD_PTR address), (override));
    MOCK_METHOD(std::string, ResolveString, (HANDLE hProcess, DWORD_PTR address), (override));
  };
}
