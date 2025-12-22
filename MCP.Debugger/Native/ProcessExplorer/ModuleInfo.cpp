#include "ModuleInfo.h"
#include <filesystem>

namespace MCP::Native::ProcessExplorer
{
  ModuleInfo::ModuleInfo(void* baseAddress,
                         DWORD sizeOfImage,
                         std::wstring path,
                         std::vector<ModuleSection> sections,
                         std::vector<ModuleExport> exports,
                         std::vector<ModuleImport> imports)
    : _baseAddress(baseAddress),
      _sizeOfImage(sizeOfImage),
      _path(std::move(path)),
      _sections(std::move(sections)),
      _exports(std::move(exports)),
      _imports(std::move(imports))
  {
    _moduleName = ExtractFilename(_path);

    for (const auto& exp : _exports)
    {
      _exportsByAddress[exp.VirtualAddress] = exp;
      _exportsByOrdinal[exp.Ordinal] = exp;
      if (!exp.Name.empty())
      {
        _exportsByName[exp.Name] = exp;
      }
    }
  }

  FARPROC ModuleInfo::FindExportByName(LPCSTR lpProcName) const
  {
    if (lpProcName == nullptr) return nullptr;

    auto it = _exportsByName.find(lpProcName);
    if (it != _exportsByName.end())
    {
      return (FARPROC)it->second.VirtualAddress;
    }
    return nullptr;
  }

  FARPROC ModuleInfo::FindExportByOrdinal(DWORD ordinal) const
  {
    auto it = _exportsByOrdinal.find(ordinal);
    if (it != _exportsByOrdinal.end())
    {
      return (FARPROC)it->second.VirtualAddress;
    }
    return nullptr;
  }

  bool ModuleInfo::FindExportByAddress(void* va, ModuleExport& outExport) const
  {
    auto it = _exportsByAddress.find(va);
    if (it != _exportsByAddress.end())
    {
      outExport = it->second; // Copy the export data
      return true;
    }
    return false;
  }

  std::wstring ModuleInfo::ExtractFilename(const std::wstring& path)
  {
    std::filesystem::path p(path);
    return p.filename().wstring();
  }
}