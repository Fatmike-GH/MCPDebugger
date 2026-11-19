#pragma once
#include <Windows.h>
#include <memory>
#include <vector>
#include <string>
#include "..\..\..\Disassembler\Instruction.h"

namespace MCP::Native::Debugger::Interfaces
{
  class IBreakpointManager;
  class IResumePlan;
}

namespace MCP::Native::Debugger::Commands::Base
{
  class ICommandContext
  {
  public:
    virtual ~ICommandContext() = default;
    virtual MCP::Native::Debugger::Interfaces::IBreakpointManager* GetBreakpointManager() = 0;

    /// <summary>
    /// Resumes the stopped debuggee as the plan says. A debuggee can be resumed only from a stop: the plan is
    /// accepted when the command is executed while the debug thread processes a stop (the debuggee stays
    /// frozen until a command resumes it) - that processing then ends with the plan. It is refused, and
    /// dropped without any effect, when the command is executed between two debug events of a running
    /// debuggee.
    /// </summary>
    /// <returns>True when the plan was accepted.</returns>
    [[nodiscard]] virtual bool Resume(std::unique_ptr<MCP::Native::Debugger::Interfaces::IResumePlan> plan) = 0;

    /// <summary>
    /// Ends the session: the commands that are still queued are discarded, and a stop that is being
    /// processed is not waited on any longer - its processing ends without resuming the debuggee.
    /// </summary>
    virtual void EndSession() = 0;

    virtual bool ExecuteReadMemory(DWORD_PTR address, size_t size, std::shared_ptr<std::vector<BYTE>>& outDataBuffer) = 0;
    virtual bool ExecuteWriteMemory(DWORD_PTR address, const BYTE* data, size_t size) = 0;
    virtual bool ExecuteDisassemble(DWORD_PTR address, size_t length, std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>>& outInstructions) = 0;
    virtual std::string ExecuteTryResolveString(DWORD_PTR address) = 0;
  };
}