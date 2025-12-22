#pragma once
#include "ICommand.h"
#include "ICommandResult.h"

namespace MCP::Native::Debugger::Commands::Base
{
  const CommandType ICommand::GetType()
  {
    return _commandType;
  }

  std::future<std::unique_ptr<ICommandResult>> ICommand::GetFuture()
  {
    return _resultPromise->get_future();
  }

  void ICommand::SetResult(std::unique_ptr<ICommandResult> result)
  {
    _resultPromise->set_value(std::move(result));
  }
}