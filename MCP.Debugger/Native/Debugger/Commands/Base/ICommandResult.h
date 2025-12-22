#pragma once

namespace MCP::Native::Debugger::Commands::Base
{
  class ICommandResult
  {
  public:
    virtual ~ICommandResult() = default;
    virtual bool IsSuccessful() const = 0;
  };

  using CommandResultPtr = std::unique_ptr<ICommandResult>;
}