#include "SymbolResolver.h"
#include <algorithm>

namespace MCP::Native::Disassembler
{
  template <typename T>
  bool IsPrintable(T c)
  {
    if (c == '\t' || c == '\n' || c == '\r') return true;
    return (c >= 0x20 && c <= 0x7E);
  }

  template <typename T>
  std::basic_string<T> ExtractString(const std::vector<BYTE>& buffer, size_t validBytes)
  {
    const size_t kMinLen = 4;
    const T* ptr = reinterpret_cast<const T*>(buffer.data());
    size_t capacity = validBytes / sizeof(T);

    std::basic_string<T> result;
    result.reserve(64);

    for (size_t i = 0; i < capacity; i++)
    {
      T c = ptr[i];
      if (c == 0) return (result.length() >= kMinLen) ? result : std::basic_string<T>();
      if (!IsPrintable(c)) return {};

      result += c;
      if (result.length() >= 64) break;
    }
    return {};
  }

  SymbolResolver::SymbolResolver(MCP::Native::ProcessExplorer::ProcessExplorer* explorer)
    : _processExplorer(explorer)
  {
  }

  std::string SymbolResolver::Resolve(HANDLE hProcess, DWORD processId, DWORD_PTR address)
  {
    if (!_processExplorer) return "";

    // Export
    std::string result = TryResolveExport(processId, address);
    if (!result.empty()) return "<" + result + ">";

    // Indirect (IAT)
    result = TryResolveIndirect(hProcess, processId, address);
    if (!result.empty()) return "<IAT:" + result + ">";

    // String
    result = TryReadString(hProcess, address);
    if (!result.empty()) return "<" + result + ">";

    return "";
  }

  std::string SymbolResolver::ResolveString(HANDLE hProcess, DWORD_PTR address)
  {
    return TryReadString(hProcess, address);
  }

  std::string SymbolResolver::TryResolveExport(DWORD processId, DWORD_PTR address)
  {
    MCP::Native::ProcessExplorer::ModuleExport exportInfo;
    if (_processExplorer->GetFunctionByVA(processId, address, exportInfo))
    {
      return exportInfo.Name;
    }
    return "";
  }

  std::string SymbolResolver::TryResolveIndirect(HANDLE hProcess, DWORD processId, DWORD_PTR address)
  {
    DWORD_PTR ptrValue = 0;
    SIZE_T bytesRead = 0;
    if (ReadProcessMemory(hProcess, (LPCVOID)address, &ptrValue, sizeof(DWORD_PTR), &bytesRead))
    {
      return TryResolveExport(processId, ptrValue);
    }
    return "";
  }

  std::string SymbolResolver::TryReadString(HANDLE hProcess, DWORD_PTR address)
  {
    const size_t kReadSize = 128;
    std::vector<BYTE> buffer(kReadSize);
    SIZE_T bytesRead = 0;

    if (!ReadProcessMemory(hProcess, (LPCVOID)address, buffer.data(), kReadSize, &bytesRead) || bytesRead < 4)
      return "";

    auto wRes = ExtractString<WCHAR>(buffer, bytesRead);
    if (!wRes.empty()) return WStringToString(wRes);

    auto aRes = ExtractString<char>(buffer, bytesRead);
    if (!aRes.empty()) return aRes;

    return "";
  }

  std::string SymbolResolver::WStringToString(const std::wstring& wstr)
  {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
  }
}