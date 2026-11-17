#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Native/Debugger/Commands/ReadMemoryCommand.h"
#include "Native/Debugger/Commands/ReadMemoryResult.h"
#include "TestHelpers/Futures.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Commands::Base::MockCommandContext;
  using MCP::Native::Tests::TestHelpers::IsCompleted;
  using ::testing::_;
  using ::testing::DoAll;
  using ::testing::Return;
  using ::testing::SetArgReferee;
  using ::testing::StrictMock;

  TEST(ReadMemoryCommandTests, ExecuteReturnsTheBufferTheContextRead)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    auto buffer = std::make_shared<std::vector<BYTE>>(std::vector<BYTE>{ 0x90, 0xCC });
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, ExecuteReadMemory(address, 2, _))
      .WillOnce(DoAll(SetArgReferee<2>(buffer), Return(true)));
    ReadMemoryCommand command(address, 2);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    auto* readResult = dynamic_cast<ReadMemoryResult*>(result.get());
    ASSERT_NE(nullptr, readResult);
    EXPECT_TRUE(readResult->IsSuccessful());
    EXPECT_EQ(buffer, readResult->GetData());
  }

  TEST(ReadMemoryCommandTests, ExecuteCompletesWithFailureWhenTheContextCannotRead)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, ExecuteReadMemory(_, _, _)).WillOnce(Return(false));
    ReadMemoryCommand command(0x401000, 16);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    auto* readResult = dynamic_cast<ReadMemoryResult*>(result.get());
    ASSERT_NE(nullptr, readResult);
    EXPECT_FALSE(readResult->IsSuccessful());
    EXPECT_EQ(nullptr, readResult->GetData());
  }

  TEST(ReadMemoryCommandTests, GetTypeReturnsReadMemory)
  {
    // Arrange
    ReadMemoryCommand command(0x401000, 16);

    // Act
    Base::CommandType type = command.GetType();

    // Assert
    EXPECT_EQ(Base::CommandType::CMD_READ_MEMORY, type);
  }
}
