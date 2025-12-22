using MCP.Debugger.Types.Interop;
using MCP.Server.Base;
using MCP.Server.McpTools.Converters;
using MCP.Server.McpTools.Extensions;
using MCP.Server.McpTools.Results;
using ModelContextProtocol;
using ModelContextProtocol.Server;
using System.ComponentModel;
using System.Globalization;
using System.Text.Json;

namespace MCP.Server.McpTools
{
  [McpServerToolType]
  public class DebuggerTools : McpToolBase
  {
    private MCP.Debugger.DebuggerService _debuggerService;
    private JsonSerializerOptions _jsonSerializerOptions;
    public DebuggerTools()
    {
      _debuggerService = new MCP.Debugger.DebuggerService(new MCP.Debugger.ConsoleDispatcher());
      _debuggerService.ExitProcess += DebuggerService_ExitProcess;

      _jsonSerializerOptions = new JsonSerializerOptions()
      {
        IncludeFields = true,
        WriteIndented = false,
        Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
      };
      _jsonSerializerOptions.Converters.Add(new IntPtrConverter());
      _jsonSerializerOptions.Converters.Add(new ByteArrayAsNumbersConverter());

      this.Started = false;
      this.Initialized = false;
    }

    private bool Started { get; set; }
    private bool Initialized { get; set; }

    [McpServerTool(Name = "Debugger_Start"), Description("Start the debugger. It will automatically break on system breakpoint.")]
    public DebuggerStartResult Debugger_Start([Description("Full path to the debuggee executable.")] string path)
    {
      return Execute(() =>
      {
        ThrowIfStarted();

        string error = string.Empty;
        uint processId = 0;
        uint threadId = 0;
        nint imageBase = 0;
        nint entryPoint = 0;
        CONTEXT context = new CONTEXT();

        if (!_debuggerService.Start(path, ref error, ref processId, ref threadId, ref imageBase, ref entryPoint, ref context))
        {
          throw new McpException($"Debugger_Start failed. Error: {error}");
        }

        var instruction = _debuggerService.Disassemble(_debuggerService.GetInstructionPointer(context));

        this.Started = true;
        return new DebuggerStartResult(processId, threadId, (ulong)imageBase, (ulong)entryPoint, context, instruction);
      });
    }

    [McpServerTool(Name = "Debugger_Stop"), Description("Stops the debugger and closes the debuggee application.")]
    public void Debugger_Stop()
    {
      Execute(() =>
      {
        ThrowIfNotStarted();

        _debuggerService.Stop();

        this.Started = false;
        this.Initialized = false;
      });
    }

    [McpServerTool(Name = "Debugger_Continue"), Description("Continues to run the debuggee if it is currently waiting on a breakpoint.")]
    public void Debugger_Continue()
    {
      Execute(() =>
      {
        ThrowIfNotReady();
        _debuggerService.Continue();
      });
    }

    [McpServerTool(Name = "Debugger_ContinueToEntryPoint"), Description("Continues to run the debuggee from system breakpoint to the entry point of the debuggee. Returns current context.")]
    public async Task<BreakpointResult> Debugger_ContinueToEntryPoint()
    {
      return await Execute(async () =>
      {
        ThrowIfNotStarted();
        ThrowIfInitialized();
        this.Initialized = true;
        CONTEXT context = await _debuggerService.ContinueToEntrypointAsync();
        var instruction = _debuggerService.Disassemble(_debuggerService.GetInstructionPointer(context));
        return new BreakpointResult(context, instruction);
      });
    }

    [McpServerTool(Name = "Debugger_ContinueToBreakpoint"), Description("Continues to run the debuggee until the specified temporary breakpoint is reached. Returns current context.")]
    public async Task<BreakpointResult> Debugger_ContinueToBreakpoint([Description("Absolute virtual address of the temporary breakpoint.")] ulong va)
    {
      return await Execute(async () =>
      {
        ThrowIfNotReady();
        CONTEXT context = await _debuggerService.ContinueToBreakpointAsync((nint)va);
        var instruction = _debuggerService.Disassemble(_debuggerService.GetInstructionPointer(context));
        return new BreakpointResult(context, instruction);
      });
    }

    [McpServerTool(Name = "Debugger_ContinueToApiBreakpoint"), Description("Continues to run the debuggee until the specified temporary API breakpoint is reached. Returns current context.")]
    public async Task<BreakpointResult> Debugger_ContinueToApiBreakpoint([Description("The process id of the debuggee.")] uint processId,
                                                                         [Description("Name of the module.")] string moduleName,
                                                                         [Description("Name of the function of the module for the temporary breakpoint.")] string functionName)
    {
      return await Execute(async () =>
      {
        ThrowIfNotReady();
        var moduleHandle = _debuggerService.GetRemoteModuleHandle(processId, moduleName);
        var address = _debuggerService.GetRemoteProcAddress(processId, moduleHandle, functionName);
        CONTEXT context = await _debuggerService.ContinueToBreakpointAsync(address);
        var instruction = _debuggerService.Disassemble(_debuggerService.GetInstructionPointer(context));
        return new BreakpointResult(context, instruction);
      });
    }

    [McpServerTool(Name = "Debugger_StepInto"), Description("Steps from the current instruction to the next instruction. If the current instruction is a call it will step into this call.")]
    public async Task<BreakpointResult> Debugger_StepInto()
    {
      return await Execute(async () =>
      {
        ThrowIfNotReady();
        CONTEXT context = await _debuggerService.StepIntoAsync();
        var instruction = _debuggerService.Disassemble(_debuggerService.GetInstructionPointer(context));
        return new BreakpointResult(context, instruction);
      });
    }

    [McpServerTool(Name = "Debugger_StepOver"), Description("Steps from the current instruction to the next instruction. If the current instruction is a call it will step over the call to the instruction behind it.")]
    public async Task<BreakpointResult> Debugger_StepOver()
    {
      return await Execute(async () =>
      {
        ThrowIfNotReady();
        CONTEXT context = await _debuggerService.StepOverAsync();
        var instruction = _debuggerService.Disassemble(_debuggerService.GetInstructionPointer(context));
        return new BreakpointResult(context, instruction);
      });
    }

    [McpServerTool(Name = "Debugger_Disassemble"), Description("Disassembles memory of the debuggee process and returns the disassembly including symbols.")]
    public DisassemblerResult Debugger_Disassemble([Description("Absolute virtual address of the memory. Must be aligned with an actual instruction, otherwise the disassembly will be corrupted!")] ulong va,
                                                   [Description("Length of memory to be disassembled in bytes.")] uint length)
    {
      var result = Execute(() =>
      {
        ThrowIfNotReady();
        return _debuggerService.Disassemble((nint)va, length);
      });

      return new DisassemblerResult(result);
    }

    [McpServerTool(Name = "Debugger_TryResolveString"), Description("Tries to read a string from the memory of the debuggee process at a given absolute virtual address.")]
    public string Debugger_TryResolveString([Description("Absolute virtual address of the memory where you believe a string is located.")] ulong va)
    {
      var result = Execute(() =>
      {
        ThrowIfNotReady();
        return _debuggerService.TryResolveString((nint)va);
      });

      return result;
    }

    [McpServerTool(Name = "Debugger_ReadMemory"), Description("Reads memory of the debuggee process and returns the bytes.")]
    public object Debugger_ReadMemory([Description("Absolute virtual address of the memory.")] ulong va,
                                      [Description("Size to be read in bytes.")] uint size)
    {
      byte[] result = Execute(() =>
      {
        ThrowIfNotReady();
        return _debuggerService.ReadMemory((nint)va, size);
      });

      return new { Memory = JsonSerializer.Serialize(result, _jsonSerializerOptions) };
    }

    [McpServerTool(Name = "Debugger_WriteMemory"), Description("Writes the given data into the memory of the debuggee process. Expecting an array of bytes (0-255) for data parameter.")]
    public bool Debugger_WriteMemory([Description("Absolute virtual address of the memory.")] ulong va,
                                     [Description("Expecting an array of bytes (0-255).")] object data)
    {
      return Execute(() =>
      {
        ThrowIfNotReady();
        if (data is JsonElement element)
        {
          int[] numbers = null;
          if (element.ValueKind == JsonValueKind.String)
          {
            string jsonString = element.GetString();
            numbers = JsonSerializer.Deserialize<int[]>(jsonString);
          }
          else if (element.ValueKind == JsonValueKind.Array)
          {
            numbers = JsonSerializer.Deserialize<int[]>(element.GetRawText());
          }
          if (numbers != null)
          {
            byte[] bytesToWrite = numbers.Select(e => (byte)e).ToArray();
            return _debuggerService.WriteMemory((nint)va, bytesToWrite);
          }
        }
        throw new McpException("data parameter must be a string containing a JSON array or a direct JSON array.");
      });
    }

    [McpServerTool(Name = "Debugger_GetModules"), Description("Returns the currently loaded modules of the given process id. Note that the loaded modules can change during runtime.")]
    public ModulesResult Debugger_GetModules([Description("The process id of the debuggee.")] uint processId)
    {
      var result = Execute(() =>
      {
        ThrowIfNotReady();
        return _debuggerService.GetModules(processId);
      });

      return new ModulesResult(result);
    }

    [McpServerTool(Name = "Debugger_GetModuleImports"), Description("Returns the imports of the given module.")]
    public ImportsResult Debugger_GetModuleImports([Description("The process id of the debuggee.")] uint processId,
                                                   [Description("The name of the module.")] string moduleName)
    {
      var result = Execute(() =>
      {
        ThrowIfNotReady();
        var moduleHandle = _debuggerService.GetRemoteModuleHandle(processId, moduleName);
        return _debuggerService.GetModuleImports(processId, (nint)moduleHandle);
      });

      return new ImportsResult(result);
    }

    [McpServerTool(Name = "Debugger_GetModuleExports"), Description("Returns the exports of the given module.")]
    public ExportsResult Debugger_GetModuleExports([Description("The process id of the debuggee.")] uint processId,
                                                   [Description("The name of the module.")] string moduleName)
    {
      var result = Execute(() =>
      {
        ThrowIfNotReady();
        var moduleHandle = _debuggerService.GetRemoteModuleHandle(processId, moduleName);
        return _debuggerService.GetModuleExports(processId, (nint)moduleHandle);
      });

      return new ExportsResult(result);
    }

    [McpServerTool(Name = "Debugger_GetModuleSections"), Description("Returns the sections of the given module.")]
    public SectionsResult Debugger_GetModuleSections([Description("The process id of the debuggee.")] uint processId,
                                                     [Description("The name of the module.")] string moduleName)
    {
      var result = Execute(() =>
      {
        ThrowIfNotReady();
        var moduleHandle = _debuggerService.GetRemoteModuleHandle(processId, moduleName);
        return _debuggerService.GetModuleSections(processId, (nint)moduleHandle);
      });

      return new SectionsResult(result);
    }

    [McpServerTool(Name = "Debugger_HexToDecConverter"), Description("Converts a given hex value to dec. It is mandatory to call this function to avoid number conversion errors.")]
    public ulong Debugger_HexToDecConverter([Description("A number in hex.")] string hexNumber)
    {
      return Execute(() =>
      {
        if (hexNumber == null)
        {
          throw new McpException("Parameter cannot be null.");
        }
        return ulong.Parse(hexNumber, NumberStyles.HexNumber, CultureInfo.InvariantCulture);
      });
    }

    private void ThrowIfStarted()
    {
      if (this.Started)
      {
        throw new McpException("Debugger already started.");
      }
    }

    private void ThrowIfNotStarted()
    {
      if (!this.Started)
      {
        throw new McpException("Debugger not yet started.");
      }
    }

    private void ThrowIfInitialized()
    {
      if (this.Initialized)
      {
        throw new McpException("Entry point already reached.");
      }
    }

    private void ThrowIfNotInitialized()
    {
      if (!this.Initialized)
      {
        throw new McpException("Entry point not yet reached.");
      }
    }

    private void ThrowIfNotReady()
    {
      ThrowIfNotStarted();
      ThrowIfNotInitialized();
    }

    private void DebuggerService_ExitProcess(object sender, Debugger.Types.EventArgs.ExitProcessEventArgs e)
    {
      this.Started = false;
      this.Initialized = false;
    }
  }
}
