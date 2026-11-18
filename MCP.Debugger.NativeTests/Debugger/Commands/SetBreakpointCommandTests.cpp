#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Mocks/Debugger/Interfaces/MockBreakpointManager.h"
#include "Native/Debugger/Commands/CommandResult.h"
#include "Native/Debugger/Commands/SetBreakpointCommand.h"
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

  TEST(SetBreakpointCommandTests, ExecuteSetsTheBreakpointAtItsAddressAndReportsSuccess)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    StrictMock<MockBreakpointManager> breakpointManager;
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, GetBreakpointManager()).WillOnce(Return(&breakpointManager));
    EXPECT_CALL(breakpointManager, SetBreakpoint(address)).WillOnce(Return(true));
    SetBreakpointCommand command(address);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    EXPECT_TRUE(future.get()->IsSuccessful());
  }

  TEST(SetBreakpointCommandTests, ExecuteCompletesWithFailureWhenTheBreakpointManagerRefuses)
  {
    // Arrange
    StrictMock<MockBreakpointManager> breakpointManager;
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, GetBreakpointManager()).WillOnce(Return(&breakpointManager));
    EXPECT_CALL(breakpointManager, SetBreakpoint(_)).WillOnce(Return(false));
    SetBreakpointCommand command(0x401000);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    EXPECT_FALSE(future.get()->IsSuccessful());
  }

  TEST(SetBreakpointCommandTests, GetTypeReturnsSetBreakpoint)
  {
    // Arrange
    SetBreakpointCommand command(0x401000);

    // Act
    Base::CommandType type = command.GetType();

    // Assert
    EXPECT_EQ(Base::CommandType::CMD_SET_BREAKPOINT, type);
  }
}
