#include "PEImageFactory.h"
#include "..\ProcessExplorer\PEImage.h"

namespace MCP::Native::Composition
{
  std::unique_ptr<MCP::Native::ProcessExplorer::Interfaces::IPEImage> PEImageFactory::Open(HANDLE processHandle, DWORD_PTR imageBase)
  {
    return std::make_unique<MCP::Native::ProcessExplorer::PEImage>(processHandle, imageBase);
  }
}
