#pragma once
#include <Windows.h>
#include <string>
#include <memory>
#include "ModuleInfo.h"
#include ".\Interfaces\IModuleAnalyzer.h"

namespace MCP::Native::ProcessExplorer::Interfaces
{
  class IPEImage;
  class IPEImageFactory;
}

namespace MCP::Native::ProcessExplorer
{
  class ModuleAnalyzer : public Interfaces::IModuleAnalyzer
  {
  public:
    /// <summary>Takes ownership of the factory it opens the module images with.</summary>
    explicit ModuleAnalyzer(std::unique_ptr<Interfaces::IPEImageFactory> peImageFactory);
    ~ModuleAnalyzer() override;

    ModuleAnalyzer(const ModuleAnalyzer&) = delete;
    ModuleAnalyzer& operator=(const ModuleAnalyzer&) = delete;
    ModuleAnalyzer(ModuleAnalyzer&&) = delete;
    ModuleAnalyzer& operator=(ModuleAnalyzer&&) = delete;

    ModuleInfoPtr AnalyzeModule(HANDLE hProcess, HANDLE hFile, void* baseAddress) override;

  private:
    static ModuleInfoPtr CreateModuleInfo(const Interfaces::IPEImage& peImage, HANDLE hProcess, HANDLE hFile, void* baseAddress);
    static std::wstring GetPathFromHandle(HANDLE hFile);
    static std::wstring GetPathFromProcess(HANDLE hProcess);
    static std::wstring GetPathFromBaseAddress(HANDLE hProcess, void* baseAddress);

  private:
    std::unique_ptr<Interfaces::IPEImageFactory> _peImageFactory;
  };
}