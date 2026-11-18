#include "DebuggerSession.h"
#include "..\Debugger\DebuggerThread.h"
#include "..\Debugger\DebugEventHandler\DebugEventStorage.h"
#include "..\Debugger\DebugEventHandler\ProcessInfo.h"
#include "..\ProcessExplorer\ProcessExplorer.h"

namespace MCP::Native::Composition
{
  DebuggerSession::DebuggerSession(std::unique_ptr<MCP::Native::ProcessExplorer::ProcessExplorer> processExplorer,
                                   std::unique_ptr<MCP::Native::Debugger::DebugEventHandler::DebugEventStorage> debugEventStorage,
                                   std::unique_ptr<MCP::Native::Debugger::DebuggerThread> debuggerThread)
    : _processExplorer(std::move(processExplorer)),
      _debugEventStorage(std::move(debugEventStorage)),
      _debuggerThread(std::move(debuggerThread))
  {
  }

  DebuggerSession::~DebuggerSession() = default;

  MCP::Native::Debugger::Interfaces::IDebuggerThread& DebuggerSession::GetDebuggerThread()
  {
    return *_debuggerThread;
  }

  MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer& DebuggerSession::GetProcessExplorer()
  {
    return *_processExplorer;
  }

  bool DebuggerSession::GetProcessById(DWORD processId, MCP::Native::Debugger::DebugEventHandler::ProcessInfo& outInfo)
  {
    return _debugEventStorage->GetProcessById(processId, outInfo);
  }
}
