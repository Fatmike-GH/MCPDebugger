#pragma once
#include "Native/PEFile/Interfaces/IExecutableValidator.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::PEFile::Interfaces
{
  /// <summary>
  /// gMock double of the check that decides whether a file can be debugged.
  /// </summary>
  class MockExecutableValidator : public MCP::Native::PEFile::Interfaces::IExecutableValidator
  {
  public:
    MOCK_METHOD(bool, Validate, (const std::string& path, std::string& error), (override));
  };
}
