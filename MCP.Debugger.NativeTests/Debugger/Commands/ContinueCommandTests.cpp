#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Native/Debugger/Commands/CommandResult.h"
#include "Native/Debugger/Commands/ContinueCommand.h"
#include "Native/Debugger/Interfaces/IResumePlan.h"
#include "TestHelpers/Futures.h"
#include "TestHelpers/ResumePlanProbe.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <utility>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Debugger::Interfaces::IResumePlan;
  using MCP::Native::Tests::Mocks::Debugger::Commands::Base::MockCommandContext;
  using MCP::Native::Tests::TestHelpers::BehaviourOfContinue;
  using MCP::Native::Tests::TestHelpers::IsCompleted;
  using MCP::Native::Tests::TestHelpers::ProbeAtACall;
  using ::testing::_;
  using ::testing::StrictMock;

  // The strict context with one expected hand-over fails a test when the command hands over a second plan.

  TEST(ContinueCommandTests, ExecuteHandsOverThePlanOfAContinueAndSucceedsWhenTheContextAcceptsIt)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    std::unique_ptr<IResumePlan> handedOver;
    EXPECT_CALL(context, Resume(_)).WillOnce([&handedOver](std::unique_ptr<IResumePlan> plan)
    {
      handedOver = std::move(plan);
      return true;
    });
    ContinueCommand command;
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(result->IsSuccessful());
    ASSERT_NE(nullptr, handedOver);
    EXPECT_EQ(BehaviourOfContinue, ProbeAtACall(*handedOver));
  }

  TEST(ContinueCommandTests, ExecuteCompletesWithAFailedResultWhenTheContextRefusesThePlan)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    std::unique_ptr<IResumePlan> handedOver;
    EXPECT_CALL(context, Resume(_)).WillOnce([&handedOver](std::unique_ptr<IResumePlan> plan)
    {
      handedOver = std::move(plan);
      return false;
    });
    ContinueCommand command;
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    ASSERT_NE(nullptr, result);
    EXPECT_FALSE(result->IsSuccessful());
    ASSERT_NE(nullptr, handedOver);
    EXPECT_EQ(BehaviourOfContinue, ProbeAtACall(*handedOver));
  }
}
