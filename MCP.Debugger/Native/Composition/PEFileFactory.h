#pragma once
#include <memory>
#include <string>
#include "..\PEFile\Interfaces\IPEFileFactory.h"

namespace MCP::Native::Composition
{
  /// <summary>
  /// Opens executable files with the PE file reader of this library.
  /// </summary>
  class PEFileFactory : public MCP::Native::PEFile::Interfaces::IPEFileFactory
  {
  public:
    std::unique_ptr<MCP::Native::PEFile::Interfaces::IPEFile> Open(const std::string& path) override;
  };
}
