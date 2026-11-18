#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Mocks/Debugger/Interfaces/MockBreakpointManager.h"
#include "Native/Debugger/Commands/CommandResult.h"
#include "Native/Debugger/Commands/RemoveBreakpointCommand.h"
#include "TestHelpers/Futures.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Commands::Base::MockCommandContext;
  using MCP::Native::Tests::Mocks::Debugger::Interfaces::MockBreakpointManager;
  using MCP::Native::Tests::TestHelpers::IsCompleted;
  using ::testing::_;
  using ::testing::Return;
  using ::testing::StrictMock;

  TEST(RemoveBreakpointCommandTests, ExecuteRemovesTheBreakpointAtItsAddressAndReportsSuccess)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    StrictMock<MockBreakpointManager> breakpointManager;
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, GetBreakpointManager()).WillOnce(Return(&breakpointManager));
    EXPECT_CALL(breakpointManager, RemoveBreakpoint(address)).WillOnce(Return(true));
    RemoveBreakpointCommand command(address);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    EXPECT_TRUE(future.get()->IsSuccessful());
  }

  TEST(RemoveBreakpointCommandTests, ExecuteCompletesWithFailureWhenTheBreakpointManagerKnowsNoBreakpointThere)
  {
    // Arrange
    StrictMock<MockBreakpointManager> breakpointManager;
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, GetBreakpointManager()).WillOnce(Return(&breakpointManager));
    EXPECT_CALL(breakpointManager, RemoveBreakpoint(_)).WillOnce(Return(false));
    RemoveBreakpointCommand command(0x401000);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    EXPECT_FALSE(future.get()->IsSuccessful());
  }

  TEST(RemoveBreakpointCommandTests, GetTypeReturnsRemoveBreakpoint)
  {
    // Arrange
    RemoveBreakpointCommand command(0x401000);

    // Act
    Base::CommandType type = command.GetType();

    // Assert
    EXPECT_EQ(Base::CommandType::CMD_REMOVE_BREAKPOINT, type);
  }
}
