#include "TryResolveStringCommand.h"
#include "TryResolveStringResult.h"
#include ".\Base\ICommandContext.h"
#include <memory>
#include <string>

namespace MCP::Native::Debugger::Commands
{
  void TryResolveStringCommand::Execute(Base::ICommandContext* context)
  {
    std::string resolvedString = context->ExecuteTryResolveString(_address);
    bool success = resolvedString.length() > 0;
    auto result = std::make_unique<Commands::TryResolveStringResult>(success, resolvedString);
    SetResult(std::move(result));
  }
}