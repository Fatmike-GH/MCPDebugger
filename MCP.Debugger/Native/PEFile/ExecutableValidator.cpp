#include "ExecutableValidator.h"
#include ".\Interfaces\IPEFile.h"
#include ".\Interfaces\IPEFileFactory.h"

namespace MCP::Native::PEFile
{
  ExecutableValidator::ExecutableValidator(std::unique_ptr<Interfaces::IPEFileFactory> peFileFactory)
    : _peFileFactory(std::move(peFileFactory))
  {
  }

  ExecutableValidator::~ExecutableValidator() = default;

  bool ExecutableValidator::Validate(const std::string& path, std::string& error)
  {
    std::unique_ptr<Interfaces::IPEFile> peFile = _peFileFactory->Open(path);

#ifdef _WIN64
    if (!peFile->IsValid() || !peFile->Isx64() || !peFile->IsNative())
    {
      error = "The provided file is not a valid x64 PE file.";
      return false;
    }
#else
    if (!peFile->IsValid() || peFile->Isx64() || !peFile->IsNative())
    {
      error = "The provided file is not a valid x86 PE file.";
      return false;
    }
#endif
    return true;
  }
}
