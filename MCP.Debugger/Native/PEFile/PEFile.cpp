#include "PEFile.h"
#include <fstream> // For std::ifstream

namespace MCP::Native::PEFile
{
  PEFile::PEFile(const std::string& path)
    : _ntHeaders(nullptr),
      _valid(false)
  {
    if (!OpenFile(path))
      return;

    _valid = ParseHeaders();
  }

  PEFile::~PEFile()
  {
  }

  bool PEFile::Isx64()
  {
    if (!IsValid())
    {
      return false;
    }
    return (_ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);
  }

  bool PEFile::IsNative()
  {
    if (!IsValid())
    {
      return false;
    }

    PIMAGE_DATA_DIRECTORY comDirectory = &_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
    return (comDirectory->VirtualAddress == 0 && comDirectory->Size == 0);
  }

  bool PEFile::IsValid()
  {
    return _valid;
  }

  bool PEFile::OpenFile(const std::string& path)
  {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
      return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < sizeof(IMAGE_DOS_HEADER))
    {
      file.close();
      return false;
    }

    _buffer.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(_buffer.data()), size))
    {
      file.close();
      return false;
    }

    file.close();
    return true;
  }

  bool PEFile::ParseHeaders()
  {
    PIMAGE_DOS_HEADER pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(_buffer.data());

    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
      return false;
    }

    if (pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS) > _buffer.size())
    {
      return false;
    }

    _ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(_buffer.data() + pDosHeader->e_lfanew);
    if (_ntHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
      _ntHeaders = nullptr;
      return false;
    }
    return true;
  }
}