#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include "ModuleSection.h"
#include "ModuleExport.h"
#include "ModuleImport.h"

namespace MCP::Native::ProcessExplorer
{
  class PEImage
  {
  public:
    PEImage(HANDLE processHandle, DWORD_PTR imageBase);
    ~PEImage() = default;

    bool IsValid() const { return _valid; }
    DWORD_PTR GetImageBase() const { return _imageBase; }
    DWORD GetSizeOfImage() const { return _sizeOfImage; }

    const std::vector<ModuleSection>& GetSections() const { return _sections; }
    const std::vector<ModuleExport>& GetExports() const { return _exports; }
    const std::vector<ModuleImport>& GetImports() const { return _imports; }

    const IMAGE_DOS_HEADER& GetDosHeader() const { return _dosHeader; }
    const IMAGE_NT_HEADERS& GetNtHeaders() const { return _ntHeaders; }

  private:
    // Main loading sequence
    void Load();
    bool LoadDosHeader();
    bool LoadNtHeaders();
    bool LoadAndParseSections();
    bool LoadAndParseExports();
    bool LoadAndParseImports();

    // Section parsing
    void ParseSections(const std::vector<IMAGE_SECTION_HEADER>& sectionHeaders);
    DWORD GetSectionHeadersRVA() const;

    // Export parsing helpers
    bool ParseExports(const IMAGE_DATA_DIRECTORY& exportDir);
    bool ReadExportDirectory(DWORD rva, IMAGE_EXPORT_DIRECTORY& outExpDir);
    bool ReadExportTables(const IMAGE_EXPORT_DIRECTORY& expDir, std::vector<DWORD>& outFunctions, std::vector<DWORD>& outNames, std::vector<WORD>& outOrdinals);
    std::map<DWORD, std::string> BuildExportNameMap(const IMAGE_EXPORT_DIRECTORY& expDir, const std::vector<DWORD>& rvaNames, const std::vector<WORD>& rvaOrdinals);
    void ProcessExportFunctions(const IMAGE_EXPORT_DIRECTORY& expDir, const std::vector<DWORD>& rvaFunctions, const std::map<DWORD, std::string>& nameMap, const IMAGE_DATA_DIRECTORY& exportDirRegion);
    std::string ReadForwardedExport(DWORD_PTR rva);

    // Import parsing helpers
    bool ParseImports(const IMAGE_DATA_DIRECTORY& importDir);
    void ParseImportFunctionsForModule(const IMAGE_IMPORT_DESCRIPTOR& importDesc, ModuleImport& outModuleImport);
    void ParseImportThunk(const IMAGE_THUNK_DATA& thunkData, DWORD_PTR pIatEntryRVA, ModuleImportFunction& outFunc);

    // Utility helpers
    bool ReadData(DWORD_PTR rva, void* buffer, SIZE_T size);
    std::string ReadStringFromRVA(DWORD_PTR rva);


  private:
    HANDLE _processHandle = nullptr;
    DWORD_PTR _imageBase = 0;
    bool _valid = false;

    DWORD _sizeOfImage = 0;
    IMAGE_DOS_HEADER _dosHeader = { 0 };
    IMAGE_NT_HEADERS _ntHeaders = { 0 };

    std::vector<ModuleSection> _sections;
    std::vector<ModuleExport> _exports;
    std::vector<ModuleImport> _imports;
  };
}