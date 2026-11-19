#include "StepOverCommand.h"
#include "CommandResult.h"
#include "StepOverPlan.h"
#include ".\Base\ICommandContext.h"
#include <memory>

namespace MCP::Native::Debugger::Commands
{
  void StepOverCommand::Execute(Base::ICommandContext* context)
  {
    // A step starts from a stop: while the debuggee runs there is no instruction to step from, and the
    // context refuses the plan
    bool accepted = context->Resume(std::make_unique<StepOverPlan>());
    SetResult(std::make_unique<CommandResult>(accepted));
  }
}
