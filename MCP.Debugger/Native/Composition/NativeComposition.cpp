#include "NativeComposition.h"
#include "DebuggerSessionFactory.h"
#include "PEFileFactory.h"
#include "..\Debugger\Debugger.h"
#include "..\Debugger\Interfaces\IDebuggerEventHandler.h"
#include "..\PEFile\ExecutableValidator.h"

namespace MCP::Native::Composition
{
  std::unique_ptr<MCP::Native::Debugger::Interfaces::IDebugger> CreateDebugger(
    MCP::Native::Debugger::Interfaces::IDebuggerEventHandler& eventHandler)
  {
    auto peFileFactory = std::make_unique<PEFileFactory>();
    auto executableValidator = std::make_unique<MCP::Native::PEFile::ExecutableValidator>(std::move(peFileFactory));
    auto sessionFactory = std::make_unique<DebuggerSessionFactory>();

    return std::make_unique<MCP::Native::Debugger::Debugger>(std::move(sessionFactory), std::move(executableValidator), eventHandler);
  }
}
