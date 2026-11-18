using MCP.Debugger.IntegrationTests.TestHelpers;
using MCP.Debugger.Types.Disassembler;
using MCP.Debugger.Types.Modules;

namespace MCP.Debugger.IntegrationTests.Managed
{
  /// <summary>
  /// Reading from a debuggee through <see cref="DebuggerService"/>: disassembly, memory and module
  /// information. None of these tests changes the debuggee, so they share one session that stands at the
  /// entry point. The fixture set-up arranges that session, so a test that needs nothing else has no arrange
  /// step of its own.
  /// </summary>
  public class DebuggerServiceInspectionTests
  {
    /// <summary>Number of bytes disassembled at the entry point.</summary>
    private const uint DisassemblyLength = 32;

    /// <summary>
    /// Longest range disassembled at the entry point by the test that lets a range end at every byte. Both
    /// sample targets have instructions of several bytes within it, so some of the ranges end inside one.
    /// </summary>
    private const int LongestEntryRange = 64;

    /// <summary>Size of a memory page; memory behind the last page of an image is not readable.</summary>
    private const uint PageSize = 0x1000;

    /// <summary>Module name of the Windows system library every process loads first.</summary>
    private const string NtdllName = "ntdll.dll";

    /// <summary>The shared session, standing at the entry point.</summary>
    private DebugSession _session;

    /// <summary>Starts the shared session and runs it to the entry point, once for all tests of the fixture.</summary>
    /// <returns>A task that completes when the debuggee stands at its entry point.</returns>
    [OneTimeSetUp]
    public async Task StartSharedSessionAsync()
    {
      _session = new DebugSession();
      try
      {
        await _session.StartAtEntryPointAsync();
      }
      catch
      {
        // The tear-down of a fixture does not run when its set-up failed, so the session is ended here.
        _session.Dispose();
        throw;
      }
    }

    /// <summary>Ends the shared session after the last test of the fixture.</summary>
    [OneTimeTearDown]
    public void EndSharedSession()
    {
      _session?.Dispose();
    }

    [Test]
    public void Disassemble_AtEntryPoint_ReturnsConsecutiveInstructions()
    {
      // Act
      List<Instruction> instructions = _session.Service.Disassemble(_session.EntryPoint, DisassemblyLength).ToList();

      // Assert
      Assert.That(instructions, Is.Not.Empty);
      Assert.That(instructions[0].VirtualAdress, Is.EqualTo(_session.EntryPoint));
      Assert.That(instructions[0].Disassembly, Is.Not.Empty);
      for (int index = 1; index < instructions.Count; index++)
      {
        Instruction previous = instructions[index - 1];
        Assert.That(instructions[index].VirtualAdress, Is.EqualTo(previous.VirtualAdress + (nint)previous.InstructionLength),
          $"Instruction {index} does not start where instruction {index - 1} ends.");
      }
    }

    [Test]
    public void Disassemble_RangesEndingAtEveryByteBehindTheEntryPoint_ReturnOnlyCompleteInstructions()
    {
      // Act
      var results = Enumerable.Range(1, LongestEntryRange)
        .Select(length => (Length: length, Instructions: _session.Service.Disassemble(_session.EntryPoint, (uint)length).ToList()))
        .ToList();

      // Assert
      int rangesEndingInsideAnInstruction = 0;
      foreach (var (length, instructions) in results)
      {
        nint nextAddress = _session.EntryPoint;
        foreach (Instruction instruction in instructions)
        {
          Assert.That(instruction.VirtualAdress, Is.EqualTo(nextAddress),
            $"Range of {length} bytes: an entry does not start where the entry in front of it ends.");
          Assert.That(instruction.Disassembly, Is.Not.Empty, $"Range of {length} bytes: the entry at 0x{nextAddress:X} has no text.");
          Assert.That(instruction.InstructionLength, Is.Positive, $"Range of {length} bytes: the entry at 0x{nextAddress:X} has no length.");
          nextAddress += (nint)instruction.InstructionLength;
        }

        nint rangeEnd = _session.EntryPoint + length;
        Assert.That(nextAddress, Is.LessThanOrEqualTo(rangeEnd), $"Range of {length} bytes: the last entry reaches beyond the range.");
        // Only a range with entries counts: an empty result is also what a failed call returns, and it would
        // not show that the complete instructions in front of the cut-off one are kept.
        if (instructions.Count > 0 && nextAddress < rangeEnd)
        {
          rangesEndingInsideAnInstruction++;
        }
      }

      Assert.That(results[^1].Instructions, Is.Not.Empty, $"The range of {LongestEntryRange} bytes returned no instruction.");
      // Without such a range the test would not have met the case it is about.
      Assert.That(rangesEndingInsideAnInstruction, Is.Positive, "No range with instructions ended inside an instruction.");
    }

    [Test]
    public void ReadMemory_ImageBase_ReturnsMzHeader()
    {
      // Act
      byte[] memory = _session.Service.ReadMemory(_session.ImageBase, 2);

      // Assert
      Assert.That(memory, Is.EqualTo(new byte[] { (byte)'M', (byte)'Z' }));
    }

    [Test]
    public void ReadMemory_RangePastImageEnd_ReturnsNull()
    {
      // Arrange
      uint sizeOfImage = FindModule(SampleTarget.FileName).SizeOfImage;

      // Act
      byte[] wholeImage = _session.Service.ReadMemory(_session.ImageBase, sizeOfImage);
      byte[] pastImageEnd = _session.Service.ReadMemory(_session.ImageBase, sizeOfImage + PageSize);

      // Assert
      Assert.That(wholeImage, Is.Not.Null);
      // The range starts readable and ends unreadable: nothing is returned, not the readable part.
      Assert.That(pastImageEnd, Is.Null);
    }

    [Test]
    public void GetModules_AtEntryPoint_ContainsExecutableAndNtdll()
    {
      // Act
      List<string> moduleNames = _session.Service.GetModules(_session.ProcessId).Select(module => module.ModuleName).ToList();

      // Assert
      Assert.That(moduleNames, Has.Some.EqualTo(SampleTarget.FileName).IgnoreCase);
      Assert.That(moduleNames, Has.Some.EqualTo(NtdllName).IgnoreCase);
    }

    [Test]
    public void GetModuleExports_Ntdll_IsNotEmpty()
    {
      // Arrange
      nint ntdll = _session.Service.GetRemoteModuleHandle(_session.ProcessId, NtdllName);

      // Act
      List<ModuleExport> exports = _session.Service.GetModuleExports(_session.ProcessId, ntdll).ToList();

      // Assert
      Assert.That(exports, Is.Not.Empty);
    }

    [Test]
    public void GetModuleSections_Executable_IsNotEmpty()
    {
      // Arrange
      nint executable = _session.Service.GetRemoteModuleHandle(_session.ProcessId, SampleTarget.FileName);

      // Act
      List<ModuleSection> sections = _session.Service.GetModuleSections(_session.ProcessId, executable).ToList();

      // Assert
      Assert.That(sections, Is.Not.Empty);
    }

    /// <summary>Finds a module of the debuggee by its name.</summary>
    /// <param name="moduleName">Module name, compared without regard to case.</param>
    /// <returns>The module.</returns>
    /// <exception cref="InvalidOperationException">The debuggee has no module of that name.</exception>
    private Module FindModule(string moduleName)
    {
      return _session.Service.GetModules(_session.ProcessId)
        .First(module => string.Equals(module.ModuleName, moduleName, StringComparison.OrdinalIgnoreCase));
    }
  }
}
