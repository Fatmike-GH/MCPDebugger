#pragma once
#include <memory>
#include <string>
#include ".\Interfaces\IExecutableValidator.h"

namespace MCP::Native::PEFile::Interfaces
{
  class IPEFileFactory;
}

namespace MCP::Native::PEFile
{
  /// <summary>
  /// Accepts a file when its PE headers describe a native executable of this build's bitness: a 64-bit build
  /// debugs 64-bit images only, a 32-bit build 32-bit images only.
  /// </summary>
  class ExecutableValidator : public Interfaces::IExecutableValidator
  {
  public:
    /// <summary>Takes ownership of the factory it opens the files with.</summary>
    explicit ExecutableValidator(std::unique_ptr<Interfaces::IPEFileFactory> peFileFactory);
    ~ExecutableValidator() override;

    ExecutableValidator(const ExecutableValidator&) = delete;
    ExecutableValidator& operator=(const ExecutableValidator&) = delete;
    ExecutableValidator(ExecutableValidator&&) = delete;
    ExecutableValidator& operator=(ExecutableValidator&&) = delete;

    bool Validate(const std::string& path, std::string& error) override;

  private:
    std::unique_ptr<Interfaces::IPEFileFactory> _peFileFactory;
  };
}
