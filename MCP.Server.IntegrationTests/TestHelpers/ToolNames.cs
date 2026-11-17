namespace MCP.Server.IntegrationTests.TestHelpers
{
  /// <summary>The names of the tools the server offers, as a client has to write them.</summary>
  public static class ToolNames
  {
    /// <summary>Starts a debugging session.</summary>
    public const string Start = "Debugger_Start";

    /// <summary>Ends the debugging session.</summary>
    public const string Stop = "Debugger_Stop";

    /// <summary>Continues without waiting for a stop.</summary>
    public const string Continue = "Debugger_Continue";

    /// <summary>Continues from the system breakpoint to the entry point.</summary>
    public const string ContinueToEntryPoint = "Debugger_ContinueToEntryPoint";

    /// <summary>Continues to a temporary breakpoint at an address.</summary>
    public const string ContinueToBreakpoint = "Debugger_ContinueToBreakpoint";

    /// <summary>Continues to a temporary breakpoint on an exported function.</summary>
    public const string ContinueToApiBreakpoint = "Debugger_ContinueToApiBreakpoint";

    /// <summary>Executes one instruction, entering a call.</summary>
    public const string StepInto = "Debugger_StepInto";

    /// <summary>Executes one instruction, running through a call.</summary>
    public const string StepOver = "Debugger_StepOver";

    /// <summary>Disassembles memory of the debuggee.</summary>
    public const string Disassemble = "Debugger_Disassemble";

    /// <summary>Reads a string from the memory of the debuggee.</summary>
    public const string TryResolveString = "Debugger_TryResolveString";

    /// <summary>Reads memory of the debuggee.</summary>
    public const string ReadMemory = "Debugger_ReadMemory";

    /// <summary>Writes memory of the debuggee.</summary>
    public const string WriteMemory = "Debugger_WriteMemory";

    /// <summary>Lists the modules of the debuggee.</summary>
    public const string GetModules = "Debugger_GetModules";

    /// <summary>Lists the imports of a module.</summary>
    public const string GetModuleImports = "Debugger_GetModuleImports";

    /// <summary>Lists the exports of a module.</summary>
    public const string GetModuleExports = "Debugger_GetModuleExports";

    /// <summary>Lists the sections of a module.</summary>
    public const string GetModuleSections = "Debugger_GetModuleSections";

    /// <summary>Converts a hexadecimal number to decimal.</summary>
    public const string HexToDecConverter = "Debugger_HexToDecConverter";

    /// <summary>All tools of the server.</summary>
    public static IReadOnlyList<string> All { get; } = new[]
    {
      Start, Stop, Continue, ContinueToEntryPoint, ContinueToBreakpoint, ContinueToApiBreakpoint, StepInto,
      StepOver, Disassemble, TryResolveString, ReadMemory, WriteMemory, GetModules, GetModuleImports,
      GetModuleExports, GetModuleSections, HexToDecConverter,
    };
  }
}
