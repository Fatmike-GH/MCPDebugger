#include "PEFileFactory.h"
#include "..\PEFile\PEFile.h"

namespace MCP::Native::Composition
{
  std::unique_ptr<MCP::Native::PEFile::Interfaces::IPEFile> PEFileFactory::Open(const std::string& path)
  {
    return std::make_unique<MCP::Native::PEFile::PEFile>(path);
  }
}
