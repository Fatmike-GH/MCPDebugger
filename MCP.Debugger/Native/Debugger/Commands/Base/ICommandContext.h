#pragma once
#include <Windows.h>
#include <memory>
#include <vector>
#include <string>
#include "..\..\..\Disassembler\Instruction.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpointManager;
}

namespace MCP::Native::Debugger::Commands::Base
{
  class ICommandContext
  {
  public:
    virtual ~ICommandContext() = default;
    virtual MCP::Native::Debugger::Interfaces::IBreakpointManager* GetBreakpointManager() = 0;

    /// <summary>
    /// Tells whether the debuggee stands at a stop: true while the debug thread executes the commands of a stop
    /// (the debuggee stays frozen until one of them resumes it), false while it executes commands between two
    /// debug events of a running debuggee. A command that resumes the debuggee can take effect only at a stop.
    /// </summary>
    virtual bool IsDebuggeeStopped() = 0;

    virtual void ExecuteClearCommandQueue() = 0;
    virtual bool ExecuteReadMemory(DWORD_PTR address, size_t size, std::shared_ptr<std::vector<BYTE>>& outDataBuffer) = 0;
    virtual bool ExecuteWriteMemory(DWORD_PTR address, const BYTE* data, size_t size) = 0;
    virtual bool ExecuteDisassemble(DWORD_PTR address, size_t length, std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>>& outInstructions) = 0;
    virtual std::string ExecuteTryResolveString(DWORD_PTR address) = 0;
  };
}