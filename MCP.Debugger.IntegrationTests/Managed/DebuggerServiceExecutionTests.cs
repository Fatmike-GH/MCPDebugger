using System.Globalization;
using MCP.Debugger.IntegrationTests.TestHelpers;
using MCP.Debugger.Types.Disassembler;
using MCP.Debugger.Types.Interop;

namespace MCP.Debugger.IntegrationTests.Managed
{
  /// <summary>
  /// Execution control through <see cref="DebuggerService"/>: continue, step into, step over, breakpoint, and
  /// what a breakpoint leaves in the debuggee's memory once it was hit, removed or set at the current stop.
  /// Every test owns its session because each one moves the debuggee. The tests stay within the first
  /// instructions behind the entry point, long before the sample target waits for console input - except the
  /// one about commands sent while the debuggee runs, which lets it run free. Addresses
  /// are never fixed values: they are taken from what the session reports and from the disassembly.
  /// The cases that set a breakpoint right after a go command do not control when the debugger executes the
  /// set - before it resumed the debuggee, while the debuggee runs or at the stop the command leads to. They
  /// state what holds in each of these orders.
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

    /// <summary>
    /// The byte the debugger writes over the first byte of an instruction to arm a breakpoint there (the
    /// <c>int 3</c> instruction). No instruction the tests set a breakpoint on starts with it.
    /// </summary>
    private const byte BreakpointOpcode = 0xCC;

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

    [Test]
    public async Task GoCommands_WhileTheDebuggeeRuns_AreRefusedAndLeaveItRunning()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();

      // Act: resume from the stop at the entry point; no breakpoint is set, so nothing stops the debuggee again
      bool resumed = session.Service.Continue();

      // Assert
      Assert.That(resumed, Is.True);

      // Act: the commands that need a stop, while the debuggee runs
      bool steppedInto = session.Service.StepInto();
      bool steppedOver = session.Service.StepOver();
      bool resumedAgain = session.Service.Continue();
      bool isRunning = session.Service.IsRunning();
      byte[] entryPointByte = session.Service.ReadMemory(session.EntryPoint, 1);

      // Assert
      Assert.That(steppedInto, Is.False);
      Assert.That(steppedOver, Is.False);
      Assert.That(resumedAgain, Is.False);
      // The service answers false for a session that has ended as well. Only a session that still runs serves
      // a memory read, so these two tell a refused command from a debuggee that is gone.
      Assert.That(isRunning, Is.True);
      Assert.That(entryPointByte, Is.Not.Null.And.Length.EqualTo(1));
      Assert.That(session.SingleStepCount, Is.Zero);
      Assert.That(session.BreakpointCount, Is.Zero);
    }

    [Test]
    public async Task StepInto_AtBreakpointRemovedDuringItsStop_LeavesOriginalByte()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      byte originalByte = ReadByte(session, second.VirtualAdress);
      await session.ContinueToBreakpointAsync(second.VirtualAdress);
      session.Service.RemoveBreakpoint(second.VirtualAdress);

      // Act
      CONTEXT stop = await session.StepIntoAsync();

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(AddressAfterStepInto(second)));
      Assert.That(session.SingleStepCount, Is.EqualTo(1));
      Assert.That(session.BreakpointCount, Is.EqualTo(1));
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(originalByte));
    }

    [Test]
    public async Task StepOver_AtBreakpointRemovedDuringItsStop_LeavesOriginalByte()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      byte originalByte = ReadByte(session, second.VirtualAdress);
      await session.ContinueToBreakpointAsync(second.VirtualAdress);
      session.Service.RemoveBreakpoint(second.VirtualAdress);

      // Act
      CONTEXT stop = await session.StepOverAsync();

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(AddressAfterStepOver(second)));
      Assert.That(session.SingleStepCount, Is.EqualTo(1));
      Assert.That(session.BreakpointCount, Is.EqualTo(1));
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(originalByte));
    }

    [Test]
    public async Task Continue_AtBreakpointRemovedDuringItsStop_LeavesOriginalByte()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      byte originalByte = ReadByte(session, second.VirtualAdress);
      nint next = AddressAfterStepOver(second);
      await session.ContinueToBreakpointAsync(second.VirtualAdress);
      session.Service.RemoveBreakpoint(second.VirtualAdress);

      // Act
      CONTEXT stop = await session.ContinueToBreakpointAsync(next);

      // Assert
      // The byte is read at the next stop: a read while the debuggee runs would race the running code.
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(next));
      Assert.That(session.BreakpointCount, Is.EqualTo(2));
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(originalByte));
    }

    [Test]
    public async Task StepInto_AtBreakpointNotRemoved_WritesBreakpointBack()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      await session.ContinueToBreakpointAsync(second.VirtualAdress);

      // Act
      await session.StepIntoAsync();

      // Assert
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(BreakpointOpcode));
    }

    [Test]
    public async Task StepInto_WithBreakpointSetAtStepStopAddress_StepsOnceAndArmsIt()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      byte originalByte = ReadByte(session, second.VirtualAdress);
      CONTEXT stepStop = await session.StepOverAsync();
      Assert.That(DebugSession.InstructionPointer(stepStop), Is.EqualTo(second.VirtualAdress), "A step over the first instruction at the entry point does not end at the second one.");

      // Act: set a breakpoint at the address the debuggee is stopped at
      session.Service.SetBreakpoint(second.VirtualAdress);

      // Assert
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(originalByte));

      // Act: step into
      CONTEXT stop = await session.StepIntoAsync();

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(AddressAfterStepInto(second)));
      Assert.That(session.SingleStepCount, Is.EqualTo(2));
      Assert.That(session.BreakpointCount, Is.Zero);
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(BreakpointOpcode));
    }

    [Test]
    public async Task StepInto_WithBreakpointRemovedAndSetAgainAtItsStop_StepsOnceAndArmsIt()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      byte originalByte = ReadByte(session, second.VirtualAdress);
      await session.ContinueToBreakpointAsync(second.VirtualAdress);

      // Act: remove the breakpoint the debuggee is stopped at and set it again
      session.Service.RemoveBreakpoint(second.VirtualAdress);
      session.Service.SetBreakpoint(second.VirtualAdress);

      // Assert
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(originalByte));

      // Act: step into
      CONTEXT stop = await session.StepIntoAsync();

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(AddressAfterStepInto(second)));
      Assert.That(session.SingleStepCount, Is.EqualTo(1));
      Assert.That(session.BreakpointCount, Is.EqualTo(1));
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(BreakpointOpcode));
    }

    [Test]
    public async Task Continue_WithBreakpointSetAtStopAddress_StopsAtTheNextBreakpoint()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      nint next = AddressAfterStepOver(second);
      CONTEXT stepStop = await session.StepOverAsync();
      Assert.That(DebugSession.InstructionPointer(stepStop), Is.EqualTo(second.VirtualAdress), "A step over the first instruction at the entry point does not end at the second one.");
      session.Service.SetBreakpoint(second.VirtualAdress);

      // Act
      CONTEXT stop = await session.ContinueToBreakpointAsync(next);

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(next));
      Assert.That(session.BreakpointCount, Is.EqualTo(1));
    }

    [Test]
    public async Task StepInto_WithBreakpointSetAtStopAddressRightAfterTheCommand_StepsOnce()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      CONTEXT stepStop = await session.StepOverAsync();
      Assert.That(DebugSession.InstructionPointer(stepStop), Is.EqualTo(second.VirtualAdress), "A step over the first instruction at the entry point does not end at the second one.");

      // Act: step into, then at once set a breakpoint at the address the debuggee is resumed from
      CONTEXT stop = await session.StepIntoAsync(breakpointBehindCommand: second.VirtualAdress);

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(AddressAfterStepInto(second)));
      Assert.That(session.SingleStepCount, Is.EqualTo(2));
      Assert.That(session.BreakpointCount, Is.Zero);
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(BreakpointOpcode));
    }

    [Test]
    public async Task StepOver_WithBreakpointSetAtStopAddressRightAfterTheCommand_StepsOnce()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      CONTEXT stepStop = await session.StepOverAsync();
      Assert.That(DebugSession.InstructionPointer(stepStop), Is.EqualTo(second.VirtualAdress), "A step over the first instruction at the entry point does not end at the second one.");

      // Act: step over, then at once set a breakpoint at the address the debuggee is resumed from
      CONTEXT stop = await session.StepOverAsync(breakpointBehindCommand: second.VirtualAdress);

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(AddressAfterStepOver(second)));
      Assert.That(session.SingleStepCount, Is.EqualTo(2));
      Assert.That(session.BreakpointCount, Is.Zero);
      Assert.That(ReadByte(session, second.VirtualAdress), Is.EqualTo(BreakpointOpcode));
    }

    [Test]
    public async Task Continue_WithBreakpointSetAtStopAddressRightAfterTheCommand_StopsAtTheNextBreakpoint()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      Instruction second = ReadEntrySequence(session)[1];
      nint next = AddressAfterStepOver(second);
      CONTEXT stepStop = await session.StepOverAsync();
      Assert.That(DebugSession.InstructionPointer(stepStop), Is.EqualTo(second.VirtualAdress), "A step over the first instruction at the entry point does not end at the second one.");

      // Act: continue, then at once set a breakpoint at the address the debuggee is resumed from
      CONTEXT stop = await session.ContinueToBreakpointAsync(next, breakpointBehindCommand: second.VirtualAdress);

      // Assert
      Assert.That(DebugSession.InstructionPointer(stop), Is.EqualTo(next));
      Assert.That(session.BreakpointCount, Is.EqualTo(1));
    }

    /// <summary>Reads the byte the debuggee holds at an address right now. The debuggee must be stopped.</summary>
    /// <param name="session">A session whose debuggee is stopped.</param>
    /// <param name="address">Absolute virtual address of the byte.</param>
    /// <returns>The byte in the debuggee's memory, including a breakpoint instruction the debugger wrote there.</returns>
    private static byte ReadByte(DebugSession session, nint address)
    {
      byte[] bytes = session.Service.ReadMemory(address, 1);
      Assert.That(bytes, Is.Not.Null, $"The debuggee's memory at 0x{address:X} could not be read.");
      return bytes[0];
    }

    /// <summary>
    /// Disassembles the first instructions at the entry point. An instruction the end of the range cuts off
    /// is not part of a disassembly, so every entry is a complete instruction.
    /// </summary>
    /// <param name="session">A session that stands at the entry point.</param>
    /// <returns>The complete instructions of the range, in address order.</returns>
    private static IReadOnlyList<Instruction> ReadEntrySequence(DebugSession session)
    {
      return session.Service.Disassemble(session.EntryPoint, EntrySequenceLength).ToList();
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

    /// <summary>
    /// Tells where a step over the given instruction ends: behind a call, otherwise where a step into it ends.
    /// Execution that runs on from the instruction reaches this address without passing the instruction again.
    /// </summary>
    /// <param name="instruction">The instruction that is stepped over.</param>
    /// <returns>The address of the instruction the step stops at.</returns>
    private static nint AddressAfterStepOver(Instruction instruction)
    {
      bool isCall = instruction.Disassembly.StartsWith("call ", StringComparison.Ordinal);
      return isCall ? AddressBehind(instruction) : AddressAfterStepInto(instruction);
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
