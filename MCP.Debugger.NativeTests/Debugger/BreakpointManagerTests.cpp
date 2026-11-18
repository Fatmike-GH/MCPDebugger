#include "Mocks/Debugger/Interfaces/MockBreakpoint.h"
#include "Mocks/Debugger/Interfaces/MockBreakpointFactory.h"
#include "Native/Debugger/BreakpointManager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <utility>

namespace MCP::Native::Tests::Debugger
{
  using MCP::Native::Debugger::BreakpointManager;
  using MCP::Native::Debugger::Interfaces::IBreakpoint;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockBreakpoint;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockBreakpointFactory;
  using ::testing::ByMove;
  using ::testing::NiceMock;
  using ::testing::Return;
  using ::testing::StrictMock;

  /// <summary>
  /// Tests the bookkeeping of the breakpoint manager - which address has a breakpoint, when one is created,
  /// enabled and disabled - against doubles of the factory and of the breakpoints, so no test touches a
  /// process.
  /// </summary>
  class BreakpointManagerTests : public ::testing::Test
  {
  protected:
    BreakpointManagerTests()
    {
      auto breakpointFactory = std::make_unique<StrictMock<MockBreakpointFactory>>();
      _breakpointFactory = breakpointFactory.get();
      // No test reaches a member that uses the process handle.
      _breakpointManager = std::make_unique<BreakpointManager>(nullptr, std::move(breakpointFactory));
    }

    /// <summary>
    /// Lets the factory hand out a breakpoint double for the address, exactly once. The manager will own the
    /// double; the returned pointer is for expectations and comparisons and stays valid as long as the
    /// manager.
    /// </summary>
    NiceMock<MockBreakpoint>* ExpectBreakpointCreatedFor(DWORD_PTR address)
    {
      auto breakpoint = std::make_unique<NiceMock<MockBreakpoint>>();
      NiceMock<MockBreakpoint>* breakpointView = breakpoint.get();
      std::unique_ptr<IBreakpoint> created = std::move(breakpoint);
      EXPECT_CALL(*_breakpointFactory, Create(address)).WillOnce(Return(ByMove(std::move(created))));
      return breakpointView;
    }

    StrictMock<MockBreakpointFactory>* _breakpointFactory = nullptr; // owned by the manager
    std::unique_ptr<BreakpointManager> _breakpointManager;
  };

  TEST_F(BreakpointManagerTests, SetBreakpointCreatesABreakpointForTheAddressAndEnablesIt)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(address);
    EXPECT_CALL(*breakpoint, Enable()).Times(1);

    // Act
    bool result = _breakpointManager->SetBreakpoint(address);

    // Assert
    EXPECT_TRUE(result);
  }

  TEST_F(BreakpointManagerTests, SetBreakpointOnAnAddressThatHasOneEnablesItAgainWithoutCreatingAnother)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    // The strict factory fails the test when it is asked a second time.
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(address);
    EXPECT_CALL(*breakpoint, Enable()).Times(2);
    _breakpointManager->SetBreakpoint(address);

    // Act
    bool result = _breakpointManager->SetBreakpoint(address);

    // Assert
    EXPECT_TRUE(result);
  }

  TEST_F(BreakpointManagerTests, SetBreakpointKeepsOneBreakpointPerAddress)
  {
    // Arrange
    NiceMock<MockBreakpoint>* first = ExpectBreakpointCreatedFor(0x401000);
    NiceMock<MockBreakpoint>* second = ExpectBreakpointCreatedFor(0x402000);
    _breakpointManager->SetBreakpoint(0x401000);
    _breakpointManager->SetBreakpoint(0x402000);

    // Act
    IBreakpoint* foundFirst = _breakpointManager->GetBreakpoint(0x401000);
    IBreakpoint* foundSecond = _breakpointManager->GetBreakpoint(0x402000);

    // Assert
    EXPECT_EQ(first, foundFirst);
    EXPECT_EQ(second, foundSecond);
  }

  TEST_F(BreakpointManagerTests, GetBreakpointReturnsNullForAnAddressWithoutBreakpoint)
  {
    // Arrange
    ExpectBreakpointCreatedFor(0x401000);
    _breakpointManager->SetBreakpoint(0x401000);

    // Act
    IBreakpoint* found = _breakpointManager->GetBreakpoint(0x402000);

    // Assert
    EXPECT_EQ(nullptr, found);
  }

  TEST_F(BreakpointManagerTests, HasBreakpointIsTrueForAnAddressThatWasSet)
  {
    // Arrange
    ExpectBreakpointCreatedFor(0x401000);
    _breakpointManager->SetBreakpoint(0x401000);

    // Act
    bool result = _breakpointManager->HasBreakpoint(0x401000);

    // Assert
    EXPECT_TRUE(result);
  }

  TEST_F(BreakpointManagerTests, HasBreakpointIsFalseForAnAddressWithoutBreakpoint)
  {
    // Arrange
    ExpectBreakpointCreatedFor(0x401000);
    _breakpointManager->SetBreakpoint(0x401000);

    // Act
    bool result = _breakpointManager->HasBreakpoint(0x402000);

    // Assert
    EXPECT_FALSE(result);
  }

  TEST_F(BreakpointManagerTests, RemoveBreakpointDisablesTheBreakpointOfTheAddress)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(address);
    NiceMock<MockBreakpoint>* other = ExpectBreakpointCreatedFor(0x402000);
    EXPECT_CALL(*breakpoint, Disable()).Times(1);
    EXPECT_CALL(*other, Disable()).Times(0);
    _breakpointManager->SetBreakpoint(address);
    _breakpointManager->SetBreakpoint(0x402000);

    // Act
    bool result = _breakpointManager->RemoveBreakpoint(address);

    // Assert
    EXPECT_TRUE(result);
  }

  TEST_F(BreakpointManagerTests, RemoveBreakpointFailsForAnAddressWithoutBreakpoint)
  {
    // Arrange
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(0x401000);
    EXPECT_CALL(*breakpoint, Disable()).Times(0);
    _breakpointManager->SetBreakpoint(0x401000);

    // Act
    bool result = _breakpointManager->RemoveBreakpoint(0x402000);

    // Assert
    EXPECT_FALSE(result);
  }
}
