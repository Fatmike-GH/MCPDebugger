using System.Text.Json;
using MCP.Debugger.Types.Disassembler;
using MCP.Server.McpTools.Results;
using MCP.Server.Tests.TestHelpers;
using ModelContextProtocol;
using Moq;

namespace MCP.Server.Tests.McpTools
{
  /// <summary>
  /// The tools that read and write the memory of a ready session: disassemble, try to resolve a string, read
  /// memory and write memory. What they hand to the debugger service, how its answers and failure values become
  /// the tool's answer, and which forms of the data to write are accepted.
  /// </summary>
  public class DebuggerToolsMemoryTests
  {
    /// <summary>The address the tools are called with.</summary>
    private const nint Address = TestAddresses.Sample;

    /// <summary>The number of bytes the disassemble tool is asked to disassemble.</summary>
    private const uint DisassembleLength = 16;

    /// <summary>The number of bytes the read-memory tool is asked to read.</summary>
    private const uint ReadSize = 2;

    /// <summary>Error text of write-memory for data that is no array of numbers.</summary>
    private const string DataFormText = "data parameter must be a string containing a JSON array or a direct JSON array.";

    /// <summary>Start of the error text for an unexpected exception; the message of the exception follows.</summary>
    private const string InternalErrorPrefix = "Unexpected internal MCP server error! Internal exception: ";

    /// <summary>Message of the exception a mock throws as an unexpected failure.</summary>
    private const string FailureMessage = "boom";

    /// <summary>The bytes the write-memory tests expect the service to receive.</summary>
    private static readonly byte[] BytesToWrite = { 1, 2, 255 };

    /// <summary>The tools on mocked collaborators.</summary>
    private DebuggerToolsHarness _harness;

    /// <summary>The bytes the mocked service was asked to write; null when it was not asked.</summary>
    private byte[] _writtenBytes;

    /// <summary>
    /// Creates the tools for a test and a session that has reached its entry point. The mocked service writes
    /// successfully and remembers the bytes.
    /// </summary>
    /// <returns>A task that ends when the session is ready.</returns>
    [SetUp]
    public async Task CreateToolsWithReadySessionAsync()
    {
      _writtenBytes = null;
      _harness = new DebuggerToolsHarness();
      _harness.Service.Setup(service => service.WriteMemory(It.IsAny<nint>(), It.IsAny<byte[]>()))
                      .Callback<nint, byte[]>((va, data) => _writtenBytes = data)
                      .Returns(true);
      await _harness.ReachEntryPointAsync();
    }

    [Test]
    public void Debugger_Disassemble_Instructions_ReturnsAddressAndTextOfEachInOrder()
    {
      // Arrange
      var instructions = new List<Instruction>
      {
        new Instruction(Address, 1, "push ebp"),
        new Instruction(Address + 1, 2, "mov ebp, esp")
      };
      _harness.Service.Setup(service => service.Disassemble(Address, DisassembleLength)).Returns(instructions);

      // Act
      DisassemblerResult result = _harness.Tools.Debugger_Disassemble((ulong)Address, DisassembleLength);

      // Assert
      Assert.That(result.Instructions.Select(instruction => instruction.VirtualAdress),
                  Is.EqualTo(new[] { (ulong)Address, (ulong)Address + 1 }));
      Assert.That(result.Instructions.Select(instruction => instruction.Disassembly),
                  Is.EqualTo(new[] { "push ebp", "mov ebp, esp" }));
    }

    [Test]
    public void Debugger_Disassemble_EmptyList_ReturnsNoInstructions()
    {
      // Arrange
      _harness.Service.Setup(service => service.Disassemble(Address, DisassembleLength)).Returns(new List<Instruction>());

      // Act
      DisassemblerResult result = _harness.Tools.Debugger_Disassemble((ulong)Address, DisassembleLength);

      // Assert
      Assert.That(result.Instructions, Is.Empty);
    }

    [Test]
    public void Debugger_Disassemble_NullList_ReturnsNoInstructions()
    {
      // Arrange
      _harness.Service.Setup(service => service.Disassemble(Address, DisassembleLength)).Returns((IEnumerable<Instruction>)null);

      // Act
      DisassemblerResult result = _harness.Tools.Debugger_Disassemble((ulong)Address, DisassembleLength);

      // Assert
      Assert.That(result.Instructions, Is.Empty);
    }

    [Test]
    public void Debugger_Disassemble_ZeroAddress_PassesItOn()
    {
      // Act
      _harness.Tools.Debugger_Disassemble(0, DisassembleLength);

      // Assert
      _harness.Service.Verify(service => service.Disassemble(nint.Zero, DisassembleLength), Times.Once());
    }

    [Test]
    public void Debugger_Disassemble_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.Service.Setup(service => service.Disassemble(Address, DisassembleLength))
                      .Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_Disassemble((ulong)Address, DisassembleLength));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    [Test]
    public void Debugger_TryResolveString_StringFound_ReturnsIt()
    {
      // Arrange
      _harness.Service.Setup(service => service.TryResolveString(Address)).Returns("Hello, world!");

      // Act
      string text = _harness.Tools.Debugger_TryResolveString((ulong)Address);

      // Assert
      Assert.That(text, Is.EqualTo("Hello, world!"));
    }

    [Test]
    public void Debugger_TryResolveString_NoString_ReturnsEmptyString()
    {
      // Arrange
      _harness.Service.Setup(service => service.TryResolveString(Address)).Returns(string.Empty);

      // Act
      string text = _harness.Tools.Debugger_TryResolveString((ulong)Address);

      // Assert
      Assert.That(text, Is.Empty);
    }

    [Test]
    public void Debugger_TryResolveString_ZeroAddress_PassesItOn()
    {
      // Act
      _harness.Tools.Debugger_TryResolveString(0);

      // Assert
      _harness.Service.Verify(service => service.TryResolveString(nint.Zero), Times.Once());
    }

    [Test]
    public void Debugger_TryResolveString_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.Service.Setup(service => service.TryResolveString(Address))
                      .Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_TryResolveString((ulong)Address));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    [Test]
    public void Debugger_ReadMemory_MemoryRead_ReturnsItsFormattedText()
    {
      // Arrange
      byte[] memory = { 77, 90 };
      _harness.Service.Setup(service => service.ReadMemory(Address, ReadSize)).Returns(memory);
      _harness.MemoryFormatter.Setup(formatter => formatter.Format(It.Is<byte[]>(bytes => ReferenceEquals(bytes, memory))))
                              .Returns("[77,90]");

      // Act
      object result = _harness.Tools.Debugger_ReadMemory((ulong)Address, ReadSize);

      // Assert
      Assert.That(MemoryOf(result), Is.EqualTo("[77,90]"));
    }

    [Test]
    public void Debugger_ReadMemory_MemoryNotReadable_ReturnsTheFormattedTextOfNoMemory()
    {
      // Arrange
      _harness.Service.Setup(service => service.ReadMemory(Address, ReadSize)).Returns((byte[])null);
      _harness.MemoryFormatter.Setup(formatter => formatter.Format(null)).Returns("null");

      // Act
      object result = _harness.Tools.Debugger_ReadMemory((ulong)Address, ReadSize);

      // Assert
      Assert.That(MemoryOf(result), Is.EqualTo("null"));
    }

    [Test]
    public void Debugger_ReadMemory_ZeroAddress_PassesItOn()
    {
      // Act
      _harness.Tools.Debugger_ReadMemory(0, ReadSize);

      // Assert
      _harness.Service.Verify(service => service.ReadMemory(nint.Zero, ReadSize), Times.Once());
    }

    [Test]
    public void Debugger_ReadMemory_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.Service.Setup(service => service.ReadMemory(Address, ReadSize))
                      .Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_ReadMemory((ulong)Address, ReadSize));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    [Test]
    public void Debugger_WriteMemory_JsonArray_WritesItsBytes()
    {
      // Arrange
      JsonElement data = JsonSerializer.SerializeToElement(new[] { 1, 2, 255 });

      // Act
      _harness.Tools.Debugger_WriteMemory((ulong)Address, data);

      // Assert
      Assert.That(_writtenBytes, Is.EqualTo(BytesToWrite));
      _harness.Service.Verify(service => service.WriteMemory(Address, It.IsAny<byte[]>()), Times.Once());
    }

    [Test]
    public void Debugger_WriteMemory_JsonStringHoldingAnArray_WritesItsBytes()
    {
      // Arrange
      JsonElement data = JsonSerializer.SerializeToElement("[1,2,255]");

      // Act
      _harness.Tools.Debugger_WriteMemory((ulong)Address, data);

      // Assert
      Assert.That(_writtenBytes, Is.EqualTo(BytesToWrite));
      _harness.Service.Verify(service => service.WriteMemory(Address, It.IsAny<byte[]>()), Times.Once());
    }

    [Test]
    public void Debugger_WriteMemory_ServiceWrites_ReturnsTrue()
    {
      // Act
      bool written = _harness.Tools.Debugger_WriteMemory((ulong)Address, JsonSerializer.SerializeToElement(new[] { 1 }));

      // Assert
      Assert.That(written, Is.True);
    }

    [Test]
    public void Debugger_WriteMemory_ServiceFails_ReturnsFalse()
    {
      // Arrange
      _harness.Service.Setup(service => service.WriteMemory(Address, It.IsAny<byte[]>())).Returns(false);

      // Act
      bool written = _harness.Tools.Debugger_WriteMemory((ulong)Address, JsonSerializer.SerializeToElement(new[] { 1 }));

      // Assert
      Assert.That(written, Is.False);
    }

    [Test]
    public void Debugger_WriteMemory_ZeroAddress_PassesItOn()
    {
      // Act
      _harness.Tools.Debugger_WriteMemory(0, JsonSerializer.SerializeToElement(new[] { 1 }));

      // Assert
      _harness.Service.Verify(service => service.WriteMemory(nint.Zero, It.IsAny<byte[]>()), Times.Once());
    }

    [TestCaseSource(nameof(DataThatIsNoArray))]
    public void Debugger_WriteMemory_DataIsNoArray_ThrowsDataFormText(object data)
    {
      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_WriteMemory((ulong)Address, data));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(DataFormText));
      _harness.Service.Verify(service => service.WriteMemory(It.IsAny<nint>(), It.IsAny<byte[]>()), Times.Never());
    }

    [TestCaseSource(nameof(DataThatCannotBeReadAsNumbers))]
    public void Debugger_WriteMemory_DataCannotBeReadAsNumbers_ThrowsInternalError(JsonElement data)
    {
      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_WriteMemory((ulong)Address, data));

      // Assert: the rest of the text is the message of the JSON serializer.
      Assert.That(caught.Message, Does.StartWith(InternalErrorPrefix));
      _harness.Service.Verify(service => service.WriteMemory(It.IsAny<nint>(), It.IsAny<byte[]>()), Times.Never());
    }

    [Test]
    public void Debugger_WriteMemory_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.Service.Setup(service => service.WriteMemory(Address, It.IsAny<byte[]>()))
                      .Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _harness.Tools.Debugger_WriteMemory((ulong)Address, JsonSerializer.SerializeToElement(new[] { 1 })));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    /// <summary>
    /// Data the write-memory tool refuses with its own text: JSON values that are neither an array nor a string
    /// holding one, and objects that are no JSON value at all - the tool reads nothing but a JSON value.
    /// </summary>
    /// <returns>One test case per form of the data.</returns>
    private static IEnumerable<TestCaseData> DataThatIsNoArray()
    {
      yield return new TestCaseData(JsonSerializer.SerializeToElement(5)).SetArgDisplayNames("JSON number");
      yield return new TestCaseData(JsonSerializer.SerializeToElement(new { value = 1 })).SetArgDisplayNames("JSON object");
      yield return new TestCaseData(JsonSerializer.SerializeToElement<object>(null)).SetArgDisplayNames("JSON null");
      yield return new TestCaseData(JsonSerializer.SerializeToElement("null")).SetArgDisplayNames("JSON string holding null");
      yield return new TestCaseData(new[] { 1, 2 }).SetArgDisplayNames("int array");
      yield return new TestCaseData(new byte[] { 1, 2 }).SetArgDisplayNames("byte array");
      yield return new TestCaseData("[1,2]").SetArgDisplayNames("string");
      yield return new TestCaseData((object)null).SetArgDisplayNames("null reference");
    }

    /// <summary>
    /// Data in an accepted form whose content the JSON serializer cannot read as an array of whole numbers.
    /// </summary>
    /// <returns>One test case per kind of content.</returns>
    private static IEnumerable<TestCaseData> DataThatCannotBeReadAsNumbers()
    {
      yield return new TestCaseData(JsonSerializer.SerializeToElement("abc")).SetArgDisplayNames("JSON string that is no JSON");
      yield return new TestCaseData(JsonSerializer.SerializeToElement(new object[] { 1, "a" })).SetArgDisplayNames("JSON array with a text");
    }

    /// <summary>
    /// Reads the memory text from the answer of the read-memory tool. The answer is an object of an anonymous
    /// type of the server, so its one property is reached the way a client reaches it: through its JSON form.
    /// </summary>
    /// <param name="result">The answer of the read-memory tool.</param>
    /// <returns>The value of its <c>Memory</c> property.</returns>
    private static string MemoryOf(object result)
    {
      return JsonSerializer.SerializeToElement(result).GetProperty("Memory").GetString();
    }
  }
}
