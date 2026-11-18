#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Native/Debugger/Commands/CommandResult.h"
#include "Native/Debugger/Commands/StepIntoCommand.h"
#include "TestHelpers/Futures.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Commands::Base::MockCommandContext;
  using MCP::Native::Tests::TestHelpers::IsCompleted;
  using ::testing::Return;
  using ::testing::StrictMock;

  TEST(StepIntoCommandTests, ExecuteAtAStopCompletesWithASuccessfulResult)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, IsDebuggeeStopped()).WillOnce(Return(true));
    StepIntoCommand command;
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(result->IsSuccessful());
  }

  TEST(StepIntoCommandTests, ExecuteWhileTheDebuggeeRunsCompletesWithAFailedResult)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, IsDebuggeeStopped()).WillOnce(Return(false));
    StepIntoCommand command;
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    ASSERT_NE(nullptr, result);
    EXPECT_FALSE(result->IsSuccessful());
  }

  TEST(StepIntoCommandTests, GetTypeReturnsStepInto)
  {
    // Arrange
    StepIntoCommand command;

    // Act
    Base::CommandType type = command.GetType();

    // Assert
    EXPECT_EQ(Base::CommandType::CMD_STEP_INTO, type);
  }
}
