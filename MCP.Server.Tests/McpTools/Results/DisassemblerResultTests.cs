using MCP.Debugger.Types.Disassembler;
using MCP.Server.McpTools.Results;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>A disassembly as the tools report it.</summary>
  public class DisassemblerResultTests
  {
    [Test]
    public void Constructor_NullList_HasNoInstructions()
    {
      // Act
      var result = new DisassemblerResult(null);

      // Assert
      Assert.That(result.Instructions, Is.Empty);
    }

    [Test]
    public void Constructor_Instructions_MapsEveryInstructionInOrder()
    {
      // Arrange
      var instructions = new List<Instruction>
      {
        new Instruction(new IntPtr(0x1000), 1, "push ebp"),
        new Instruction(new IntPtr(0x1001), 2, "mov ebp, esp")
      };

      // Act
      var result = new DisassemblerResult(instructions);

      // Assert
      Assert.That(result.Instructions.Select(instruction => instruction.Disassembly), Is.EqualTo(new[] { "push ebp", "mov ebp, esp" }));
      Assert.That(result.Instructions.Select(instruction => instruction.VirtualAdress), Is.EqualTo(new[] { 0x1000ul, 0x1001ul }));
    }
  }
}
