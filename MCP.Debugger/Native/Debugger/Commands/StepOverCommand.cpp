#include "StepOverCommand.h"
#include "CommandResult.h"
#include ".\Base\ICommandContext.h"
#include <memory>
#include <vector>

namespace MCP::Native::Debugger::Commands
{
  void StepOverCommand::Execute(Base::ICommandContext* context)
  {
    // A step starts from a stop: while the debuggee runs there is no instruction to step from
    auto result = std::make_unique<Commands::CommandResult>(context->IsDebuggeeStopped());
    SetResult(std::move(result));
  }
}