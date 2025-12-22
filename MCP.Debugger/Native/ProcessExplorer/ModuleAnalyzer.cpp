#include "ModuleAnalyzer.h"
#include "ModuleInfo.h"
#include <psapi.h>

namespace MCP::Native::ProcessExplorer
{
  ModuleInfoPtr ModuleAnalyzer::AnalyzeModule(HANDLE hProcess, HANDLE hFile, void* baseAddress)
  {
    if (hProcess == nullptr || baseAddress == nullptr) return nullptr;

    PEImage peImage(hProcess, (DWORD_PTR)baseAddress);
    if (!peImage.IsValid())
    {
      return nullptr;
    }

    return CreateModuleInfo(peImage, hProcess, hFile, baseAddress);
  }

  ModuleInfoPtr ModuleAnalyzer::CreateModuleInfo(PEImage& peImage, HANDLE hProcess, HANDLE hFile, void* baseAddress)
  {
    std::wstring path = GetPathFromHandle(hFile);

    if (path.empty() && hProcess != nullptr)
    {
      path = GetPathFromProcess(hProcess);
    }

    if (path.empty() && hProcess != nullptr && baseAddress != nullptr)
    {
      path = GetPathFromBaseAddress(hProcess, baseAddress);
    }

    DWORD sizeOfImage = peImage.GetSizeOfImage();

    return std::make_shared<ModuleInfo>(
      baseAddress,
      sizeOfImage,
      std::move(path),
      peImage.GetSections(),
      peImage.GetExports(),
      peImage.GetImports()
    );
  }

  std::wstring ModuleAnalyzer::GetPathFromHandle(HANDLE hFile)
  {
    if (hFile == nullptr) return std::wstring();

    wchar_t buffer[MAX_PATH];
    if (GetFinalPathNameByHandleW(hFile, buffer, MAX_PATH, FILE_NAME_NORMALIZED) == 0)
    {
      return std::wstring();
    }

    std::wstring path(buffer);
    static const std::wstring prefix = L"\\\\?\\";
    if (path.rfind(prefix, 0) == 0)
    {
      return path.substr(prefix.length());
    }

    return path;
  }

  std::wstring ModuleAnalyzer::GetPathFromProcess(HANDLE hProcess)
  {
    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
      return std::wstring(path);
    }
    return L"";
  }

  std::wstring ModuleAnalyzer::GetPathFromBaseAddress(HANDLE hProcess, void* baseAddress)
  {
    wchar_t mappedPath[MAX_PATH];
    if (GetMappedFileNameW(hProcess, baseAddress, mappedPath, MAX_PATH))
    {
      // This returns a device path
      return std::wstring(mappedPath);
    }
    return L"";
  }
}