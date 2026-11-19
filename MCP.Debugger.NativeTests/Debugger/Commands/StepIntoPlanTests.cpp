#include "Mocks/Debugger/Interfaces/MockStopSite.h"
#include "Native/Debugger/Commands/StepIntoPlan.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockStopSite;
  using ::testing::StrictMock;

  // The strict stop site fails a test as soon as the plan asks it anything or places anything.

  TEST(StepIntoPlanTests, BeginDoesNothingAtTheStop)
  {
    // Arrange
    StrictMock<MockStopSite> stopSite;
    StepIntoPlan plan;

    // Act
    plan.Begin(stopSite);
  }

  TEST(StepIntoPlanTests, TheDebuggeeHaltsAfterTheFirstInstruction)
  {
    // Arrange
    StrictMock<MockStopSite> stopSite;
    StepIntoPlan plan;
    plan.Begin(stopSite);

    // Act
    bool halts = plan.HaltsAfterFirstInstruction();

    // Assert
    EXPECT_TRUE(halts);
  }

  TEST(StepIntoPlanTests, ThePlanOwnsNoBreakpoint)
  {
    // Arrange
    StrictMock<MockStopSite> stopSite;
    StepIntoPlan plan;
    plan.Begin(stopSite);

    // Act
    bool owns = plan.OwnsBreakpointAt(0x401015);

    // Assert
    EXPECT_FALSE(owns);
  }

  TEST(StepIntoPlanTests, EndRemovesNothing)
  {
    // Arrange
    StrictMock<MockStopSite> stopSite;
    StepIntoPlan plan;
    plan.Begin(stopSite);

    // Act
    plan.End(stopSite);
  }
}
