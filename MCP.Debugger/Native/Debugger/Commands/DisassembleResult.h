#pragma once
#include ".\Base\ICommandResult.h"
#include "..\..\Disassembler\Instruction.h"
#include <memory>
#include <vector>

namespace MCP::Native::Debugger::Commands
{
  class DisassembleResult : public Base::ICommandResult
  {
  public:
    DisassembleResult(bool success, std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>> instructions = nullptr)
      : _success(success), _instructions(instructions) {
    }

    bool IsSuccessful() const override { return _success; }
    std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>> GetInstructions() { return _instructions; }

  private:
    bool _success;
    std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>> _instructions;
  };
}