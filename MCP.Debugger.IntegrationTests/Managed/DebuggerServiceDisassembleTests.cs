using MCP.Debugger.IntegrationTests.TestHelpers;
using MCP.Debugger.Types.Disassembler;

namespace MCP.Debugger.IntegrationTests.Managed
{
  /// <summary>
  /// Disassembling memory of a debuggee that holds bytes which are no instruction, through
  /// <see cref="DebuggerService"/>. The test writes those bytes into the debuggee and therefore owns its
  /// session.
  /// </summary>
  public class DebuggerServiceDisassembleTests
  {
    /// <summary>Section characteristic of the PE format: the section is writable.</summary>
    private const uint SectionIsWritable = 0x80000000;

    /// <summary>
    /// Machine code that means the same in the 32-bit and the 64-bit debuggee: 90 is <c>nop</c>; at the 0F no
    /// instruction starts (0F 0A is none); decoded again one byte further, 0A C0 is <c>or al, al</c>; the
    /// last 90 is <c>nop</c>.
    /// </summary>
    private static readonly byte[] CodeWithAByteThatIsNoInstruction = { 0x90, 0x0F, 0x0A, 0xC0, 0x90 };

    [Test]
    public async Task Disassemble_ByteThatIsNoInstruction_ReturnsItAsDataByteBetweenTheInstructions()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      nint executable = session.Service.GetRemoteModuleHandle(session.ProcessId, SampleTarget.FileName);
      nint address = session.Service.GetModuleSections(session.ProcessId, executable)
        .First(section => (section.Characteristics & SectionIsWritable) != 0).BaseAddress;
      bool isWritten = session.Service.WriteMemory(address, CodeWithAByteThatIsNoInstruction);
      Assert.That(isWritten, Is.True, $"The machine code could not be written to 0x{address:X} of the debuggee.");

      // Act
      List<Instruction> instructions =
        session.Service.Disassemble(address, (uint)CodeWithAByteThatIsNoInstruction.Length).ToList();

      // Assert: the entries follow each other without a gap and end with the range
      Assert.That(instructions.Select(instruction => instruction.Disassembly),
                  Is.EqualTo(new[] { "nop", "db 0x0F", "or al, al", "nop" }));
      Assert.That(instructions.Select(instruction => instruction.VirtualAdress),
                  Is.EqualTo(new[] { address, address + 1, address + 2, address + 4 }));
      Assert.That(instructions.Select(instruction => instruction.InstructionLength),
                  Is.EqualTo(new uint[] { 1, 1, 2, 1 }));
    }
  }
}
