#pragma once
#include <string>

namespace MCP::Native::PEFile::Interfaces
{
  /// <summary>
  /// Decides whether a file can be debugged by this build: a native executable of the build's bitness.
  /// </summary>
  /// <remarks>
  /// Threading: called on the thread that starts the debugger.
  /// Ownership: borrows both arguments for the duration of the call.
  /// </remarks>
  class IExecutableValidator
  {
  public:
    virtual ~IExecutableValidator() = default;

    /// <summary>True when the file can be debugged; false with the reason in error (unchanged otherwise).</summary>
    virtual bool Validate(const std::string& path, std::string& error) = 0;
  };
}
