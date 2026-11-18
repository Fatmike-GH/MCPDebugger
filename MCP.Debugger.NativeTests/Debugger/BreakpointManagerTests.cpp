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
  using ::testing::Sequence;
  using ::testing::StrictMock;

  /// <summary>
  /// Tests the bookkeeping of the breakpoint manager - which address has a breakpoint, when one is created,
  /// enabled, disabled and forgotten, and what holding an address changes - against doubles of the factory
  /// and of the breakpoints, so no test touches a process.
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
    /// Lets the factory hand out a breakpoint double for the address, exactly once; the factory must be asked
    /// in the order of these calls. The manager will own the double: the returned pointer is for expectations
    /// and is valid only until the manager removes the breakpoint, which destroys the double and verifies
    /// its expectations.
    /// </summary>
    NiceMock<MockBreakpoint>* ExpectBreakpointCreatedFor(DWORD_PTR address)
    {
      auto breakpoint = std::make_unique<NiceMock<MockBreakpoint>>();
      NiceMock<MockBreakpoint>* breakpointView = breakpoint.get();
      std::unique_ptr<IBreakpoint> created = std::move(breakpoint);
      EXPECT_CALL(*_breakpointFactory, Create(address)).InSequence(_creations).WillOnce(Return(ByMove(std::move(created))));
      return breakpointView;
    }

    StrictMock<MockBreakpointFactory>* _breakpointFactory = nullptr; // owned by the manager
    Sequence _creations; // orders the creations, so one address can be given a second double after a removal
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
    EXPECT_CALL(*first, Enable()).Times(1);
    EXPECT_CALL(*second, Enable()).Times(1);

    // Act
    _breakpointManager->SetBreakpoint(0x401000);
    _breakpointManager->SetBreakpoint(0x402000);

    // Assert
    EXPECT_TRUE(_breakpointManager->HasBreakpoint(0x401000));
    EXPECT_TRUE(_breakpointManager->HasBreakpoint(0x402000));
  }

  TEST_F(BreakpointManagerTests, SetBreakpointAtAnAddressThatIsNotHeldEnablesItWhileAnotherAddressIsHeld)
  {
    // Arrange
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(0x402000);
    EXPECT_CALL(*breakpoint, Enable()).Times(1);
    _breakpointManager->HoldBreakpointAt(0x401000);

    // Act
    bool result = _breakpointManager->SetBreakpoint(0x402000);

    // Assert
    EXPECT_TRUE(result);
  }

  TEST_F(BreakpointManagerTests, SetBreakpointAtTheHeldAddressCreatesTheBreakpointWithoutEnablingIt)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(address);
    EXPECT_CALL(*breakpoint, Enable()).Times(0);
    _breakpointManager->HoldBreakpointAt(address);

    // Act
    bool result = _breakpointManager->SetBreakpoint(address);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_TRUE(_breakpointManager->HasBreakpoint(address));
  }

  TEST_F(BreakpointManagerTests, SetBreakpointAfterARemovalCreatesTheBreakpointAnew)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    // The strict factory fails the test unless it is asked exactly twice.
    NiceMock<MockBreakpoint>* removed = ExpectBreakpointCreatedFor(address);
    NiceMock<MockBreakpoint>* created = ExpectBreakpointCreatedFor(address);
    EXPECT_CALL(*removed, Enable()).Times(1);
    EXPECT_CALL(*created, Enable()).Times(1);
    _breakpointManager->SetBreakpoint(address);
    _breakpointManager->RemoveBreakpoint(address);

    // Act
    bool result = _breakpointManager->SetBreakpoint(address);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_TRUE(_breakpointManager->HasBreakpoint(address));
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

  TEST_F(BreakpointManagerTests, HasBreakpointStaysTrueWhileTheAddressIsHeld)
  {
    // Arrange
    ExpectBreakpointCreatedFor(0x401000);
    _breakpointManager->SetBreakpoint(0x401000);
    _breakpointManager->HoldBreakpointAt(0x401000);

    // Act
    bool result = _breakpointManager->HasBreakpoint(0x401000);

    // Assert
    EXPECT_TRUE(result);
  }

  TEST_F(BreakpointManagerTests, RemoveBreakpointDisablesTheBreakpointOfTheAddressAndForgetsIt)
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
    EXPECT_FALSE(_breakpointManager->HasBreakpoint(address));
    EXPECT_TRUE(_breakpointManager->HasBreakpoint(0x402000));
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

  TEST_F(BreakpointManagerTests, RemoveBreakpointFailsForAnAddressWhoseBreakpointWasRemoved)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    ExpectBreakpointCreatedFor(address);
    _breakpointManager->SetBreakpoint(address);
    _breakpointManager->RemoveBreakpoint(address);

    // Act
    bool result = _breakpointManager->RemoveBreakpoint(address);

    // Assert
    EXPECT_FALSE(result);
  }

  TEST_F(BreakpointManagerTests, RemoveBreakpointKeepsTheHoldOnItsAddress)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    ExpectBreakpointCreatedFor(address);
    NiceMock<MockBreakpoint>* setAgain = ExpectBreakpointCreatedFor(address);
    // Enabling it would show that the removal ended the hold.
    EXPECT_CALL(*setAgain, Enable()).Times(0);
    _breakpointManager->SetBreakpoint(address);
    _breakpointManager->HoldBreakpointAt(address);

    // Act
    _breakpointManager->RemoveBreakpoint(address);
    _breakpointManager->SetBreakpoint(address);

    // Assert
    EXPECT_TRUE(_breakpointManager->HasBreakpoint(address));
  }

  TEST_F(BreakpointManagerTests, HoldBreakpointAtDisablesTheBreakpointOfTheAddress)
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
    _breakpointManager->HoldBreakpointAt(address);

    // Assert: the expectations above
  }

  TEST_F(BreakpointManagerTests, HoldBreakpointAtTheHeldAddressKeepsItsBreakpointDisabled)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(address);
    // A second enable would show that holding the address again released it in between.
    EXPECT_CALL(*breakpoint, Enable()).Times(1);
    _breakpointManager->SetBreakpoint(address);
    _breakpointManager->HoldBreakpointAt(address);

    // Act
    _breakpointManager->HoldBreakpointAt(address);

    // Assert: the expectations above
  }

  TEST_F(BreakpointManagerTests, HoldBreakpointAtAnotherAddressEnablesTheBreakpointOfTheAddressHeldBefore)
  {
    // Arrange
    NiceMock<MockBreakpoint>* heldBefore = ExpectBreakpointCreatedFor(0x401000);
    NiceMock<MockBreakpoint>* heldNow = ExpectBreakpointCreatedFor(0x402000);
    // Once by the set, once by the release that the second hold implies.
    EXPECT_CALL(*heldBefore, Enable()).Times(2);
    EXPECT_CALL(*heldNow, Disable()).Times(1);
    _breakpointManager->SetBreakpoint(0x401000);
    _breakpointManager->SetBreakpoint(0x402000);
    _breakpointManager->HoldBreakpointAt(0x401000);

    // Act
    _breakpointManager->HoldBreakpointAt(0x402000);

    // Assert: the expectations above
  }

  TEST_F(BreakpointManagerTests, ReleaseHeldBreakpointEnablesTheBreakpointThatWasSetAtTheHeldAddress)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(address);
    // The set at the held address does not enable it, so the one enable is the release.
    EXPECT_CALL(*breakpoint, Enable()).Times(1);
    _breakpointManager->HoldBreakpointAt(address);
    _breakpointManager->SetBreakpoint(address);

    // Act
    _breakpointManager->ReleaseHeldBreakpoint();

    // Assert: the expectations above
  }

  TEST_F(BreakpointManagerTests, ReleaseHeldBreakpointEnablesTheBreakpointThatTheHoldDisabled)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(address);
    // Once by the set, once by the release.
    EXPECT_CALL(*breakpoint, Enable()).Times(2);
    _breakpointManager->SetBreakpoint(address);
    _breakpointManager->HoldBreakpointAt(address);

    // Act
    _breakpointManager->ReleaseHeldBreakpoint();

    // Assert: the expectations above
  }

  TEST_F(BreakpointManagerTests, ReleaseHeldBreakpointEndsTheHold)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(address);
    // Enabled at once by the set: the address is no longer held.
    EXPECT_CALL(*breakpoint, Enable()).Times(1);
    _breakpointManager->HoldBreakpointAt(address);
    _breakpointManager->ReleaseHeldBreakpoint();

    // Act
    _breakpointManager->SetBreakpoint(address);

    // Assert: the expectations above
  }

  TEST_F(BreakpointManagerTests, ReleaseHeldBreakpointCreatesNothingWhenTheBreakpointOfTheHeldAddressWasRemoved)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    // The strict factory fails the test when the release asks it for a breakpoint.
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(address);
    // Only by the set; the removal destroys the double and verifies this.
    EXPECT_CALL(*breakpoint, Enable()).Times(1);
    _breakpointManager->SetBreakpoint(address);
    _breakpointManager->HoldBreakpointAt(address);
    _breakpointManager->RemoveBreakpoint(address);

    // Act
    _breakpointManager->ReleaseHeldBreakpoint();

    // Assert
    EXPECT_FALSE(_breakpointManager->HasBreakpoint(address));
  }

  TEST_F(BreakpointManagerTests, ReleaseHeldBreakpointDoesNothingWhenNoAddressIsHeld)
  {
    // Arrange
    NiceMock<MockBreakpoint>* breakpoint = ExpectBreakpointCreatedFor(0x401000);
    EXPECT_CALL(*breakpoint, Enable()).Times(1);
    EXPECT_CALL(*breakpoint, Disable()).Times(0);
    _breakpointManager->SetBreakpoint(0x401000);

    // Act
    _breakpointManager->ReleaseHeldBreakpoint();

    // Assert: the expectations above
  }
}
