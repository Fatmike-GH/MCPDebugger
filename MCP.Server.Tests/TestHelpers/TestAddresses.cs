namespace MCP.Server.Tests.TestHelpers
{
  /// <summary>An address of the debuggee for tests, with the text the tools report it as.</summary>
  internal static class TestAddresses
  {
    /// <summary>
    /// An address below 2 GB: a pointer of either bitness holds it without a sign, so its numeric value is the
    /// same in both builds.
    /// </summary>
    public const int Sample = 0x400000;

#if X64
    /// <summary><see cref="Sample"/> as a 64-bit build writes an address: 16 hex digits.</summary>
    public const string SampleFormatted = "0x0000000000400000";
#else
    /// <summary><see cref="Sample"/> as a 32-bit build writes an address: 8 hex digits.</summary>
    public const string SampleFormatted = "0x00400000";
#endif
  }
}
