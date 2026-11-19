#include "Mocks/Debugger/Interfaces/MockBreakpointManager.h"
#include "Mocks/Disassembler/Interfaces/MockDisassembler.h"
#include "Native/Debugger/StopSite.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>

namespace MCP::Native::Tests::Debugger
{
  using MCP::Native::Debugger::StopSite;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockBreakpointManager;
  using MCP::Native::Tests::Mocks::Disassembler::Interfaces::MockDisassembler;
  using ::testing::_;
  using ::testing::DoAll;
  using ::testing::Return;
  using ::testing::SetArgReferee;
  using ::testing::StrictMock;

  /// <summary>
  /// Tests the stop site against strict doubles of the breakpoint manager and the disassembler: every call
  /// the test does not expect, and every call with another process or address, fails the test.
  /// </summary>
  class StopSiteTests : public ::testing::Test
  {
  protected:
    static constexpr DWORD_PTR StopAddress = 0x401010;

    /// <summary>A value that stands for a process handle. It is only passed on and compared, never used.</summary>
    static HANDLE FakeProcessHandle()
    {
      // The cast builds a handle value from a number; no test hands it to the system.
      return reinterpret_cast<HANDLE>(static_cast<DWORD_PTR>(0x1234));
    }

    StrictMock<MockBreakpointManager> _breakpointManager;
    StrictMock<MockDisassembler> _disassembler;
    StopSite _stopSite{ FakeProcessHandle(), StopAddress, _breakpointManager, _disassembler };
  };

  TEST_F(StopSiteTests, GetStopAddressReturnsTheAddressTheSiteWasCreatedFor)
  {
    // Act
    DWORD_PTR result = _stopSite.GetStopAddress();

    // Assert
    EXPECT_EQ(StopAddress, result);
  }

  TEST_F(StopSiteTests, TryGetCallLengthReturnsTheLengthTheDisassemblerReportsForACallAtTheStopAddress)
  {
    // Arrange
    EXPECT_CALL(_disassembler, IsCallInstruction(FakeProcessHandle(), StopAddress, _))
      .WillOnce(DoAll(SetArgReferee<2>(size_t{ 5 }), Return(true)));

    // Act
    std::optional<size_t> result = _stopSite.TryGetCallLength();

    // Assert
    EXPECT_EQ(std::optional<size_t>(5), result);
  }

  TEST_F(StopSiteTests, TryGetCallLengthIsEmptyWhenTheDisassemblerReportsNoCall)
  {
    // Arrange
    // The length the disassembler leaves behind must not leak out: only its answer counts.
    EXPECT_CALL(_disassembler, IsCallInstruction(FakeProcessHandle(), StopAddress, _))
      .WillOnce(DoAll(SetArgReferee<2>(size_t{ 2 }), Return(false)));

    // Act
    std::optional<size_t> result = _stopSite.TryGetCallLength();

    // Assert
    EXPECT_EQ(std::nullopt, result);
  }

  TEST_F(StopSiteTests, SetBreakpointSetsTheBreakpointInTheBreakpointManager)
  {
    // Arrange
    EXPECT_CALL(_breakpointManager, SetBreakpoint(0x401015)).WillOnce(Return(true));

    // Act
    _stopSite.SetBreakpoint(0x401015);
  }

  TEST_F(StopSiteTests, RemoveBreakpointRemovesTheBreakpointFromTheBreakpointManager)
  {
    // Arrange
    EXPECT_CALL(_breakpointManager, RemoveBreakpoint(0x401015)).WillOnce(Return(true));

    // Act
    _stopSite.RemoveBreakpoint(0x401015);
  }

  TEST_F(StopSiteTests, RemoveBreakpointToleratesAnAddressWithoutBreakpoint)
  {
    // Arrange
    EXPECT_CALL(_breakpointManager, RemoveBreakpoint(0x401015)).WillOnce(Return(false));

    // Act
    _stopSite.RemoveBreakpoint(0x401015);
  }
}
