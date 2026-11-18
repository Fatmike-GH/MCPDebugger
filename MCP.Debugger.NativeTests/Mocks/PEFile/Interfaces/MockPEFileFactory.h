#pragma once
#include "Native/PEFile/Interfaces/IPEFileFactory.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::PEFile::Interfaces
{
  /// <summary>
  /// gMock double of the factory that opens executable files.
  /// </summary>
  class MockPEFileFactory : public MCP::Native::PEFile::Interfaces::IPEFileFactory
  {
  public:
    MOCK_METHOD(std::unique_ptr<MCP::Native::PEFile::Interfaces::IPEFile>, Open, (const std::string& path), (override));
  };
}
