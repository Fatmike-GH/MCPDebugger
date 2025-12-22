#include "StopCommand.h"
#include "CommandResult.h"
#include ".\Base\ICommandContext.h"
#include <memory>

namespace MCP::Native::Debugger::Commands
{
  void StopCommand::Execute(Base::ICommandContext* context)
  {
    context->ExecuteClearCommandQueue();
    SetResult(std::make_unique<CommandResult>(true));
  }
}