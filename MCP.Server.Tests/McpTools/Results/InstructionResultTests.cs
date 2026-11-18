using MCP.Debugger.Types.Disassembler;
using MCP.Server.McpTools.Results;
using MCP.Server.Tests.TestHelpers;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>A disassembled instruction as the tools report it.</summary>
  public class InstructionResultTests
  {
    [Test]
    public void Constructor_Instruction_MapsAddressAndDisassembly()
    {
      // Arrange
      var instruction = new Instruction(new IntPtr(TestAddresses.Sample), 2, "mov eax, ebx");

      // Act
      var result = new InstructionResult(instruction);

      // Assert
      Assert.That(result.VirtualAdress, Is.EqualTo((ulong)TestAddresses.Sample));
      Assert.That(result.Disassembly, Is.EqualTo("mov eax, ebx"));
    }
  }
}
