#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Native/Debugger/Commands/CommandResult.h"
#include "Native/Debugger/Commands/StopCommand.h"
#include "TestHelpers/Futures.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Commands::Base::MockCommandContext;
  using MCP::Native::Tests::TestHelpers::IsCompleted;
  using ::testing::StrictMock;

  TEST(StopCommandTests, ExecuteClearsTheQueueAndCompletesWithSuccess)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, ExecuteClearCommandQueue()).Times(1);
    StopCommand command;
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    EXPECT_TRUE(future.get()->IsSuccessful());
  }

  TEST(StopCommandTests, GetTypeReturnsStop)
  {
    // Arrange
    StopCommand command;

    // Act
    Base::CommandType type = command.GetType();

    // Assert
    EXPECT_EQ(Base::CommandType::CMD_STOP, type);
  }
}
