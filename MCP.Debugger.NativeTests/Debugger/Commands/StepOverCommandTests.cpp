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
  using ::testing::StrictMock;

  TEST(StepOverCommandTests, ExecuteCompletesWithAResultWithoutUsingTheContext)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    StepOverCommand command;
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    EXPECT_NE(nullptr, future.get());
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
