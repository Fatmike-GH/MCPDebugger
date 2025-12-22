#include "Debugger.h"
#include "DebuggerThread.h"
#include ".\Interfaces\IDebuggerEventHandler.h"
#include ".\DebugEventHandler\DebugEventStorage.h"
#include ".\DebugEventHandler\ProcessInfo.h"
#include "..\ProcessExplorer\ProcessExplorer.h"
#include "..\PEFile\PEFile.h"

namespace MCP::Native::Debugger
{
  Debugger::Debugger(MCP::Native::Debugger::Interfaces::IDebuggerEventHandler* eventHandler)
  {
    _eventHandler = eventHandler;
    _debuggerThread = nullptr;
    _processExplorer = nullptr;
    _debugEventStorage = nullptr;
  }

  Debugger::~Debugger()
  {
    Stop();
  }

  bool Debugger::IsRunning() const
  {
    return _debuggerThread && _debuggerThread->IsRunning();
  }

  bool Debugger::Start(const std::string& path, std::string& error)
  {
    if (IsRunning())
    {
      error = "Debugger is already running.";
      return false;
    }     

    if (!ValidateExecutable(path, error))
    {
      return false;
    }

    Initialize();
    if (!StartDebuggerThread(path))
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
      auto result = _debuggerThread->Stop();
      _debuggerThread.reset();
      return result;
    }
    else
    {
      _debuggerThread.reset();
      return Commands::CommandResult(false);
    }
  }

  Commands::CommandResult Debugger::Continue()
  {
    if (IsRunning())
    {
      return _debuggerThread->Continue();
    }
    return Commands::CommandResult(false);
  }

  Commands::CommandResult Debugger::StepInto()
  {
    if (IsRunning())
    {
      return _debuggerThread->StepInto();
    }
    return Commands::CommandResult(false);
  }

  Commands::CommandResult Debugger::StepOver()
  {
    if (IsRunning())
    {
      return _debuggerThread->StepOver();
    }
    return Commands::CommandResult(false);
  }

  Commands::CommandResult Debugger::SetBreakpoint(DWORD_PTR va)
  {
    if (IsRunning())
    {
      return _debuggerThread->SetBreakpoint(va);
    }
    return Commands::CommandResult(false);
  }

  Commands::CommandResult Debugger::RemoveBreakpoint(DWORD_PTR va)
  {
    if (IsRunning())
    {
      return _debuggerThread->RemoveBreakpoint(va);
    }
    return Commands::CommandResult(false);
  }

  Commands::ReadMemoryResult Debugger::ReadMemory(DWORD_PTR va, size_t size)
  {
    if (IsRunning())
    {
      return _debuggerThread->ReadMemory(va, size);
    }
    return Commands::ReadMemoryResult(false);
  }

  Commands::CommandResult Debugger::WriteMemory(DWORD_PTR va, const BYTE* data, size_t size)
  {
    if (IsRunning())
    {
      return _debuggerThread->WriteMemory(va, data, size);
    }
    return Commands::CommandResult(false);
  }

  Commands::DisassembleResult Debugger::Disassemble(DWORD_PTR va, size_t length)
  {
    if (IsRunning())
    {
      return _debuggerThread->Disassemble(va, length);
    }
    return Commands::DisassembleResult(false);
  }

  Commands::TryResolveStringResult Debugger::TryResolveString(DWORD_PTR va)
  {
    if (IsRunning())
    {
      return _debuggerThread->TryResolveString(va);
    }
    return Commands::TryResolveStringResult(false, std::string());
  }

  PROCESS_INFORMATION Debugger::GetProcessInformation() const
  {
    if (!IsRunning()) return { 0 };
    return _debuggerThread->GetProcessInformation();
  }

  DWORD_PTR Debugger::GetEntryPoint(DWORD processId) const
  {
    if (!IsRunning())
    {
      return { 0 };
    }

    MCP::Native::Debugger::DebugEventHandler::ProcessInfo processInfo = { 0 };
    if (!_debugEventStorage->GetProcessById(processId, processInfo))
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
    if (!_debugEventStorage->GetProcessById(processId, processInfo))
    {
      return { 0 };
    }

    return (DWORD_PTR)processInfo.info.lpBaseOfImage;
  }

  bool Debugger::ValidateExecutable(const std::string& path, std::string& error)
  {
    MCP::Native::PEFile::PEFile peFile(path);

#ifdef _WIN64
    if (!peFile.IsValid() || !peFile.Isx64() || !peFile.IsNative())
    {
      error = "The provided file is not a valid x64 PE file.";
      return false;
    }
#else
    if (!peFile.IsValid() || peFile.Isx64() || !peFile.IsNative())
    {
      error = "The provided file is not a valid x86 PE file.";
      return false;
    }
#endif
    return true;
  }

  void Debugger::Initialize()
  {
    _processExplorer = std::make_unique<MCP::Native::ProcessExplorer::ProcessExplorer>();
    _debugEventStorage = std::make_unique<MCP::Native::Debugger::DebugEventHandler::DebugEventStorage>();
    _debuggerThread = std::make_unique<DebuggerThread>(_processExplorer.get(), _debugEventStorage.get(), _eventHandler);
    
    _debugEventStorage->AddListener(_processExplorer.get());
  }

  bool Debugger::StartDebuggerThread(const std::string& path)
  {
    if (_debuggerThread->Start(path))
    {
      return true;
    }    
    return false;
  }
}