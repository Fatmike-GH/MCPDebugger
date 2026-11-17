using System.Globalization;
using MCP.Debugger.IntegrationTests.TestHelpers;
using MCP.Debugger.Types.Disassembler;
using MCP.Debugger.Types.Interop;

namespace MCP.Debugger.IntegrationTests.Managed
{
  /// <summary>
  /// Execution control through <see cref="DebuggerService"/>: continue, step into, step over, breakpoint.
  /// Every test owns its session because each one moves the debuggee. The tests stay within the first
  /// instructions behind the entry point, long before the sample target waits for console input. Addresses
  /// are never fixed values: they are taken from what the session reports and from the disassembly.
  /// </summary>
  public class DebuggerServiceExecutionTests
  {
    /// <summary>
    /// Number of bytes disassembled at the entry point. Enough for the first instructions of both sample
    /// targets, including their first call.
    /// </summary>
    private const uint EntrySequenceLength = 32;

    /// <summary>Prefix of the hexadecimal target address in the text of a direct call or jump.</summary>
    private const string HexPrefix = "0x";

    [Test]
    public async Task Continue_FromSystemBreakpoint_StopsAtEntryPoint()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAsync();

      // Act
      CONTEXT stop = await session.ContinueToEntryPointAsync();

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(session.EntryPoint));
    }

    [Test]
    public async Task StepInto_AtEntryPoint_StopsAtNextInstruction()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction first = ReadEntrySequence(session)[0];

      // Act
      CONTEXT stop = await session.StepIntoAsync();

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(AddressAfterStepInto(first)));
      Assert.That(session.SingleStepCount, Is.EqualTo(1));
    }

    [Test]
    public async Task StepOver_OnCall_StopsBehindTheCall()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      IReadOnlyList<Instruction> entrySequence = ReadEntrySequence(session);
      int callIndex = IndexOfFirstCall(entrySequence);
      Assert.That(callIndex, Is.Not.Negative, "The sample target has no call in the first instructions at its entry point.");
      Instruction call = entrySequence[callIndex];
      nint instructionPointer = session.EntryPoint;
      for (int step = 0; step < callIndex; step++)
      {
        instructionPointer = DebugSession.InstructionPointer(await session.StepIntoAsync());
      }
      Assert.That(instructionPointer, Is.EqualTo(call.VirtualAdress), "The instructions in front of the first call do not run straight into it.");

      // Act
      CONTEXT stop = await session.StepOverAsync();

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(AddressBehind(call)));
    }

    [Test]
    public async Task Continue_WithBreakpointSet_StopsAtBreakpoint()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];

      // Act
      CONTEXT stop = await session.ContinueToBreakpointAsync(second.VirtualAdress);

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(second.VirtualAdress));
    }

    /// <summary>
    /// Disassembles the first instructions at the entry point. The last entry of a disassembly is left out:
    /// the range may end inside an instruction, and what is reported for the cut-off rest is not reliable.
    /// </summary>
    /// <param name="session">A session that stands at the entry point.</param>
    /// <returns>The complete instructions of the range, in address order.</returns>
    private static IReadOnlyList<Instruction> ReadEntrySequence(DebugSession session)
    {
      List<Instruction> instructions = session.Service.Disassemble(session.EntryPoint, EntrySequenceLength).ToList();
      return instructions.Take(instructions.Count - 1).ToList();
    }

    /// <summary>Finds the first call instruction of a disassembly.</summary>
    /// <param name="instructions">Instructions in address order.</param>
    /// <returns>The index of the first call, or -1 when there is none.</returns>
    private static int IndexOfFirstCall(IReadOnlyList<Instruction> instructions)
    {
      for (int index = 0; index < instructions.Count; index++)
      {
        if (instructions[index].Disassembly.StartsWith("call ", StringComparison.Ordinal))
        {
          return index;
        }
      }

      return -1;
    }

    /// <summary>
    /// Tells where a step into the given instruction ends: at the target of a direct call or jump (named in
    /// the disassembly text), otherwise at the instruction behind it. The first instruction at the entry point
    /// differs between the 64-bit and the 32-bit sample target, so both cases occur.
    /// </summary>
    /// <param name="instruction">The instruction that is stepped into.</param>
    /// <returns>The address of the instruction executed next.</returns>
    private static nint AddressAfterStepInto(Instruction instruction)
    {
      string text = instruction.Disassembly;
      bool isDirectBranch = text.StartsWith("call " + HexPrefix, StringComparison.Ordinal)
        || text.StartsWith("jmp " + HexPrefix, StringComparison.Ordinal);
      if (!isDirectBranch)
      {
        return AddressBehind(instruction);
      }

      string target = text.Substring(text.IndexOf(HexPrefix, StringComparison.Ordinal) + HexPrefix.Length);
      return unchecked((nint)ulong.Parse(target, NumberStyles.HexNumber, CultureInfo.InvariantCulture));
    }

    /// <summary>Computes the address of the instruction that follows the given one in memory.</summary>
    /// <param name="instruction">An instruction.</param>
    /// <returns>Its address plus its length.</returns>
    private static nint AddressBehind(Instruction instruction)
    {
      return instruction.VirtualAdress + (nint)instruction.InstructionLength;
    }
  }
}
