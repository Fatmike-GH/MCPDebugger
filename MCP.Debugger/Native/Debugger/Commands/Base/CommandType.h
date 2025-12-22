#pragma once

namespace MCP::Native::Debugger::Commands::Base
{
  enum class CommandType
  {
    CMD_NONE,
    CMD_STOP,
    CMD_CONTINUE,
    CMD_STEP_INTO,
    CMD_STEP_OVER,
    CMD_SET_BREAKPOINT,
    CMD_REMOVE_BREAKPOINT,
    CMD_READ_MEMORY,
    CMD_WRITE_MEMORY,
    CMD_DISASSEMBLE,
    CMD_TRY_RESOLVE_STRING
  };
}