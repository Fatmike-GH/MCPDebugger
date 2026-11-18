using MCP.Debugger.Types.Interop;
using MCP.Server.McpTools.Results;

namespace MCP.Server.Tests.McpTools.Results
{
  /// <summary>
  /// A thread context as the tools report it: the registers of this build's bitness. Every register gets a
  /// value of its own, so a register mapped from the wrong field is noticed.
  /// </summary>
  public class ContextResultTests
  {
#if X64
    [Test]
    public void Constructor_Context_MapsTheRegistersOfA64BitThread()
    {
      // Arrange
      var context = new CONTEXT
      {
        SegCs = 1, SegDs = 2, SegEs = 3, SegFs = 4, SegGs = 5, SegSs = 6, EFlags = 7,
        Rax = 8, Rcx = 9, Rdx = 10, Rbx = 11, Rsp = 12, Rbp = 13, Rsi = 14, Rdi = 15,
        R8 = 16, R9 = 17, R10 = 18, R11 = 19, R12 = 20, R13 = 21, R14 = 22, R15 = 23,
        Rip = 0x7FF612340000
      };

      // Act
      var result = new ContextResult(context);

      // Assert
      Assert.That(result.SegCs, Is.EqualTo((ushort)1));
      Assert.That(result.SegDs, Is.EqualTo((ushort)2));
      Assert.That(result.SegEs, Is.EqualTo((ushort)3));
      Assert.That(result.SegFs, Is.EqualTo((ushort)4));
      Assert.That(result.SegGs, Is.EqualTo((ushort)5));
      Assert.That(result.SegSs, Is.EqualTo((ushort)6));
      Assert.That(result.EFlags, Is.EqualTo(7u));
      Assert.That(result.Rax, Is.EqualTo(8ul));
      Assert.That(result.Rcx, Is.EqualTo(9ul));
      Assert.That(result.Rdx, Is.EqualTo(10ul));
      Assert.That(result.Rbx, Is.EqualTo(11ul));
      Assert.That(result.Rsp, Is.EqualTo(12ul));
      Assert.That(result.Rbp, Is.EqualTo(13ul));
      Assert.That(result.Rsi, Is.EqualTo(14ul));
      Assert.That(result.Rdi, Is.EqualTo(15ul));
      Assert.That(result.R8, Is.EqualTo(16ul));
      Assert.That(result.R9, Is.EqualTo(17ul));
      Assert.That(result.R10, Is.EqualTo(18ul));
      Assert.That(result.R11, Is.EqualTo(19ul));
      Assert.That(result.R12, Is.EqualTo(20ul));
      Assert.That(result.R13, Is.EqualTo(21ul));
      Assert.That(result.R14, Is.EqualTo(22ul));
      Assert.That(result.R15, Is.EqualTo(23ul));
      Assert.That(result.Rip, Is.EqualTo(0x7FF612340000ul));
    }
#else
    [Test]
    public void Constructor_Context_MapsTheRegistersOfA32BitThread()
    {
      // Arrange
      var context = new CONTEXT
      {
        SegGs = 1, SegFs = 2, SegEs = 3, SegDs = 4,
        Edi = 5, Esi = 6, Ebx = 7, Edx = 8, Ecx = 9, Eax = 10,
        Ebp = 11, Eip = 0x401000, SegCs = 12, EFlags = 13, Esp = 14, SegSs = 15
      };

      // Act
      var result = new ContextResult(context);

      // Assert
      Assert.That(result.SegGs, Is.EqualTo(1u));
      Assert.That(result.SegFs, Is.EqualTo(2u));
      Assert.That(result.SegEs, Is.EqualTo(3u));
      Assert.That(result.SegDs, Is.EqualTo(4u));
      Assert.That(result.Edi, Is.EqualTo(5u));
      Assert.That(result.Esi, Is.EqualTo(6u));
      Assert.That(result.Ebx, Is.EqualTo(7u));
      Assert.That(result.Edx, Is.EqualTo(8u));
      Assert.That(result.Ecx, Is.EqualTo(9u));
      Assert.That(result.Eax, Is.EqualTo(10u));
      Assert.That(result.Ebp, Is.EqualTo(11u));
      Assert.That(result.Eip, Is.EqualTo(0x401000u));
      Assert.That(result.SegCs, Is.EqualTo(12u));
      Assert.That(result.EFlags, Is.EqualTo(13u));
      Assert.That(result.Esp, Is.EqualTo(14u));
      Assert.That(result.SegSs, Is.EqualTo(15u));
    }
#endif
  }
}
