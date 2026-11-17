#include "Mocks/Debugger/Commands/Base/MockCommandContext.h"
#include "Native/Debugger/Commands/DisassembleCommand.h"
#include "Native/Debugger/Commands/DisassembleResult.h"
#include "TestHelpers/Futures.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace MCP::Native::Tests::Debugger::Commands
{
  using namespace MCP::Native::Debugger::Commands;
  using MCP::Native::Disassembler::Instruction;
  using MCP::Native::Tests::Mocks::Debugger::Commands::Base::MockCommandContext;
  using MCP::Native::Tests::TestHelpers::IsCompleted;
  using ::testing::_;
  using ::testing::DoAll;
  using ::testing::Return;
  using ::testing::SetArgReferee;
  using ::testing::StrictMock;

  TEST(DisassembleCommandTests, ExecuteReturnsTheInstructionsTheContextDecoded)
  {
    // Arrange
    const DWORD_PTR address = 0x401000;
    auto instructions = std::make_shared<std::vector<Instruction>>();
    instructions->push_back(Instruction{ address, 1, "nop" });
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, ExecuteDisassemble(address, 32, _))
      .WillOnce(DoAll(SetArgReferee<2>(instructions), Return(true)));
    DisassembleCommand command(address, 32);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    auto* disassembleResult = dynamic_cast<DisassembleResult*>(result.get());
    ASSERT_NE(nullptr, disassembleResult);
    EXPECT_TRUE(disassembleResult->IsSuccessful());
    EXPECT_EQ(instructions, disassembleResult->GetInstructions());
  }

  TEST(DisassembleCommandTests, ExecuteCompletesWithFailureWhenTheContextCannotDisassemble)
  {
    // Arrange
    StrictMock<MockCommandContext> context;
    EXPECT_CALL(context, ExecuteDisassemble(_, _, _)).WillOnce(Return(false));
    DisassembleCommand command(0x401000, 32);
    auto future = command.GetFuture();

    // Act
    command.Execute(&context);

    // Assert
    ASSERT_TRUE(IsCompleted(future));
    auto result = future.get();
    auto* disassembleResult = dynamic_cast<DisassembleResult*>(result.get());
    ASSERT_NE(nullptr, disassembleResult);
    EXPECT_FALSE(disassembleResult->IsSuccessful());
    EXPECT_EQ(nullptr, disassembleResult->GetInstructions());
  }

  TEST(DisassembleCommandTests, GetTypeReturnsDisassemble)
  {
    // Arrange
    DisassembleCommand command(0x401000, 32);

    // Act
    Base::CommandType type = command.GetType();

    // Assert
    EXPECT_EQ(Base::CommandType::CMD_DISASSEMBLE, type);
  }
}
