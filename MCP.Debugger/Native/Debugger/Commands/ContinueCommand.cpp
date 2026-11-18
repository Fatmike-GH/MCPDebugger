#include "ContinueCommand.h"
#include "CommandResult.h"
#include ".\Base\ICommandContext.h"
#include <memory>

namespace MCP::Native::Debugger::Commands
{
  void ContinueCommand::Execute(Base::ICommandContext* context)
  {
    // A debuggee that runs already cannot be resumed: the command succeeds only at a stop
    auto result = std::make_unique<Commands::CommandResult>(context->IsDebuggeeStopped());
    SetResult(std::move(result));
  }
}
