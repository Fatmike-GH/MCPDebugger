using MCP.Debugger.IntegrationTests.TestHelpers;
using MCP.Debugger.Types.Modules;

namespace MCP.Debugger.IntegrationTests.Managed
{
  /// <summary>
  /// Writing into the memory of a debuggee through <see cref="DebuggerService"/>. The test changes the
  /// debuggee and therefore owns its session.
  /// </summary>
  public class DebuggerServiceWriteMemoryTests
  {
    /// <summary>Section characteristic of the PE format: the section is writable.</summary>
    private const uint SectionIsWritable = 0x80000000;

    /// <summary>Number of bytes written and read back.</summary>
    private const int ByteCount = 4;

    [Test]
    public async Task WriteMemory_WritableSection_ReadsBackWrittenBytes()
    {
      // Arrange
      using var session = new DebugSession();
      await session.StartAtEntryPointAsync();
      nint executable = session.Service.GetRemoteModuleHandle(session.ProcessId, SampleTarget.FileName);
      ModuleSection writableSection = session.Service.GetModuleSections(session.ProcessId, executable)
        .First(section => (section.Characteristics & SectionIsWritable) != 0);
      byte[] original = session.Service.ReadMemory(writableSection.BaseAddress, ByteCount);
      // Inverted bytes differ from the original in every position, so reading them back proves the write.
      byte[] inverted = original.Select(value => (byte)~value).ToArray();

      // Act
      bool isWritten = session.Service.WriteMemory(writableSection.BaseAddress, inverted);
      byte[] readBack = session.Service.ReadMemory(writableSection.BaseAddress, ByteCount);

      // Assert
      Assert.That(isWritten, Is.True);
      Assert.That(readBack, Is.EqualTo(inverted));
    }
  }
}
