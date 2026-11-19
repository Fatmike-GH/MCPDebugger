#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Native/Debugger/Commands/TryResolveStringCommand.h"
#include "Native/Debugger/Commands/TryResolveStringResult.h"
#include "TestHelpers/Futures.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Tests::Mocks::Debugger::Commands::Base::MockCommandContext;
  using MCP::Native::Tests::TestHelpers::IsCompleted;
  using ::testing::_;
  using ::testing::Return;
  using ::testing::StrictMock;

  TEST(TryResolveStringCommandTests, ExecuteReturnsTheTextTheContextResolved)
  {
    // Arrange
    const DWORD_PTR address = 0x403000;
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, ExecuteTryResolveString(address)).WillOnce(Return(std::string("secret")));
    TryResolveStringCommand command(address);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    auto* stringResult = dynamic_cast<TryResolveStringResult*>(result.get());
    ASSERT_NE(nullptr, stringResult);
    EXPECT_TRUE(stringResult->IsSuccessful());
    EXPECT_EQ("secret", stringResult->GetResult());
  }

  TEST(TryResolveStringCommandTests, ExecuteCompletesWithFailureWhenTheContextResolvesNoText)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, ExecuteTryResolveString(_)).WillOnce(Return(std::string()));
    TryResolveStringCommand command(0x403000);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    auto* stringResult = dynamic_cast<TryResolveStringResult*>(result.get());
    ASSERT_NE(nullptr, stringResult);
    EXPECT_FALSE(stringResult->IsSuccessful());
    EXPECT_TRUE(stringResult->GetResult().empty());
  }
}
