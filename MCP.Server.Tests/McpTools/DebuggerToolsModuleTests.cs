using MCP.Debugger.Types.Modules;
using MCP.Server.McpTools.Results;
using MCP.Server.Tests.TestHelpers;
using ModelContextProtocol;
using Moq;

namespace MCP.Server.Tests.McpTools
{
  /// <summary>
  /// The tools that list the modules of a ready session and the imports, exports and sections of one module.
  /// What they hand to the debugger service - a module is asked for by the handle the service reports for its
  /// name - and how its answers and failure values become the tool's answer.
  /// </summary>
  public class DebuggerToolsModuleTests
  {
    /// <summary>The process the tools are asked about.</summary>
    private const uint ProcessId = DebuggerToolsHarness.StartedProcessId;

    /// <summary>The module the tools are asked about.</summary>
    private const string ModuleName = "target.exe";

    /// <summary>The handle the mocked service reports for <see cref="ModuleName"/>.</summary>
    private const nint ModuleHandle = TestAddresses.Sample;

    /// <summary>A module the mocked service does not know: it reports the handle zero for it.</summary>
    private const string UnknownModuleName = "unknown.dll";

    /// <summary>Start of the error text for an unexpected exception; the message of the exception follows.</summary>
    private const string InternalErrorPrefix = "Unexpected internal MCP server error! Internal exception: ";

    /// <summary>Message of the exception a mock throws as an unexpected failure.</summary>
    private const string FailureMessage = "boom";

    /// <summary>The tools on mocked collaborators.</summary>
    private DebuggerToolsHarness _harness;

    /// <summary>
    /// Creates the tools for a test and a session that has reached its entry point. The mocked service knows the
    /// module <see cref="ModuleName"/>.
    /// </summary>
    /// <returns>A task that ends when the session is ready.</returns>
    [SetUp]
    public async Task CreateToolsWithReadySessionAsync()
    {
      _harness = new DebuggerToolsHarness();
      _harness.Service.Setup(service => service.GetRemoteModuleHandle(ProcessId, ModuleName)).Returns(ModuleHandle);
      await _harness.ReachEntryPointAsync();
    }

    [Test]
    public void Debugger_GetModules_ModulesLoaded_ReturnsNameAndBaseAddressOfEachInOrder()
    {
      // Arrange
      var modules = new List<Module>
      {
        new Module(ProcessId, "target.exe", new IntPtr(0x400000), new IntPtr(0x400000), 0x8000),
        new Module(ProcessId, "kernel32.dll", new IntPtr(0x10000000), new IntPtr(0x10000000), 0x9000)
      };
      _harness.Service.Setup(service => service.GetModules(ProcessId)).Returns(modules);

      // Act
      ModulesResult result = _harness.Tools.Debugger_GetModules(ProcessId);

      // Assert
      Assert.That(result.Modules.Select(module => module.ModuleName), Is.EqualTo(new[] { "target.exe", "kernel32.dll" }));
      Assert.That(result.Modules.Select(module => module.BaseAddress), Is.EqualTo(new[] { 0x400000ul, 0x10000000ul }));
    }

    [Test]
    public void Debugger_GetModules_EmptyList_ReturnsNoModules()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetModules(ProcessId)).Returns(new List<Module>());

      // Act
      ModulesResult result = _harness.Tools.Debugger_GetModules(ProcessId);

      // Assert
      Assert.That(result.Modules, Is.Empty);
    }

    [Test]
    public void Debugger_GetModules_NullList_ReturnsNoModules()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetModules(ProcessId)).Returns((IEnumerable<Module>)null);

      // Act
      ModulesResult result = _harness.Tools.Debugger_GetModules(ProcessId);

      // Assert
      Assert.That(result.Modules, Is.Empty);
    }

    [Test]
    public void Debugger_GetModules_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetModules(ProcessId)).Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(() => _harness.Tools.Debugger_GetModules(ProcessId));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    [Test]
    public void Debugger_GetModuleImports_ModuleFound_ReturnsTheImportsOfItsHandleInOrder()
    {
      // Arrange
      var imports = new List<ModuleImport>
      {
        new ModuleImport { ModuleName = "kernel32.dll" },
        new ModuleImport { ModuleName = "user32.dll" }
      };
      _harness.Service.Setup(service => service.GetModuleImports(ProcessId, ModuleHandle)).Returns(imports);

      // Act
      ImportsResult result = _harness.Tools.Debugger_GetModuleImports(ProcessId, ModuleName);

      // Assert
      Assert.That(result.Imports.Select(import => import.ModuleName), Is.EqualTo(new[] { "kernel32.dll", "user32.dll" }));
    }

    [Test]
    public void Debugger_GetModuleImports_ModuleNotFound_AsksWithHandleZeroAndReturnsNoImports()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetModuleImports(ProcessId, nint.Zero)).Returns(new List<ModuleImport>());

      // Act
      ImportsResult result = _harness.Tools.Debugger_GetModuleImports(ProcessId, UnknownModuleName);

      // Assert
      Assert.That(result.Imports, Is.Empty);
      _harness.Service.Verify(service => service.GetModuleImports(ProcessId, nint.Zero), Times.Once());
    }

    [Test]
    public void Debugger_GetModuleImports_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetModuleImports(ProcessId, ModuleHandle))
                      .Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _harness.Tools.Debugger_GetModuleImports(ProcessId, ModuleName));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    [Test]
    public void Debugger_GetModuleExports_ModuleFound_ReturnsTheExportsOfItsHandleInOrder()
    {
      // Arrange
      var exports = new List<ModuleExport>
      {
        new ModuleExport { Name = "First" },
        new ModuleExport { Name = "Second" }
      };
      _harness.Service.Setup(service => service.GetModuleExports(ProcessId, ModuleHandle)).Returns(exports);

      // Act
      ExportsResult result = _harness.Tools.Debugger_GetModuleExports(ProcessId, ModuleName);

      // Assert
      Assert.That(result.Exports.Select(export => export.Name), Is.EqualTo(new[] { "First", "Second" }));
    }

    [Test]
    public void Debugger_GetModuleExports_ModuleNotFound_AsksWithHandleZeroAndReturnsNoExports()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetModuleExports(ProcessId, nint.Zero)).Returns(new List<ModuleExport>());

      // Act
      ExportsResult result = _harness.Tools.Debugger_GetModuleExports(ProcessId, UnknownModuleName);

      // Assert
      Assert.That(result.Exports, Is.Empty);
      _harness.Service.Verify(service => service.GetModuleExports(ProcessId, nint.Zero), Times.Once());
    }

    [Test]
    public void Debugger_GetModuleExports_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetModuleExports(ProcessId, ModuleHandle))
                      .Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _harness.Tools.Debugger_GetModuleExports(ProcessId, ModuleName));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }

    [Test]
    public void Debugger_GetModuleSections_ModuleFound_ReturnsTheSectionsOfItsHandleInOrder()
    {
      // Arrange
      var sections = new List<ModuleSection>
      {
        new ModuleSection { Name = ".text" },
        new ModuleSection { Name = ".data" }
      };
      _harness.Service.Setup(service => service.GetModuleSections(ProcessId, ModuleHandle)).Returns(sections);

      // Act
      SectionsResult result = _harness.Tools.Debugger_GetModuleSections(ProcessId, ModuleName);

      // Assert
      Assert.That(result.Sections.Select(section => section.Name), Is.EqualTo(new[] { ".text", ".data" }));
    }

    [Test]
    public void Debugger_GetModuleSections_ModuleNotFound_AsksWithHandleZeroAndReturnsNoSections()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetModuleSections(ProcessId, nint.Zero)).Returns(new List<ModuleSection>());

      // Act
      SectionsResult result = _harness.Tools.Debugger_GetModuleSections(ProcessId, UnknownModuleName);

      // Assert
      Assert.That(result.Sections, Is.Empty);
      _harness.Service.Verify(service => service.GetModuleSections(ProcessId, nint.Zero), Times.Once());
    }

    [Test]
    public void Debugger_GetModuleSections_ServiceThrows_ThrowsInternalErrorText()
    {
      // Arrange
      _harness.Service.Setup(service => service.GetModuleSections(ProcessId, ModuleHandle))
                      .Throws(new InvalidOperationException(FailureMessage));

      // Act
      McpException caught = Assert.Throws<McpException>(
        () => _harness.Tools.Debugger_GetModuleSections(ProcessId, ModuleName));

      // Assert
      Assert.That(caught.Message, Is.EqualTo(InternalErrorPrefix + FailureMessage));
    }
  }
}
