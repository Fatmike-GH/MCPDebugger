#include "SetBreakpointCommand.h"
#include "CommandResult.h"
#include ".\Base\ICommandContext.h"
#include "..\BreakpointManager.h"
#include <memory>

namespace MCP::Native::Debugger::Commands
{
  void SetBreakpointCommand::Execute(Base::ICommandContext* context)
  {
    auto breakpointManager = context->GetBreakpointManager();
    bool result = breakpointManager->SetBreakpoint(_address);
    SetResult(std::make_unique<CommandResult>(result));
  }
}