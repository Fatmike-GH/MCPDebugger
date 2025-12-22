#include "StepIntoCommand.h"
#include "CommandResult.h"
#include ".\Base\ICommandContext.h"
#include <memory>
#include <vector>

namespace MCP::Native::Debugger::Commands
{
  void StepIntoCommand::Execute(Base::ICommandContext* context)
  {
    auto result = std::make_unique<Commands::CommandResult>(true);
    SetResult(std::move(result));
  }
}