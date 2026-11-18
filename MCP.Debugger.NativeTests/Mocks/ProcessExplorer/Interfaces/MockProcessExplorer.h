#pragma once
#include "Native/ProcessExplorer/Interfaces/IProcessExplorer.h"
#include <gmock/gmock.h>

namespace MCP::Native::Tests::Mocks::ProcessExplorer::Interfaces
{
  /// <summary>
  /// gMock double of the module queries of a session.
  /// </summary>
  class MockProcessExplorer : public MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer
  {
  public:
    MOCK_METHOD(bool, GetModules, (DWORD processId, std::vector<MCP::Native::ProcessExplorer::ModuleInfoPtr>& modules), (override));
    MOCK_METHOD(bool, GetModule, (DWORD processId, HMODULE moduleHandle, MCP::Native::ProcessExplorer::ModuleInfoPtr& outModuleInfo), (override));
    MOCK_METHOD(bool, GetModuleByVA, (DWORD processId, DWORD_PTR va, MCP::Native::ProcessExplorer::ModuleInfoPtr& outModuleInfo), (override));
    MOCK_METHOD(bool, GetFunctionByVA, (DWORD processId, DWORD_PTR va, MCP::Native::ProcessExplorer::ModuleExport& outExport), (override));
    MOCK_METHOD(HMODULE, GetRemoteModuleHandle, (DWORD processId, const wchar_t* moduleName), (override));
    MOCK_METHOD(FARPROC, GetRemoteProcAddress, (DWORD processId, HMODULE hModule, LPCSTR lpProcName), (override));
  };
}
