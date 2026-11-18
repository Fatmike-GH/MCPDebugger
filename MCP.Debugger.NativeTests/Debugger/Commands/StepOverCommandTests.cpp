#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Native/Debugger/Commands/CommandResult.h"
#include "Native/Debugger/Commands/StepOverCommand.h"
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

  TEST(StepOverCommandTests, ExecuteAtAStopCompletesWithASuccessfulResult)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, IsDebuggeeStopped()).WillOnce(Return(true));
    StepOverCommand command;
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(result->IsSuccessful());
  }

  TEST(StepOverCommandTests, ExecuteWhileTheDebuggeeRunsCompletesWithAFailedResult)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, IsDebuggeeStopped()).WillOnce(Return(false));
    StepOverCommand command;
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    ASSERT_NE(nullptr, result);
    EXPECT_FALSE(result->IsSuccessful());
  }

  TEST(StepOverCommandTests, GetTypeReturnsStepOver)
  {
    // Arrange
    StepOverCommand command;

    // Act
    Base::CommandType type = command.GetType();

    // Assert
    EXPECT_EQ(Base::CommandType::CMD_STEP_OVER, type);
  }
}
