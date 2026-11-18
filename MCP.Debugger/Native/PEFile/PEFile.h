#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include ".\Interfaces\IPEFile.h"

namespace MCP::Native::PEFile
{
  class PEFile : public Interfaces::IPEFile
  {
  public:
    PEFile(const std::string& path);
    ~PEFile() override;

    bool IsValid() override;
    bool Isx64() override;
    bool IsNative() override;   

  private:
    bool OpenFile(const std::string& path);
    bool ParseHeaders();

    std::vector<BYTE>_buffer;
    PIMAGE_NT_HEADERS _ntHeaders;
    bool _valid;
  };
}