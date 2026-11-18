#pragma once
#include <Windows.h>
#include <memory>
#include "..\ProcessExplorer\Interfaces\IPEImageFactory.h"

namespace MCP::Native::Composition
{
  /// <summary>
  /// Opens module images in debuggee memory with the PE image reader of this library.
  /// </summary>
  class PEImageFactory : public MCP::Native::ProcessExplorer::Interfaces::IPEImageFactory
  {
  public:
    std::unique_ptr<MCP::Native::ProcessExplorer::Interfaces::IPEImage> Open(HANDLE processHandle, DWORD_PTR imageBase) override;
  };
}
