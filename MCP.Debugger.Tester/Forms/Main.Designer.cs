namespace MCP.Debugger.Tester
{
  partial class Main
  {
    /// <summary>
    ///  Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    ///  Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
      {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    ///  Required method for Designer support - do not modify
    ///  the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      _start = new Button();
      _stop = new Button();
      _stepInto = new Button();
      _continue = new Button();
      _setBreakpoint = new Button();
      _breakpoint = new TextBox();
      _getModules = new Button();
      _continueUntil = new Button();
      _stepOver = new Button();
      _currentInstructionPointer = new TextBox();
      _disassembly = new TextBox();
      SuspendLayout();
      // 
      // _start
      // 
      _start.Location = new Point(12, 12);
      _start.Name = "_start";
      _start.Size = new Size(75, 23);
      _start.TabIndex = 0;
      _start.Text = "Start";
      _start.UseVisualStyleBackColor = true;
      _start.Click += Start_Click;
      // 
      // _stop
      // 
      _stop.Location = new Point(12, 41);
      _stop.Name = "_stop";
      _stop.Size = new Size(75, 23);
      _stop.TabIndex = 1;
      _stop.Text = "Stop";
      _stop.UseVisualStyleBackColor = true;
      _stop.Click += Stop_Click;
      // 
      // _stepInto
      // 
      _stepInto.Location = new Point(93, 12);
      _stepInto.Name = "_stepInto";
      _stepInto.Size = new Size(75, 23);
      _stepInto.TabIndex = 2;
      _stepInto.Text = "Step Into";
      _stepInto.UseVisualStyleBackColor = true;
      _stepInto.Click += StepInto_Click;
      // 
      // _continue
      // 
      _continue.Location = new Point(174, 12);
      _continue.Name = "_continue";
      _continue.Size = new Size(75, 23);
      _continue.TabIndex = 3;
      _continue.Text = "Continue";
      _continue.UseVisualStyleBackColor = true;
      _continue.Click += Continue_Click;
      // 
      // _setBreakpoint
      // 
      _setBreakpoint.Location = new Point(12, 84);
      _setBreakpoint.Name = "_setBreakpoint";
      _setBreakpoint.Size = new Size(100, 23);
      _setBreakpoint.TabIndex = 4;
      _setBreakpoint.Text = "Set Breakpoint";
      _setBreakpoint.UseVisualStyleBackColor = true;
      _setBreakpoint.Click += SetBreakpoint_Click;
      // 
      // _breakpoint
      // 
      _breakpoint.Location = new Point(118, 85);
      _breakpoint.Name = "_breakpoint";
      _breakpoint.Size = new Size(162, 23);
      _breakpoint.TabIndex = 5;
      _breakpoint.Text = "4230187";
      // 
      // _getModules
      // 
      _getModules.Location = new Point(12, 142);
      _getModules.Name = "_getModules";
      _getModules.Size = new Size(100, 23);
      _getModules.TabIndex = 6;
      _getModules.Text = "GetModules";
      _getModules.UseVisualStyleBackColor = true;
      _getModules.Click += GetModules_Click;
      // 
      // _continueUntil
      // 
      _continueUntil.Location = new Point(12, 113);
      _continueUntil.Name = "_continueUntil";
      _continueUntil.Size = new Size(100, 23);
      _continueUntil.TabIndex = 7;
      _continueUntil.Text = "Continue Until";
      _continueUntil.UseVisualStyleBackColor = true;
      _continueUntil.Click += ContinueUntil_Click;
      // 
      // _stepOver
      // 
      _stepOver.Location = new Point(93, 41);
      _stepOver.Name = "_stepOver";
      _stepOver.Size = new Size(75, 23);
      _stepOver.TabIndex = 8;
      _stepOver.Text = "Step Over";
      _stepOver.UseVisualStyleBackColor = true;
      _stepOver.Click += StepOver_Click;
      // 
      // _currentInstructionPointer
      // 
      _currentInstructionPointer.Location = new Point(255, 12);
      _currentInstructionPointer.Name = "_currentInstructionPointer";
      _currentInstructionPointer.Size = new Size(162, 23);
      _currentInstructionPointer.TabIndex = 9;
      // 
      // _disassembly
      // 
      _disassembly.Location = new Point(423, 13);
      _disassembly.Name = "_disassembly";
      _disassembly.Size = new Size(329, 23);
      _disassembly.TabIndex = 10;
      // 
      // Main
      // 
      AutoScaleDimensions = new SizeF(7F, 15F);
      AutoScaleMode = AutoScaleMode.Font;
      ClientSize = new Size(761, 182);
      Controls.Add(_disassembly);
      Controls.Add(_currentInstructionPointer);
      Controls.Add(_stepOver);
      Controls.Add(_continueUntil);
      Controls.Add(_getModules);
      Controls.Add(_breakpoint);
      Controls.Add(_setBreakpoint);
      Controls.Add(_continue);
      Controls.Add(_stepInto);
      Controls.Add(_stop);
      Controls.Add(_start);
      Name = "Main";
      Text = "MCP.Debugger.Tester";
      ResumeLayout(false);
      PerformLayout();
    }

    #endregion

    private Button _start;
    private Button _stop;
    private Button _stepInto;
    private Button _continue;
    private Button _setBreakpoint;
    private TextBox _breakpoint;
    private Button _getModules;
    private Button _continueUntil;
    private Button _stepOver;
    private TextBox _currentInstructionPointer;
    private TextBox _disassembly;
  }
}
