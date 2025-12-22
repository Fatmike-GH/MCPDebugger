#include "PEImage.h"
#include <stddef.h>

namespace MCP::Native::ProcessExplorer
{
  PEImage::PEImage(HANDLE processHandle, DWORD_PTR imageBase)
    : _processHandle(processHandle), _imageBase(imageBase)
  {
    Load();
  }

  void PEImage::Load()
  {
    if (_processHandle == nullptr || _imageBase == 0) return;

    if (!LoadDosHeader()) return;
    if (!LoadNtHeaders()) return;

    _sizeOfImage = _ntHeaders.OptionalHeader.SizeOfImage;

    if (!LoadAndParseSections()) return;
    if (!LoadAndParseExports()) return;
    if (!LoadAndParseImports()) return;

    _valid = true;
  }

  bool PEImage::LoadDosHeader()
  {
    if (!ReadData(0, &_dosHeader, sizeof(_dosHeader)))
    {
      return false;
    }

    return (_dosHeader.e_magic == IMAGE_DOS_SIGNATURE);
  }

  bool PEImage::LoadNtHeaders()
  {
    if (!ReadData(_dosHeader.e_lfanew, &_ntHeaders, sizeof(_ntHeaders)))
    {
      return false;
    }

    return (_ntHeaders.Signature == IMAGE_NT_SIGNATURE);
  }

  DWORD PEImage::GetSectionHeadersRVA() const
  {
    return _dosHeader.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + _ntHeaders.FileHeader.SizeOfOptionalHeader;
  }

  bool PEImage::LoadAndParseSections()
  {
    WORD numSections = _ntHeaders.FileHeader.NumberOfSections;
    if (numSections == 0) return true; // No sections is not an error

    DWORD sectionHeadersSize = numSections * sizeof(IMAGE_SECTION_HEADER);
    std::vector<IMAGE_SECTION_HEADER> sectionHeaders(numSections);

    DWORD sectionHeadersRVA = GetSectionHeadersRVA();

    if (!ReadData(sectionHeadersRVA, sectionHeaders.data(), sectionHeadersSize))
    {
      return false;
    }

    ParseSections(sectionHeaders);
    return true;
  }

  bool PEImage::LoadAndParseExports()
  {
    IMAGE_DATA_DIRECTORY exportDir = _ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (exportDir.VirtualAddress == 0 || exportDir.Size == 0)
    {
      return true; // No exports is not an error
    }

    return ParseExports(exportDir);
  }

  bool PEImage::LoadAndParseImports()
  {
    const auto& importDir = _ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.VirtualAddress == 0 || importDir.Size == 0)
    {
      return true; // No imports, not an error
    }

    return ParseImports(importDir);
  }

  void PEImage::ParseSections(const std::vector<IMAGE_SECTION_HEADER>& sectionHeaders)
  {
    BYTE* base = (BYTE*)_imageBase;
    _sections.reserve(sectionHeaders.size());

    for (const auto& header : sectionHeaders)
    {
      ModuleSection section;
      strncpy_s(section.Name, (const char*)header.Name, IMAGE_SIZEOF_SHORT_NAME);
      section.BaseAddress = base + header.VirtualAddress;
      section.VirtualSize = header.Misc.VirtualSize;
      section.Characteristics = header.Characteristics;
      _sections.push_back(section);
    }
  }

  bool PEImage::ParseExports(const IMAGE_DATA_DIRECTORY& exportDir)
  {
    IMAGE_EXPORT_DIRECTORY expDir = {};
    if (!ReadExportDirectory(exportDir.VirtualAddress, expDir))
    {
      return false;
    }

    std::vector<DWORD> rvaFunctions;
    std::vector<DWORD> rvaNames;
    std::vector<WORD> rvaOrdinals;

    if (!ReadExportTables(expDir, rvaFunctions, rvaNames, rvaOrdinals))
    {
      return false;
    }

    std::map<DWORD, std::string> nameMap = BuildExportNameMap(expDir, rvaNames, rvaOrdinals);
    ProcessExportFunctions(expDir, rvaFunctions, nameMap, exportDir);

    return true;
  }

  bool PEImage::ParseImports(const IMAGE_DATA_DIRECTORY& importDir)
  {
    DWORD_PTR pDescriptorRVA = importDir.VirtualAddress;
    IMAGE_IMPORT_DESCRIPTOR importDesc;

    while (ReadData(pDescriptorRVA, &importDesc, sizeof(importDesc)) && importDesc.Name != 0)
    {
      ModuleImport moduleImport;
      moduleImport.ModuleName = ReadStringFromRVA(importDesc.Name);

      if (moduleImport.ModuleName.empty())
      {
        pDescriptorRVA += sizeof(IMAGE_IMPORT_DESCRIPTOR);
        continue; // Skip if name is invalid
      }

      ParseImportFunctionsForModule(importDesc, moduleImport);

      if (!moduleImport.Functions.empty())
      {
        _imports.push_back(std::move(moduleImport));
      }

      pDescriptorRVA += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }

    return true;
  }

  void PEImage::ParseImportFunctionsForModule(const IMAGE_IMPORT_DESCRIPTOR& importDesc, ModuleImport& outModuleImport)
  {
    DWORD_PTR pThunkRVA = importDesc.OriginalFirstThunk ? importDesc.OriginalFirstThunk : importDesc.FirstThunk;
    DWORD_PTR pIatEntryRVA = importDesc.FirstThunk;

    if (pThunkRVA == 0) return;

    IMAGE_THUNK_DATA thunkData;
    while (ReadData(pThunkRVA, &thunkData, sizeof(thunkData)) && thunkData.u1.AddressOfData != 0)
    {
      ModuleImportFunction impFunc;
      ParseImportThunk(thunkData, pIatEntryRVA, impFunc);
      outModuleImport.Functions.push_back(impFunc);

      pThunkRVA += sizeof(IMAGE_THUNK_DATA);
      pIatEntryRVA += sizeof(DWORD_PTR);
    }
  }

  void PEImage::ParseImportThunk(const IMAGE_THUNK_DATA& thunkData, DWORD_PTR pIatEntryRVA, ModuleImportFunction& outFunc)
  {
    if (IMAGE_SNAP_BY_ORDINAL(thunkData.u1.Ordinal))
    {
      outFunc.ImportByName = false;
      outFunc.Ordinal = IMAGE_ORDINAL(thunkData.u1.Ordinal);
    }
    else
    {
      outFunc.ImportByName = true;
      outFunc.Name = ReadStringFromRVA(thunkData.u1.AddressOfData + offsetof(IMAGE_IMPORT_BY_NAME, Name));
    }
    outFunc.IATEntryAddress = (void*)(_imageBase + pIatEntryRVA);
  }

  bool PEImage::ReadExportDirectory(DWORD rva, IMAGE_EXPORT_DIRECTORY& outExpDir)
  {
    return ReadData(rva, &outExpDir, sizeof(outExpDir));
  }

  bool PEImage::ReadExportTables(const IMAGE_EXPORT_DIRECTORY& expDir,
                                 std::vector<DWORD>& outFunctions,
                                 std::vector<DWORD>& outNames,
                                 std::vector<WORD>& outOrdinals)
  {
    outFunctions.resize(expDir.NumberOfFunctions);
    outNames.resize(expDir.NumberOfNames);
    outOrdinals.resize(expDir.NumberOfNames);

    bool readFuncs = ReadData(expDir.AddressOfFunctions, outFunctions.data(), sizeof(DWORD) * expDir.NumberOfFunctions);
    bool readNames = ReadData(expDir.AddressOfNames, outNames.data(), sizeof(DWORD) * expDir.NumberOfNames);
    bool readOrds = ReadData(expDir.AddressOfNameOrdinals, outOrdinals.data(), sizeof(WORD) * expDir.NumberOfNames);

    return (readFuncs && readNames && readOrds);
  }

  std::map<DWORD, std::string> PEImage::BuildExportNameMap(const IMAGE_EXPORT_DIRECTORY& expDir, const std::vector<DWORD>& rvaNames, const std::vector<WORD>& rvaOrdinals)
  {
    std::map<DWORD, std::string> nameMap;
    for (DWORD i = 0; i < expDir.NumberOfNames; ++i)
    {
      std::string name = ReadStringFromRVA(rvaNames[i]);
      if (!name.empty())
      {
        nameMap[rvaOrdinals[i]] = name;
      }
    }
    return nameMap;
  }

  void PEImage::ProcessExportFunctions(const IMAGE_EXPORT_DIRECTORY& expDir,
                                       const std::vector<DWORD>& rvaFunctions,
                                       const std::map<DWORD, std::string>& nameMap,
                                       const IMAGE_DATA_DIRECTORY& exportDirRegion)
  {
    BYTE* base = (BYTE*)_imageBase;
    _exports.reserve(expDir.NumberOfFunctions);

    for (DWORD i = 0; i < expDir.NumberOfFunctions; ++i)
    {
      ModuleExport exp;
      DWORD funcRVA = rvaFunctions[i];
      if (funcRVA == 0) continue;

      exp.Ordinal = expDir.Base + i;

      auto nameIt = nameMap.find((WORD)i);
      if (nameIt != nameMap.end())
      {
        exp.Name = nameIt->second;
      }

      // Check if RVA points inside the export directory itself
      bool isForwarded = (funcRVA >= exportDirRegion.VirtualAddress && funcRVA < (exportDirRegion.VirtualAddress + exportDirRegion.Size));

      if (isForwarded)
      {
        exp.ForwardedName = ReadForwardedExport(funcRVA);
        exp.VirtualAddress = nullptr;
      }
      else
      {
        exp.VirtualAddress = base + funcRVA;
      }

      _exports.push_back(exp);
    }
  }

  std::string PEImage::ReadForwardedExport(DWORD_PTR rva)
  {
    return ReadStringFromRVA(rva);
  }

  bool PEImage::ReadData(DWORD_PTR rva, void* buffer, SIZE_T size)
  {
    if (_processHandle == nullptr || buffer == nullptr || size == 0)
    {
      return false;
    }

    SIZE_T bytesRead = 0;
    BOOL success = ReadProcessMemory(_processHandle, (LPCVOID)(_imageBase + rva), buffer, size, &bytesRead);

    return success && (bytesRead == size);
  }

  std::string PEImage::ReadStringFromRVA(DWORD_PTR rva)
  {
    if (rva == 0) return std::string();

    char buffer[256];
    if (!ReadData(rva, buffer, sizeof(buffer) - 1))
    {
      return std::string();
    }

    buffer[sizeof(buffer) - 1] = '\0';
    return std::string(buffer);
  }
}