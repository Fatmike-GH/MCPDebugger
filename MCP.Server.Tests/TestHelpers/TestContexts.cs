using MCP.Debugger.Types.Interop;

namespace MCP.Server.Tests.TestHelpers
{
  /// <summary>Thread contexts of this build's bitness for tests.</summary>
  internal static class TestContexts
  {
    /// <summary>Creates a thread context that stands at an instruction; every other register is zero.</summary>
    /// <param name="instructionPointer">Address of the next instruction to execute.</param>
    /// <returns>The thread context.</returns>
    public static CONTEXT AtInstructionPointer(nint instructionPointer)
    {
      var context = new CONTEXT();
#if X64
      context.Rip = (ulong)instructionPointer;
#else
      context.Eip = (uint)instructionPointer;
#endif
      return context;
    }
  }
}
