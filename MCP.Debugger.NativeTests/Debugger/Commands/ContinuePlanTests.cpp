#include "Mocks/Debugger/Interfaces/MockStopSite.h"
#include "Native/Debugger/Commands/ContinuePlan.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockStopSite;
  using ::testing::StrictMock;

  // The strict stop site fails a test as soon as the plan asks it anything or places anything.

  TEST(ContinuePlanTests, BeginDoesNothingAtTheStop)
  {
    // Arrange
    StrictMock<MockStopSite> stopSite;
    ContinuePlan plan;

    // Act
    plan.Begin(stopSite);
  }

  TEST(ContinuePlanTests, TheDebuggeeDoesNotHaltAfterTheFirstInstruction)
  {
    // Arrange
    StrictMock<MockStopSite> stopSite;
    ContinuePlan plan;
    plan.Begin(stopSite);

    // Act
    bool halts = plan.HaltsAfterFirstInstruction();

    // Assert
    EXPECT_FALSE(halts);
  }

  TEST(ContinuePlanTests, ThePlanOwnsNoBreakpoint)
  {
    // Arrange
    StrictMock<MockStopSite> stopSite;
    ContinuePlan plan;
    plan.Begin(stopSite);

    // Act
    bool owns = plan.OwnsBreakpointAt(0x401015);

    // Assert
    EXPECT_FALSE(owns);
  }

  TEST(ContinuePlanTests, EndRemovesNothing)
  {
    // Arrange
    StrictMock<MockStopSite> stopSite;
    ContinuePlan plan;
    plan.Begin(stopSite);

    // Act
    plan.End(stopSite);
  }
}
