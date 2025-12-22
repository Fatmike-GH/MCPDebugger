#pragma once
#include "CommandType.h"
#include <Windows.h>
#include <future>

namespace MCP::Native::Debugger::Commands::Base
{
  class ICommandResult;
  class ICommandContext;
}

namespace MCP::Native::Debugger::Commands::Base
{
  class ICommand
  {
  public:
    ICommand(CommandType type)
      : _commandType(type),
        _resultPromise(new std::promise<std::unique_ptr<ICommandResult>>())
    {
    }

    virtual ~ICommand() = default;

    virtual void Execute(ICommandContext* context) = 0;

    const CommandType GetType();
    std::future<std::unique_ptr<ICommandResult>> GetFuture();

  protected:
    void SetResult(std::unique_ptr<ICommandResult> result);

  private:
    CommandType _commandType;
    std::unique_ptr<std::promise<std::unique_ptr<ICommandResult>>> _resultPromise;
  };

  using CommandPtr = std::unique_ptr<ICommand>;
}