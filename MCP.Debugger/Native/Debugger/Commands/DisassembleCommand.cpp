#include "DisassembleCommand.h"
#include "DisassembleResult.h"
#include ".\Base\ICommandContext.h"
#include <memory>
#include <vector>

namespace MCP::Native::Debugger::Commands
{
  void DisassembleCommand::Execute(Base::ICommandContext* context)
  {
    std::shared_ptr<std::vector<MCP::Native::Disassembler::Instruction>> instructions = nullptr;
    bool success = context->ExecuteDisassemble(_address, _length, instructions);
    auto result = std::make_unique<Commands::DisassembleResult>(success, std::move(instructions));
    SetResult(std::move(result));
  }
}