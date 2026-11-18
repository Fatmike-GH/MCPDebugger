#pragma once
#include <Windows.h>
#include <memory>
#include "IPEImage.h"

namespace MCP::Native::ProcessExplorer::Interfaces
{
  /// <summary>
  /// Opens the PE images of modules that are loaded into a debuggee process.
  /// </summary>
  /// <remarks>
  /// Threading: debug thread only - opening an image reads the debuggee's memory.
  /// Ownership: the caller owns the returned object; the process handle is borrowed for the call.
  /// </remarks>
  class IPEImageFactory
  {
  public:
    virtual ~IPEImageFactory() = default;

    /// <summary>
    /// Reads the image at the base address. Never returns null: an image that cannot be read yields an
    /// object whose IsValid is false.
    /// </summary>
    virtual std::unique_ptr<IPEImage> Open(HANDLE processHandle, DWORD_PTR imageBase) = 0;
  };
}
