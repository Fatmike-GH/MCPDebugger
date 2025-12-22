using MCP.Debugger.Types.Disassembler;
using MCP.Debugger.Types.EventArgs;
using MCP.Debugger.Types.Interop;

namespace MCP.Debugger.Tester
{
  public partial class Main : Form
  {
    private MCP.Debugger.DebuggerService _service;

    private uint _processId;
    private uint _threadId;
    private nint _imageBase;
    private nint _entryPoint;

    public Main()
    {
      InitializeComponent();
      _service = new MCP.Debugger.DebuggerService(new MCP.Debugger.UiDispatcher());
      _service.SystemBreakpoint += Service_SystemBreakpoint;
      _service.EntryPointBreakpoint += Service_EntryPointBreakpoint;
      _service.Breakpoint += Service_Breakpoint;
      _service.SingleStep += Service_SingleStep;
      _service.ExitProcess += Service_ExitProcess;
    }

    private void Start_Click(object sender, EventArgs e)
    {
      if (_service.IsRunning()) return;

      string error = string.Empty;
      _processId = 0;
      _threadId = 0;
      _imageBase = 0;
      _entryPoint = 0;
      MCP.Debugger.Types.Interop.CONTEXT context = new Debugger.Types.Interop.CONTEXT();

      if (!_service.Start("Example.exe", ref error, ref _processId, ref _threadId, ref _imageBase, ref _entryPoint, ref context))
      {
        MessageBox.Show($"Error: {error}");
        return;
      }
    }

    private void Stop_Click(object sender, EventArgs e)
    {
      _service.Stop();
    }

    private void StepInto_Click(object sender, EventArgs e)
    {
      _service.StepInto();
    }

    private void StepOver_Click(object sender, EventArgs e)
    {
      _service.StepOver();
    }

    private void Continue_Click(object sender, EventArgs e)
    {
      _service.Continue();
    }

    private void SetBreakpoint_Click(object sender, EventArgs e)
    {
      if (string.IsNullOrEmpty(_breakpoint.Text)) return;
      nint address = nint.Parse(_breakpoint.Text);
      _service.SetBreakpoint(address);
    }

    private async void ContinueUntil_Click(object sender, EventArgs e)
    {
      if (string.IsNullOrEmpty(_breakpoint.Text)) return;
      nint address = nint.Parse(_breakpoint.Text);

      _continueUntil.Enabled = false;

      bool success = await ContinueUntilBreakpointAsync(address);

      _continueUntil.Enabled = true;

      if (success)
      {
        MessageBox.Show($"Temporary breakpoint at 0x{address:X} hit!");
      }
      else
      {
        MessageBox.Show($"ContinueUntilBreakpointAsync failed or timed out.");
      }
    }

    private void GetModules_Click(object sender, EventArgs e)
    {
      var modules = _service.GetModules(_processId);
      int i = 0;
      i++;
    }

    private async Task<bool> ContinueUntilBreakpointAsync(nint va)
    {
      if (!_service.IsRunning() || va == nint.Zero)
      {
        return false;
      }

      var tcs = new TaskCompletionSource<bool>();

      EventHandler<BreakpointEventArgs> temporaryHandler = null;
      temporaryHandler = (sender, e) =>
      {
        if (GetInstructionPointer(e.Context) == va)
        {
          _service.Breakpoint -= temporaryHandler;
          tcs.SetResult(true);
        }
      };

      _service.SetBreakpoint(va);
      _service.Breakpoint += temporaryHandler;

      try
      {
        _service.Continue();
        Task timeoutTask = Task.Delay(30000);
        Task completedTask = await Task.WhenAny(tcs.Task, timeoutTask);

        if (completedTask == timeoutTask)
        {
          // Timeout occurred
          MessageBox.Show("Operation timed out waiting for breakpoint.");
          tcs.SetResult(false); // Cancel the TCS if necessary
          return false;
        }
        return await tcs.Task;
      }
      catch (Exception ex)
      {
        MessageBox.Show($"Operation interrupted: {ex.Message}");
        tcs.TrySetResult(false);
        return false;
      }
      finally
      {
        _service.Breakpoint -= temporaryHandler;
        _service.RemoveBreakpoint(va);
      }
    }

    private void Service_SystemBreakpoint(object sender, BreakpointEventArgs e)
    {
      _currentInstructionPointer.Text = FormatAddress(GetInstructionPointer(e.Context));
      var instruction = Disassemble(GetInstructionPointer(e.Context));
      _disassembly.Text = FormatAddress(instruction.VirtualAdress) + " " + instruction.Disassembly;
    }

    private void Service_EntryPointBreakpoint(object sender, BreakpointEventArgs e)
    {
      _currentInstructionPointer.Text = FormatAddress(GetInstructionPointer(e.Context));
      var instruction = Disassemble(GetInstructionPointer(e.Context));
      _disassembly.Text = FormatAddress(instruction.VirtualAdress) + " " + instruction.Disassembly;
    }

    private void Service_Breakpoint(object sender, BreakpointEventArgs e)
    {
      _currentInstructionPointer.Text = FormatAddress(GetInstructionPointer(e.Context));
      var instruction = Disassemble(GetInstructionPointer(e.Context));
      _disassembly.Text = FormatAddress(instruction.VirtualAdress) + " " + instruction.Disassembly;
    }

    private void Service_SingleStep(object sender, BreakpointEventArgs e)
    {
      _currentInstructionPointer.Text = FormatAddress(GetInstructionPointer(e.Context));
      var instruction = Disassemble(GetInstructionPointer(e.Context));
      _disassembly.Text = FormatAddress(instruction.VirtualAdress) + " " + instruction.Disassembly;
    }

    private void Service_ExitProcess(object sender, ExitProcessEventArgs e)
    {
      MessageBox.Show("Service_ExitProcess");
      //_service.Stop();
    }

    private nint GetInstructionPointer(CONTEXT context)
    {
#if X64
      return (nint)context.Rip;
#else
      return (nint)context.Eip;
#endif
    }

    private string FormatAddress(nint address)
    {
#if X64
      return $"0x{address:X16}";
#else
      return $"0x{address:X8}";
#endif
    }

    public Instruction Disassemble(nint va)
    {
      var result = _service.Disassemble(va, 16);
      return result?.FirstOrDefault();
    }
  }
}
