#include "Native/ProcessExplorer/ProcessDataStore.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace MCP::Native::Tests::ProcessExplorer
{
  using namespace MCP::Native::ProcessExplorer;

  namespace
  {
    constexpr DWORD ProcessId = 1000;
    constexpr DWORD UnknownProcessId = 2000;

    // Handles and addresses are plain numbers here: the store only keeps and compares them.
    HANDLE AsHandle(DWORD_PTR value)
    {
      return reinterpret_cast<HANDLE>(value);
    }

    void* AsAddress(DWORD_PTR value)
    {
      return reinterpret_cast<void*>(value);
    }

    ModuleInfoPtr CreateModule(DWORD_PTR baseAddress, DWORD sizeOfImage, const std::wstring& path)
    {
      return std::make_shared<ModuleInfo>(AsAddress(baseAddress), sizeOfImage, path,
        std::vector<ModuleSection>{}, std::vector<ModuleExport>{}, std::vector<ModuleImport>{});
    }
  }

  TEST(ProcessDataStoreTests, GetProcessHandleReturnsTheHandleOfAnAddedProcess)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));

    // Act
    HANDLE handle = store.GetProcessHandle(ProcessId);

    // Assert
    EXPECT_EQ(AsHandle(0x44), handle);
  }

  TEST(ProcessDataStoreTests, GetProcessHandleReturnsNullForAnUnknownProcess)
  {
    // Arrange
    ProcessDataStore store;

    // Act
    HANDLE handle = store.GetProcessHandle(UnknownProcessId);

    // Assert
    EXPECT_EQ(nullptr, handle);
  }

  TEST(ProcessDataStoreTests, RemoveProcessForgetsHandleAndModules)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    store.AddModule(ProcessId, CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe"));

    // Act
    store.RemoveProcess(ProcessId);

    // Assert
    std::vector<ModuleInfoPtr> modules;
    EXPECT_EQ(nullptr, store.GetProcessHandle(ProcessId));
    EXPECT_FALSE(store.GetModules(ProcessId, modules));
    EXPECT_TRUE(modules.empty());
  }

  TEST(ProcessDataStoreTests, GetModulesReturnsTheModulesOfTheProcessOrderedByBaseAddress)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    auto second = CreateModule(0x700000, 0x2000, L"C:\\Windows\\System32\\kernel32.dll");
    auto first = CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe");
    store.AddModule(ProcessId, second);
    store.AddModule(ProcessId, first);

    // Act
    std::vector<ModuleInfoPtr> modules;
    bool found = store.GetModules(ProcessId, modules);

    // Assert
    EXPECT_TRUE(found);
    ASSERT_EQ(2u, modules.size());
    EXPECT_EQ(first, modules[0]);
    EXPECT_EQ(second, modules[1]);
  }

  TEST(ProcessDataStoreTests, GetModulesFailsForAnUnknownProcess)
  {
    // Arrange
    ProcessDataStore store;
    store.AddModule(UnknownProcessId, CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe"));

    // Act
    std::vector<ModuleInfoPtr> modules;
    bool found = store.GetModules(UnknownProcessId, modules);

    // Assert
    EXPECT_FALSE(found);
    EXPECT_TRUE(modules.empty());
  }

  TEST(ProcessDataStoreTests, RemoveModuleForgetsOnlyThatModule)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    auto kept = CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe");
    store.AddModule(ProcessId, kept);
    store.AddModule(ProcessId, CreateModule(0x700000, 0x2000, L"C:\\Windows\\System32\\kernel32.dll"));

    // Act
    store.RemoveModule(ProcessId, AsAddress(0x700000));

    // Assert
    std::vector<ModuleInfoPtr> modules;
    ASSERT_TRUE(store.GetModules(ProcessId, modules));
    ASSERT_EQ(1u, modules.size());
    EXPECT_EQ(kept, modules[0]);
  }

  TEST(ProcessDataStoreTests, GetModuleFindsAModuleByItsHandle)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    auto module = CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe");
    store.AddModule(ProcessId, module);

    // Act
    ModuleInfoPtr found;
    bool wasFound = store.GetModule(ProcessId, static_cast<HMODULE>(AsAddress(0x400000)), found);

    // Assert
    EXPECT_TRUE(wasFound);
    EXPECT_EQ(module, found);
  }

  TEST(ProcessDataStoreTests, GetModuleFailsForAnAddressThatIsNoModuleBase)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    store.AddModule(ProcessId, CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe"));

    // Act
    ModuleInfoPtr found;
    bool wasFound = store.GetModule(ProcessId, static_cast<HMODULE>(AsAddress(0x400010)), found);

    // Assert
    EXPECT_FALSE(wasFound);
    EXPECT_EQ(nullptr, found);
  }

  TEST(ProcessDataStoreTests, GetModuleByVAFindsTheModuleThatContainsTheAddress)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    auto first = CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe");
    auto second = CreateModule(0x700000, 0x2000, L"C:\\Windows\\System32\\kernel32.dll");
    store.AddModule(ProcessId, first);
    store.AddModule(ProcessId, second);

    // Act
    ModuleInfoPtr atBase;
    ModuleInfoPtr atLastByte;
    ModuleInfoPtr inSecond;
    bool foundAtBase = store.GetModuleByVA(ProcessId, 0x400000, atBase);
    bool foundAtLastByte = store.GetModuleByVA(ProcessId, 0x400FFF, atLastByte);
    bool foundInSecond = store.GetModuleByVA(ProcessId, 0x701234, inSecond);

    // Assert
    EXPECT_TRUE(foundAtBase);
    EXPECT_EQ(first, atBase);
    EXPECT_TRUE(foundAtLastByte);
    EXPECT_EQ(first, atLastByte);
    EXPECT_TRUE(foundInSecond);
    EXPECT_EQ(second, inSecond);
  }

  TEST(ProcessDataStoreTests, GetModuleByVAFailsForAnAddressOutsideEveryModule)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    store.AddModule(ProcessId, CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe"));

    // Act
    ModuleInfoPtr below;
    ModuleInfoPtr behindEnd;
    bool foundBelow = store.GetModuleByVA(ProcessId, 0x3FFFFF, below);
    bool foundBehindEnd = store.GetModuleByVA(ProcessId, 0x401000, behindEnd);

    // Assert
    EXPECT_FALSE(foundBelow);
    EXPECT_FALSE(foundBehindEnd);
  }

  TEST(ProcessDataStoreTests, GetModuleByNameFindsTheModuleByFileNameIgnoringCase)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    auto module = CreateModule(0x700000, 0x2000, L"C:\\Windows\\System32\\kernel32.dll");
    store.AddModule(ProcessId, CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe"));
    store.AddModule(ProcessId, module);

    // Act
    ModuleInfoPtr found;
    bool wasFound = store.GetModuleByName(ProcessId, L"KERNEL32.DLL", found);

    // Assert
    EXPECT_TRUE(wasFound);
    EXPECT_EQ(module, found);
  }

  TEST(ProcessDataStoreTests, GetModuleByNameFailsForAnUnknownName)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    store.AddModule(ProcessId, CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe"));

    // Act
    ModuleInfoPtr found;
    bool wasFound = store.GetModuleByName(ProcessId, L"user32.dll", found);

    // Assert
    EXPECT_FALSE(wasFound);
    EXPECT_EQ(nullptr, found);
  }

  TEST(ProcessDataStoreTests, LookupsFailForAnUnknownProcess)
  {
    // Arrange
    ProcessDataStore store;
    store.AddProcess(ProcessId, AsHandle(0x44));
    store.AddModule(ProcessId, CreateModule(0x400000, 0x1000, L"C:\\Target\\Example.exe"));

    // Act
    ModuleInfoPtr byHandle;
    ModuleInfoPtr byAddress;
    ModuleInfoPtr byName;
    bool foundByHandle = store.GetModule(UnknownProcessId, static_cast<HMODULE>(AsAddress(0x400000)), byHandle);
    bool foundByAddress = store.GetModuleByVA(UnknownProcessId, 0x400000, byAddress);
    bool foundByName = store.GetModuleByName(UnknownProcessId, L"Example.exe", byName);

    // Assert
    EXPECT_FALSE(foundByHandle);
    EXPECT_FALSE(foundByAddress);
    EXPECT_FALSE(foundByName);
  }
}
