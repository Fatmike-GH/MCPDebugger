#pragma once
#include "Native/PEFile/Interfaces/IPEFile.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::PEFile::Interfaces
{
  /// <summary>
  /// gMock double of the headers of an executable file.
  /// </summary>
  class MockPEFile : public MCP::Native::PEFile::Interfaces::IPEFile
  {
  public:
    MOCK_METHOD(bool, IsValid, (), (override));
    MOCK_METHOD(bool, Isx64, (), (override));
    MOCK_METHOD(bool, IsNative, (), (override));
  };
}
