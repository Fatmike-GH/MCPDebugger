using MCP.Server.Tests.TestHelpers;
using ModelContextProtocol;

namespace MCP.Server.Tests.McpTools
{
  /// <summary>
  /// The tool that converts a hex number into its value: which texts it accepts and how it refuses the others.
  /// It needs no session.
  /// </summary>
  public class DebuggerToolsConversionTests
  {
    /// <summary>Error text for a missing hex number.</summary>
    private const string ParameterNullText = "Parameter cannot be null.";

    /// <summary>Start of the error text for an unexpected exception; the message of the exception follows.</summary>
    private const string InternalErrorPrefix = "Unexpected internal MCP server error! Internal exception: ";

    /// <summary>The tools on mocked collaborators.</summary>
    private DebuggerToolsHarness _harness;

    /// <summary>Creates the tools for a test. No session is running.</summary>
    [SetUp]
    public void CreateTools()
    {
      _harness = new DebuggerToolsHarness();
    }

    [TestCase("FF", 255ul)]
    [TestCase("ff", 255ul)]
    [TestCase("400000", 4194304ul)]
    [TestCase("FFFFFFFFFFFFFFFF", 18446744073709551615ul)]
    [TestCase("0", 0ul)]
    public void Debugger_HexToDecConverter_HexDigits_ReturnsTheValue(string hexNumber, ulong expected)
    {
      // Act
      ulong value = _harness.Tools.Debugger_HexToDecConverter(hexNumber);

      // Assert
      Assert.That(value, Is.EqualTo(expected));
    }

    [Test]
    [SetCulture("de-DE")]
    public void Debugger_HexToDecConverter_GermanCulture_ReturnsTheSameValue()
    {
      // Act
      ulong value = _harness.Tools.Debugger_HexToDecConverter("FFFFFFFF");

      // Assert
      Assert.That(value, Is.EqualTo(4294967295ul));
    }

    [Test]
    public void Debugger_HexToDecConverter_Null_ThrowsParameterNullText()
    {
      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_HexToDecConverter(null));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(ParameterNullText));
    }

    [TestCase("0xFF")]
    [TestCase("")]
    [TestCase("XYZ")]
    [TestCase("10000000000000000")]
    public void Debugger_HexToDecConverter_NotAHexNumber_ThrowsInternalError(string hexNumber)
    {
      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_HexToDecConverter(hexNumber));

      // Assert: the rest of the text is the message of the number parser.
      Assert.That(caught.Message, Does.StartWith(InternalErrorPrefix));
    }
  }
}
