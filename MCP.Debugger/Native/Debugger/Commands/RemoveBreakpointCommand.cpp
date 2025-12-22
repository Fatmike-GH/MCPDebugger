#include "RemoveBreakpointCommand.h"
#include "CommandResult.h"
#include ".\Base\ICommandContext.h"
#include "..\BreakpointManager.h"
#include <memory>

namespace MCP::Native::Debugger::Commands
{
  void RemoveBreakpointCommand::Execute(Base::ICommandContext* context)
  {
    auto breakpointManager = context->GetBreakpointManager();
    bool result = breakpointManager->RemoveBreakpoint(_address);
    SetResult(std::make_unique<CommandResult>(result));
  }
}