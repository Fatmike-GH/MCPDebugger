#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Native/Debugger/Commands/Command.h"
#include "TestHelpers/Futures.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Commands::Base::MockCommandContext;
  using MCP::Native::Tests::TestHelpers::IsCompleted;
  using ::testing::StrictMock;

  TEST(CommandTests, ExecuteCompletesWithAResultWithoutUsingTheContext)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    Command command(Base::CommandType::CMD_CONTINUE);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    EXPECT_NE(nullptr, future.get());
  }

  TEST(CommandTests, GetTypeReturnsTheTypeTheCommandWasCreatedWith)
  {
    // Arrange
    Command command(Base::CommandType::CMD_CONTINUE);

    // Act
    Base::CommandType type = command.GetType();

    // Assert
    EXPECT_EQ(Base::CommandType::CMD_CONTINUE, type);
  }
}
