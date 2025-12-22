#pragma once
#include ".\Base\ICommandResult.h"
#include "..\..\Disassembler\Instruction.h"
#include <memory>
#include <string>

namespace MCP::Native::Debugger::Commands
{
  class TryResolveStringResult : public Base::ICommandResult
  {
  public:
    TryResolveStringResult(bool success, std::string result)
      : _success(success), _result(result) {
    }

    bool IsSuccessful() const override { return _success; }
    std::string GetResult() { return _result; }

  private:
    bool _success;
    std::string _result;
  };
}