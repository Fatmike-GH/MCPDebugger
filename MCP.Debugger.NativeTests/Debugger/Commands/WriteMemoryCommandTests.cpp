#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Native/Debugger/Commands/CommandResult.h"
#include "Native/Debugger/Commands/WriteMemoryCommand.h"
#include "TestHelpers/Futures.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Commands::Base::MockCommandContext;
  using MCP::Native::Tests::TestHelpers::IsCompleted;
  using ::testing::_;
  using ::testing::Args;
  using ::testing::ElementsAreArray;
  using ::testing::Return;
  using ::testing::StrictMock;

  // The command keeps the caller's buffer pointer instead of a copy, so every buffer below
  // is declared before the command and outlives the call of Execute.

  TEST(WriteMemoryCommandTests, ExecutePassesAddressAndBytesToTheContext)
  {
    // Arrange
    const DWORD_PTR address = 0x405000;
    const BYTE data[] = { 0x01, 0x02, 0x03 };
    StrictMock<MockCommandContext> context;
    // The bytes are compared, not the pointer: which buffer holds them is up to the command.
    EXPECT_CALL(context, ExecuteWriteMemory(address, _, sizeof(data)))
      .With(Args<1, 2>(ElementsAreArray(data)))
      .WillOnce(Return(true));
    WriteMemoryCommand command(address, data, sizeof(data));
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    EXPECT_TRUE(future.get()->IsSuccessful());
  }

  TEST(WriteMemoryCommandTests, ExecuteCompletesWithFailureWhenTheContextCannotWrite)
  {
    // Arrange
    const BYTE data[] = { 0x01 };
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, ExecuteWriteMemory(_, _, _)).WillOnce(Return(false));
    WriteMemoryCommand command(0x405000, data, sizeof(data));
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    EXPECT_FALSE(future.get()->IsSuccessful());
  }
}
