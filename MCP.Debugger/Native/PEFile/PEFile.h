#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace MCP::Native::PEFile
{
  class PEFile
  {
  public:
    PEFile(const std::string& path);
    ~PEFile();

    bool IsValid();
    bool Isx64();
    bool IsNative();   

  private:
    bool OpenFile(const std::string& path);
    bool ParseHeaders();

    std::vector<BYTE>_buffer;
    PIMAGE_NT_HEADERS _ntHeaders;
    bool _valid;
  };
}