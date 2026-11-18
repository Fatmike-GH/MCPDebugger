#include "Mocks/ProcessExplorer/Interfaces/MockProcessExplorer.h"
#include "Native/Disassembler/SymbolResolver.h"
#include "Native/ProcessExplorer/ModuleExport.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace MCP::Native::Tests::Disassembler
{
  using MCP::Native::Disassembler::SymbolResolver;
  using MCP::Native::ProcessExplorer::ModuleExport;
  using MCP::Native::Tests::Mocks::ProcessExplorer::Interfaces::MockProcessExplorer;
  using ::testing::_;
  using ::testing::DoAll;
  using ::testing::Return;
  using ::testing::SetArgReferee;
  using ::testing::StrictMock;

  /// <summary>
  /// Tests how the resolver names an address from the module bookkeeping, with a double of the explorer. The
  /// double is strict, so a test fails when the resolver asks the explorer more often than expected.
  ///
  /// No test has a debuggee: the process handle is always null. Reading the memory of a process through a null
  /// handle fails inside the system call, so the paths of the resolver that look into debuggee memory find
  /// nothing here. A made-up handle value must not be used instead: -1 is the pseudo handle of the test process
  /// itself, and any other number could be a real handle of it.
  /// </summary>
  class SymbolResolverTests : public ::testing::Test
  {
  protected:
    static constexpr DWORD ProcessId = 1000;
    static constexpr DWORD_PTR Address = 0x401000;
    static constexpr HANDLE NoProcess = nullptr;

    StrictMock<MockProcessExplorer> _processExplorer;
    SymbolResolver _resolver{ _processExplorer };
  };

  TEST_F(SymbolResolverTests, ResolveNamesTheExportedFunctionAnAddressBelongsTo)
  {
    // Arrange
    ModuleExport exportedFunction;
    exportedFunction.Name = "CreateFileW";
    EXPECT_CALL(_processExplorer, GetFunctionByVA(ProcessId, Address, _))
      .WillOnce(DoAll(SetArgReferee<2>(exportedFunction), Return(true)));

    // Act
    std::string text = _resolver.Resolve(NoProcess, ProcessId, Address);

    // Assert
    EXPECT_EQ("<CreateFileW>", text);
  }

  TEST_F(SymbolResolverTests, ResolveGivesAnEmptyTextForAnAddressNoModuleExports)
  {
    // Arrange
    EXPECT_CALL(_processExplorer, GetFunctionByVA(ProcessId, Address, _)).WillOnce(Return(false));

    // Act
    std::string text = _resolver.Resolve(NoProcess, ProcessId, Address);

    // Assert
    EXPECT_EQ("", text);
  }
}
