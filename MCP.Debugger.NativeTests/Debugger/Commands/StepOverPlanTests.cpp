#include "Mocks/Debugger/Interfaces/MockStopSite.h"
#include "Native/Debugger/Commands/StepOverPlan.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockStopSite;
  using ::testing::Return;
  using ::testing::StrictMock;

  /// <summary>
  /// Tests the plan of a step over at a stop on a call and at a stop on any other instruction. The stop site
  /// is strict: a breakpoint the plan sets or removes without the test expecting it fails the test.
  /// </summary>
  class StepOverPlanTests : public ::testing::Test
  {
  protected:
    static constexpr DWORD_PTR StopAddress = 0x401010;
    static constexpr size_t CallLength = 5;
    static constexpr DWORD_PTR ReturnAddress = StopAddress + CallLength;

    /// <summary>Begins the plan at a stop on a call; the breakpoint behind the call is expected once.</summary>
    void BeginAtACall()
    {
      EXPECT_CALL(_stopSite, GetStopAddress()).WillRepeatedly(Return(StopAddress));
      EXPECT_CALL(_stopSite, TryGetCallLength()).WillOnce(Return(std::optional<size_t>(CallLength)));
      EXPECT_CALL(_stopSite, SetBreakpoint(ReturnAddress)).Times(1);
      _plan.Begin(_stopSite);
    }

    /// <summary>
    /// Begins the plan at a stop where the stop site reports no call: another instruction stands there, or
    /// the instruction could not be read or decoded.
    /// </summary>
    void BeginWhereNoCallIsFound()
    {
      EXPECT_CALL(_stopSite, TryGetCallLength()).WillOnce(Return(std::nullopt));
      _plan.Begin(_stopSite);
    }

    StrictMock<MockStopSite> _stopSite;
    StepOverPlan _plan;
  };

  TEST_F(StepOverPlanTests, BeginAtACallSetsABreakpointBehindTheCall)
  {
    // Act: the expectations of the helper are the assertion
    BeginAtACall();
  }

  TEST_F(StepOverPlanTests, AfterBeginAtACallTheDebuggeeRunsOnToTheBreakpointBehindTheCall)
  {
    // Arrange
    BeginAtACall();

    // Act
    bool halts = _plan.HaltsAfterFirstInstruction();

    // Assert
    EXPECT_FALSE(halts);
  }

  TEST_F(StepOverPlanTests, AfterBeginAtACallThePlanOwnsTheBreakpointBehindTheCallAndNoOther)
  {
    // Arrange
    BeginAtACall();

    // Act
    bool ownsReturnAddress = _plan.OwnsBreakpointAt(ReturnAddress);
    bool ownsStopAddress = _plan.OwnsBreakpointAt(StopAddress);

    // Assert
    EXPECT_TRUE(ownsReturnAddress);
    EXPECT_FALSE(ownsStopAddress);
  }

  TEST_F(StepOverPlanTests, EndAfterBeginAtACallRemovesTheBreakpointBehindTheCallAndOwnsItNoLonger)
  {
    // Arrange
    BeginAtACall();
    EXPECT_CALL(_stopSite, RemoveBreakpoint(ReturnAddress)).Times(1);

    // Act
    _plan.End(_stopSite);

    // Assert
    EXPECT_FALSE(_plan.OwnsBreakpointAt(ReturnAddress));
  }

  TEST_F(StepOverPlanTests, ASecondEndRemovesNothing)
  {
    // Arrange
    BeginAtACall();
    EXPECT_CALL(_stopSite, RemoveBreakpoint(ReturnAddress)).Times(1);
    _plan.End(_stopSite);

    // Act: the strict stop site fails the test when the breakpoint is removed a second time
    _plan.End(_stopSite);
  }

  TEST_F(StepOverPlanTests, BeginWhereNoCallIsFoundSetsNoBreakpoint)
  {
    // Act: the strict stop site fails the test when a breakpoint is set
    BeginWhereNoCallIsFound();
  }

  TEST_F(StepOverPlanTests, AfterBeginWhereNoCallIsFoundTheDebuggeeHaltsAfterTheFirstInstruction)
  {
    // Arrange
    BeginWhereNoCallIsFound();

    // Act
    bool halts = _plan.HaltsAfterFirstInstruction();

    // Assert
    EXPECT_TRUE(halts);
  }

  TEST_F(StepOverPlanTests, AfterBeginWhereNoCallIsFoundThePlanOwnsNoBreakpoint)
  {
    // Arrange
    BeginWhereNoCallIsFound();

    // Act
    bool owns = _plan.OwnsBreakpointAt(ReturnAddress);

    // Assert
    EXPECT_FALSE(owns);
  }

  TEST_F(StepOverPlanTests, EndAfterBeginWhereNoCallIsFoundRemovesNothing)
  {
    // Arrange
    BeginWhereNoCallIsFound();

    // Act: the strict stop site fails the test when a breakpoint is removed
    _plan.End(_stopSite);
  }
}
