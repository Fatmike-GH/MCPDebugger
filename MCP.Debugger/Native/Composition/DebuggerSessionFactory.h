#pragma once
#include <memory>
#include "..\Debugger\Interfaces\IDebuggerSessionFactory.h"

namespace MCP::Native::Composition
{
  /// <summary>
  /// Builds the object graph of one debugging session from the classes of this library.
  /// </summary>
  class DebuggerSessionFactory : public MCP::Native::Debugger::Interfaces::IDebuggerSessionFactory
  {
  public:
    std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebuggerSession> CreateSession(
      MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler) override;
  };
}
