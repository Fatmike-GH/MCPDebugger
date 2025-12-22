#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "ModuleSection.h"
#include "ModuleExport.h"
#include "ModuleImport.h"

namespace MCP::Native::ProcessExplorer
{
  class ModuleInfo
  {
  public:
    ModuleInfo(void* baseAddress,
               DWORD sizeOfImage,
               std::wstring path,
               std::vector<ModuleSection> sections,
               std::vector<ModuleExport> exports,
               std::vector<ModuleImport> imports);

    FARPROC FindExportByName(LPCSTR lpProcName) const;
    FARPROC FindExportByOrdinal(DWORD ordinal) const;
    bool FindExportByAddress(void* va, ModuleExport& outExport) const;

    HANDLE GetModuleHandle() const { return _baseAddress; }
    void* GetBaseAddress() const { return _baseAddress; }
    DWORD GetSizeOfImage() const { return _sizeOfImage; }
    const std::wstring& GetPath() const { return _path; }
    const std::wstring& GetModuleName() const { return _moduleName; }
    const std::vector<ModuleSection>& GetSections() const { return _sections; }
    const std::vector<ModuleExport>& GetExports() const { return _exports; }
    const std::vector<ModuleImport>& GetImports() const { return _imports; }

  private:
    std::wstring ExtractFilename(const std::wstring& path);

  private:
    void* _baseAddress = nullptr;
    DWORD _sizeOfImage = 0;
    std::wstring _path;
    std::wstring _moduleName;
    std::vector<ModuleSection> _sections;
    std::vector<ModuleExport> _exports;
    std::vector<ModuleImport> _imports;

    std::map<std::string, ModuleExport> _exportsByName;
    std::map<DWORD, ModuleExport> _exportsByOrdinal;
    std::map<void*, ModuleExport> _exportsByAddress;
  };

  using ModuleInfoPtr = std::shared_ptr<ModuleInfo>;
  using ModuleMap = std::map<void*, ModuleInfoPtr>;
}