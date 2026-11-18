#include "Mocks/Disassembler/Interfaces/MockSymbolResolver.h"
#include "Mocks/Platform/Interfaces/MockProcessMemory.h"
#include "Native/Disassembler/Disassembler.h"
#include "Native/Disassembler/Instruction.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace MCP::Native::Tests::Disassembler
{
  using MCP::Native::Disassembler::Instruction;
  using MCP::Native::Tests::Mocks::Disassembler::Interfaces::MockSymbolResolver;
  using MCP::Native::Tests::Mocks::Platform::Interfaces::MockProcessMemory;
  using ::testing::_;
  using ::testing::AllOf;
  using ::testing::ElementsAre;
  using ::testing::EndsWith;
  using ::testing::Field;
  using ::testing::IsEmpty;
  using ::testing::Matcher;
  using ::testing::Return;
  using ::testing::StartsWith;
  using ::testing::StrictMock;

  /// <summary>
  /// Tests what the disassembler makes of machine code: instructions, bytes that are no instruction, and a
  /// range or a read that ends too early. The bytes come from a double of the process memory, so no test has a
  /// debuggee or calls the operating system. Both doubles are strict: a test fails when the disassembler reads
  /// memory or asks for a symbol it was not expected to.
  ///
  /// The byte sequences mean the same in the 32-bit and the 64-bit build: 90 is nop, 0A C0 is or al, al,
  /// E8 with four bytes is a call, F3 A4 is rep movsb; 0F 0A and FF FF are no instruction in either mode.
  /// </summary>
  class DisassemblerTests : public ::testing::Test
  {
  protected:
    static constexpr DWORD ProcessId = 1000;
    static constexpr DWORD_PTR StartAddress = 0x401000;

    /// <summary>The one-byte instruction that does nothing.</summary>
    static constexpr BYTE Nop = 0x90;

    /// <summary>
    /// Number of bytes the call check has to read: one more than the longest instruction, so that the bytes
    /// read never end inside the instruction at the address.
    /// </summary>
    static constexpr size_t CallCheckReadSize = 16;

    /// <summary>A length no instruction has; a test starts with it to see that the call check wrote one.</summary>
    static constexpr size_t UnsetLength = 999;

    /// <summary>
    /// Lets the memory double answer one read of exactly these bytes at the start address.
    /// </summary>
    void ExpectRangeRead(const std::vector<BYTE>& bytes)
    {
      EXPECT_CALL(_processMemory, Read(_process, StartAddress, _, bytes.size()))
        .WillOnce([bytes](HANDLE, DWORD_PTR, BYTE* buffer, size_t)
        {
          std::memcpy(buffer, bytes.data(), bytes.size());
          return true;
        });
    }

    /// <summary>
    /// Lets the memory double answer the read of the call check at the start address: the given bytes,
    /// followed by nop instructions up to the size of the read.
    /// </summary>
    void ExpectCallCheckRead(const std::vector<BYTE>& bytes)
    {
      EXPECT_CALL(_processMemory, Read(_process, StartAddress, _, CallCheckReadSize))
        .WillOnce([bytes](HANDLE, DWORD_PTR, BYTE* buffer, size_t size)
        {
          std::memset(buffer, Nop, size);
          std::memcpy(buffer, bytes.data(), bytes.size());
          return true;
        });
    }

    /// <summary>Decodes the given number of bytes at the start address.</summary>
    std::shared_ptr<std::vector<Instruction>> DisassembleRange(SSIZE_T length)
    {
      std::shared_ptr<std::vector<Instruction>> instructions;
      _disassembler.Disassemble(_process, ProcessId, StartAddress, length, instructions);
      return instructions;
    }

    /// <summary>Matches a list entry by its address, its length and its whole text.</summary>
    static Matcher<const Instruction&> IsEntry(DWORD_PTR address, DWORD_PTR length, const std::string& text)
    {
      return AllOf(Field("VA", &Instruction::VA, address),
                   Field("InstructionLength", &Instruction::InstructionLength, length),
                   Field("Disassembly", &Instruction::Disassembly, text));
    }

    // Only the doubles see this value, and they compare it as a number: it is never used as a handle of the
    // operating system, so the cast from a number is safe.
    const HANDLE _process = reinterpret_cast<HANDLE>(static_cast<DWORD_PTR>(0x1234));

    StrictMock<MockSymbolResolver> _symbolResolver;
    StrictMock<MockProcessMemory> _processMemory;
    MCP::Native::Disassembler::Disassembler _disassembler{ _symbolResolver, _processMemory };
  };

  TEST_F(DisassemblerTests, DisassembleReportsAByteThatIsNoInstructionAsADataByteAndGoesOnBehindIt)
  {
    // Arrange: nop, then 0F 0A which is no instruction, then - one byte further - or al, al and nop
    ExpectRangeRead({ Nop, 0x0F, 0x0A, 0xC0, Nop });

    // Act
    auto instructions = DisassembleRange(5);

    // Assert: the data byte has no symbol - the strict resolver was not asked for one
    ASSERT_NE(nullptr, instructions);
    EXPECT_THAT(*instructions, ElementsAre(IsEntry(StartAddress, 1, "nop"),
                                           IsEntry(StartAddress + 1, 1, "db 0x0F"),
                                           IsEntry(StartAddress + 2, 2, "or al, al"),
                                           IsEntry(StartAddress + 4, 1, "nop")));
  }

  TEST_F(DisassemblerTests, DisassembleReportsEveryPositionThatIsNoInstructionAsItsOwnDataByte)
  {
    // Arrange: FF FF is no instruction at the first three positions; the last FF alone could begin one
    ExpectRangeRead({ 0xFF, 0xFF, 0xFF, 0xFF });

    // Act
    auto instructions = DisassembleRange(4);

    // Assert
    ASSERT_NE(nullptr, instructions);
    EXPECT_THAT(*instructions, ElementsAre(IsEntry(StartAddress, 1, "db 0xFF"),
                                           IsEntry(StartAddress + 1, 1, "db 0xFF"),
                                           IsEntry(StartAddress + 2, 1, "db 0xFF")));
  }

  TEST_F(DisassemblerTests, DisassembleLeavesOutAnInstructionTheEndOfTheRangeCutsOff)
  {
    // Arrange: nop, then the first three of the five bytes of a call
    ExpectRangeRead({ Nop, 0xE8, 0x00, 0x00 });

    // Act
    auto instructions = DisassembleRange(4);

    // Assert
    ASSERT_NE(nullptr, instructions);
    EXPECT_THAT(*instructions, ElementsAre(IsEntry(StartAddress, 1, "nop")));
  }

  TEST_F(DisassemblerTests, DisassembleGivesAnEmptyListForARangeThatHoldsOnlyTheBeginningOfAnInstruction)
  {
    // Arrange: the first three of the five bytes of a call
    ExpectRangeRead({ 0xE8, 0x00, 0x00 });

    // Act
    auto instructions = DisassembleRange(3);

    // Assert
    ASSERT_NE(nullptr, instructions);
    EXPECT_THAT(*instructions, IsEmpty());
  }

  TEST_F(DisassemblerTests, DisassembleAppendsTheSymbolOfTheAddressAnInstructionRefersTo)
  {
    // Arrange: push of a 32-bit value that is large enough to be an address
    const DWORD_PTR pushedAddress = 0x403228;
    const DWORD_PTR pushLength = 5;
    ExpectRangeRead({ 0x68, 0x28, 0x32, 0x40, 0x00 });
    EXPECT_CALL(_symbolResolver, Resolve(_process, ProcessId, pushedAddress)).WillOnce(Return("<Target>"));

    // Act
    auto instructions = DisassembleRange(5);

    // Assert
    ASSERT_NE(nullptr, instructions);
    EXPECT_THAT(*instructions, ElementsAre(AllOf(Field("VA", &Instruction::VA, StartAddress),
                                                 Field("InstructionLength", &Instruction::InstructionLength, pushLength),
                                                 Field("Disassembly", &Instruction::Disassembly,
                                                       AllOf(StartsWith("push "), EndsWith(" <Target>"))))));
  }

  TEST_F(DisassemblerTests, DisassembleLeavesTheListUnsetWhenTheMemoryCannotBeRead)
  {
    // Arrange
    EXPECT_CALL(_processMemory, Read(_process, StartAddress, _, size_t{ 4 })).WillOnce(Return(false));

    // Act
    auto instructions = DisassembleRange(4);

    // Assert
    EXPECT_EQ(nullptr, instructions);
  }

  TEST_F(DisassemblerTests, DisassembleReadsNoMemoryForAnEmptyRange)
  {
    // Act
    auto instructions = DisassembleRange(0);

    // Assert: the strict memory double was not asked
    EXPECT_EQ(nullptr, instructions);
  }

  TEST_F(DisassemblerTests, IsCallInstructionIsTrueForACallAndReportsItsLength)
  {
    // Arrange
    ExpectCallCheckRead({ 0xE8, 0x00, 0x00, 0x00, 0x00 });
    size_t length = UnsetLength;

    // Act
    bool isCall = _disassembler.IsCallInstruction(_process, StartAddress, length);

    // Assert
    EXPECT_TRUE(isCall);
    EXPECT_EQ(5u, length);
  }

  TEST_F(DisassemblerTests, IsCallInstructionIsTrueForARepeatedInstructionAndReportsItsLength)
  {
    // Arrange: rep movsb
    ExpectCallCheckRead({ 0xF3, 0xA4 });
    size_t length = UnsetLength;

    // Act
    bool isCall = _disassembler.IsCallInstruction(_process, StartAddress, length);

    // Assert
    EXPECT_TRUE(isCall);
    EXPECT_EQ(2u, length);
  }

  TEST_F(DisassemblerTests, IsCallInstructionIsFalseForAPlainInstructionAndReportsItsLength)
  {
    // Arrange
    ExpectCallCheckRead({ Nop });
    size_t length = UnsetLength;

    // Act
    bool isCall = _disassembler.IsCallInstruction(_process, StartAddress, length);

    // Assert
    EXPECT_FALSE(isCall);
    EXPECT_EQ(1u, length);
  }

  TEST_F(DisassemblerTests, IsCallInstructionIsFalseWithoutALengthForBytesThatAreNoInstruction)
  {
    // Arrange
    ExpectCallCheckRead({ 0x0F, 0x0A });
    size_t length = UnsetLength;

    // Act
    bool isCall = _disassembler.IsCallInstruction(_process, StartAddress, length);

    // Assert
    EXPECT_FALSE(isCall);
    EXPECT_EQ(0u, length);
  }

  TEST_F(DisassemblerTests, IsCallInstructionIsFalseWithoutALengthForARepeatPrefixInFrontOfNoInstruction)
  {
    // Arrange: the prefix that makes rep movsb a repeated instruction, here in front of 0F 0A
    ExpectCallCheckRead({ 0xF3, 0x0F, 0x0A });
    size_t length = UnsetLength;

    // Act
    bool isCall = _disassembler.IsCallInstruction(_process, StartAddress, length);

    // Assert
    EXPECT_FALSE(isCall);
    EXPECT_EQ(0u, length);
  }

  TEST_F(DisassemblerTests, IsCallInstructionIsFalseWithoutALengthWhenTheMemoryCannotBeRead)
  {
    // Arrange: also what a call near the end of readable memory looks like - its read fails as a whole
    EXPECT_CALL(_processMemory, Read(_process, StartAddress, _, _)).WillOnce(Return(false));
    size_t length = UnsetLength;

    // Act
    bool isCall = _disassembler.IsCallInstruction(_process, StartAddress, length);

    // Assert
    EXPECT_FALSE(isCall);
    EXPECT_EQ(0u, length);
  }
}
