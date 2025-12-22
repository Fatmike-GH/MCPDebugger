#pragma once
#include ".\Base\ICommandResult.h"
#include <memory>
#include <vector>

namespace MCP::Native::Debugger::Commands
{
  class ReadMemoryResult : public Base::ICommandResult
  {
  public:
    ReadMemoryResult(bool success, std::shared_ptr<std::vector<uint8_t>> data = nullptr)
      : _success(success), _data(data) {
    }

    bool IsSuccessful() const override { return _success; }
    std::shared_ptr<std::vector<uint8_t>> GetData() { return _data; }

  private:
    bool _success;
    std::shared_ptr<std::vector<uint8_t>> _data;
  };
}