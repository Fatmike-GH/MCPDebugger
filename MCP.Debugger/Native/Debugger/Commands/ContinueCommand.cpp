#include "ContinueCommand.h"
#include "CommandResult.h"
#include "ContinuePlan.h"
#include ".\Base\ICommandContext.h"
#include <memory>

namespace MCP::Native::Debugger::Commands
{
  void ContinueCommand::Execute(Base::ICommandContext* context)
  {
    // A debuggee that runs already cannot be resumed: the context accepts the plan only at a stop
    bool accepted = context->Resume(std::make_unique<ContinuePlan>());
    SetResult(std::make_unique<CommandResult>(accepted));
  }
}
