#pragma once

namespace MCP::Native::PEFile::Interfaces
{
  /// <summary>
  /// The headers of one executable file, read when the object was created.
  /// </summary>
  /// <remarks>
  /// Threading: not synchronised; used by the thread that opened the file.
  /// Ownership: the object holds its own copy of the file content.
  /// </remarks>
  class IPEFile
  {
  public:
    virtual ~IPEFile() = default;

    /// <summary>False when the file could not be read or is not a PE image; the other answers are then meaningless.</summary>
    virtual bool IsValid() = 0;

    virtual bool Isx64() = 0;

    /// <summary>False for an image that needs the .NET runtime.</summary>
    virtual bool IsNative() = 0;
  };
}
