#pragma once
#include <memory>
#include <string>
#include "IPEFile.h"

namespace MCP::Native::PEFile::Interfaces
{
  /// <summary>
  /// Opens executable files for inspection.
  /// </summary>
  /// <remarks>
  /// Ownership: the caller owns the returned object.
  /// Threading: may be called from any thread; each call reads the file anew.
  /// </remarks>
  class IPEFileFactory
  {
  public:
    virtual ~IPEFileFactory() = default;

    /// <summary>
    /// Reads the file. Never returns null: a file that is missing or unreadable yields an object whose
    /// IsValid is false.
    /// </summary>
    virtual std::unique_ptr<IPEFile> Open(const std::string& path) = 0;
  };
}
