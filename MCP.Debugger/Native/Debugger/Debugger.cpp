#include "Debugger.h"
#include ".\Interfaces\IDebuggerEventHandler.h"
#include ".\Interfaces\IDebuggerSession.h"
#include ".\Interfaces\IDebuggerSessionFactory.h"
#include ".\Interfaces\IDebuggerThread.h"
#include ".\DebugEventHandler\ProcessInfo.h"
#include "..\PEFile\Interfaces\IExecutableValidator.h"
#include "..\ProcessExplorer\Interfaces\IProcessExplorer.h"

namespace MCP::Native::Debugger
{
  Debugger::Debugger(std::unique_ptr<Interfaces::IDebuggerSessionFactory> sessionFactory,
                     std::unique_ptr<MCP::Native::PEFile::Interfaces::IExecutableValidator> executableValidator,
                     Interfaces::IDebuggerEventHandler& eventHandler)
    : _sessionFactory(std::move(sessionFactory)),
      _executableValidator(std::move(executableValidator)),
      _eventHandler(eventHandler)
  {
  }

  Debugger::~Debugger()
  {
    Stop();
  }

  bool Debugger::IsRunning() const
  {
    return _session && _session->GetDebuggerThread().IsRunning();
  }

  bool Debugger::Start(const std::string& path, std::string& error)
  {
    if (IsRunning())
    {
      error = "Debugger is already running.";
      return false;
    }

    if (!_executableValidator->Validate(path, error))
    {
      return false;
    }

    // Replaces the last session: its thread is not running any more and is joined when it is released.
    _session = _sessionFactory->CreateSession(_eventHandler);
    if (!_session->GetDebuggerThread().Start(path))
    {
      error = "The debugger could not be started.";
      return false;
    }
    return true;
  }

  Commands::CommandResult Debugger::Stop()
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().Stop();
    }
    return Commands::CommandResult(false);
  }

  Commands::CommandResult Debugger::Continue()
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().Continue();
    }
    return Commands::CommandResult(false);
  }

  Commands::CommandResult Debugger::StepInto()
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().StepInto();
    }
    return Commands::CommandResult(false);
  }

  Commands::CommandResult Debugger::StepOver()
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().StepOver();
    }
    return Commands::CommandResult(false);
  }

  Commands::CommandResult Debugger::SetBreakpoint(DWORD_PTR va)
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().SetBreakpoint(va);
    }
    return Commands::CommandResult(false);
  }

  Commands::CommandResult Debugger::RemoveBreakpoint(DWORD_PTR va)
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().RemoveBreakpoint(va);
    }
    return Commands::CommandResult(false);
  }

  Commands::ReadMemoryResult Debugger::ReadMemory(DWORD_PTR va, size_t size)
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().ReadMemory(va, size);
    }
    return Commands::ReadMemoryResult(false);
  }

  Commands::CommandResult Debugger::WriteMemory(DWORD_PTR va, const BYTE* data, size_t size)
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().WriteMemory(va, data, size);
    }
    return Commands::CommandResult(false);
  }

  Commands::DisassembleResult Debugger::Disassemble(DWORD_PTR va, size_t length)
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().Disassemble(va, length);
    }
    return Commands::DisassembleResult(false);
  }

  Commands::TryResolveStringResult Debugger::TryResolveString(DWORD_PTR va)
  {
    if (IsRunning())
    {
      return _session->GetDebuggerThread().TryResolveString(va);
    }
    return Commands::TryResolveStringResult(false, std::string());
  }

  PROCESS_INFORMATION Debugger::GetProcessInformation() const
  {
    if (!IsRunning()) return { 0 };
    return _session->GetDebuggerThread().GetProcessInformation();
  }

  DWORD_PTR Debugger::GetEntryPoint(DWORD processId) const
  {
    if (!IsRunning())
    {
      return { 0 };
    }

    MCP::Native::Debugger::DebugEventHandler::ProcessInfo processInfo = { 0 };
    if (!_session->GetProcessById(processId, processInfo))
    {
      return { 0 };
    }

    return (DWORD_PTR)processInfo.info.lpStartAddress;
  }

  DWORD_PTR Debugger::GetImageBase(DWORD processId) const
  {
    if (!IsRunning())
    {
      return { 0 };
    }

    MCP::Native::Debugger::DebugEventHandler::ProcessInfo processInfo = { 0 };
    if (!_session->GetProcessById(processId, processInfo))
    {
      return { 0 };
    }

    return (DWORD_PTR)processInfo.info.lpBaseOfImage;
  }

  MCP::Native::ProcessExplorer::Interfaces::IProcessExplorer* Debugger::GetProcessExplorer() const
  {
    if (!_session)
    {
      return nullptr;
    }
    return &_session->GetProcessExplorer();
  }
}
