#pragma once
#include ".\Base\ICommandResult.h"

namespace MCP::Native::Debugger::Commands
{
  class CommandResult : public Base::ICommandResult
  {
  public:
    CommandResult(bool success) : _success(success){}

    bool IsSuccessful() const override { return _success; }

  private:
    bool _success;
  };
}