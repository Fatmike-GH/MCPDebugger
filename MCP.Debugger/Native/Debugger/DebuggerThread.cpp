#include "DebuggerThread.h"
#include "DebuggerCommandQueue.h"
#include ".\DebugEventHandler\DebugEventHandler.h"
#include ".\Interfaces\IDebuggerEventHandler.h"
#include "..\ProcessExplorer\ProcessExplorer.h"
#include <memory>

namespace MCP::Native::Debugger
{
  DebuggerThread::DebuggerThread(MCP::Native::ProcessExplorer::ProcessExplorer* processExplorer,
                                 MCP::Native::Debugger::DebugEventHandler::DebugEventStorage* debugEventStorage,
                                 Interfaces::IDebuggerEventHandler* eventHandler)
    : _processExplorer(processExplorer),
      _debugEventStorage(debugEventStorage),
      _eventHandler(eventHandler)
  {
    _processInfo = {};
  }

  DebuggerThread::~DebuggerThread()
  {
    Stop();
  }

  bool DebuggerThread::Start(const std::string& path)
  {
    if (!Is(State::Stopped))
    {
      return false;
    }

    SetState(State::Starting);
    ResetMembers();

    if (!LaunchThread(path))
    {
      SetState(State::Stopped);
      return false;
    }

    if (!WaitForInitialization())
    {
      SetState(State::Stopped);
      return false;
    }

    SetState(State::Running);
    return true;
  }

  Commands::CommandResult DebuggerThread::Stop()
  {
    if (_commandQueue == nullptr) return Commands::CommandResult(false);
    if (Is(State::Stopped))
    {
      WaitForDebuggerThread();
      return Commands::CommandResult(false);
    }
    else
    {
      SetState(State::Stopping);

      auto result = _commandQueue->Stop();
      WaitForDebuggerThread();

      SetState(State::Stopped);

      return result;
    }    
  }

  Commands::CommandResult DebuggerThread::Continue()
  {
    if (_commandQueue == nullptr) return Commands::CommandResult(false);
    auto result = _commandQueue->Continue();
    return result;
  }

  Commands::CommandResult DebuggerThread::StepInto()
  {
    if (_commandQueue == nullptr) return Commands::CommandResult(false);
    auto result = _commandQueue->StepInto();
    return result;
  }

  Commands::CommandResult DebuggerThread::StepOver()
  {
    if (_commandQueue == nullptr) return Commands::CommandResult(false);
    auto result = _commandQueue->StepOver();
    return result;
  }

  Commands::CommandResult DebuggerThread::SetBreakpoint(DWORD_PTR va)
  {
    if (_commandQueue == nullptr) return Commands::CommandResult(false);
    auto result = _commandQueue->SetBreakpoint(va);
    return result;
  }

  Commands::CommandResult DebuggerThread::RemoveBreakpoint(DWORD_PTR va)
  {
    if (_commandQueue == nullptr) return Commands::CommandResult(false);
    auto result = _commandQueue->RemoveBreakpoint(va);
    return result;
  }

  Commands::ReadMemoryResult DebuggerThread::ReadMemory(DWORD_PTR va, size_t size)
  {
    if (_commandQueue == nullptr) return Commands::ReadMemoryResult::ReadMemoryResult(false);
    auto result = _commandQueue->ReadMemory(va, size);
    return result;
  }

  Commands::CommandResult DebuggerThread::WriteMemory(DWORD_PTR va, const BYTE* data, size_t size)
  {
    if (_commandQueue == nullptr) return Commands::CommandResult(false);
    auto result = _commandQueue->WriteMemory(va, data, size);
    return result;
  }

  Commands::DisassembleResult DebuggerThread::Disassemble(DWORD_PTR va, size_t length)
  {
    if (_commandQueue == nullptr) return Commands::DisassembleResult(false);
    auto result = _commandQueue->Disassemble(va, length);
    return result;
  }

  Commands::TryResolveStringResult DebuggerThread::TryResolveString(DWORD_PTR va)
  {
    if (_commandQueue == nullptr) return Commands::TryResolveStringResult(false, std::string());
    auto result = _commandQueue->TryResolveString(va);
    return result;
  }

  PROCESS_INFORMATION DebuggerThread::GetProcessInformation()
  {
    if (!IsRunning())
      return { 0 };
    return _processInfo;
  }

  void DebuggerThread::ResetMembers()
  {
    _startupWaiter.Reset();
    _commandQueue.reset();
    _breakpointManager.reset();
    _debugEventHandler.reset();
    _processInfo = {};
  }

  bool DebuggerThread::LaunchThread(const std::string& path)
  {
    try
    {
      _thread = std::thread(&DebuggerThread::ThreadMain, this, path);
      return true;
    }
    catch (...)
    {
      return false;
    }
  }

  bool DebuggerThread::WaitForInitialization()
  {
    bool success = _startupWaiter.Wait();
    if (!success)
    {
      WaitForDebuggerThread();
    }
    return success;
  }

  void DebuggerThread::ThreadMain(std::string path)
  {
    if (!CreateDebugSession(path))
    {
      _startupWaiter.Wakeup(false);
      SetState(State::Stopped);
      return;
    }

    RunDebugLoop();
    CleanupDebugSession();
    SetState(State::Stopped);
  }

  bool DebuggerThread::CreateDebugSession(const std::string& path)
  {
    DebugSetProcessKillOnExit(TRUE);
    if (!Launch(path))
    {
      return false;
    }

    _breakpointManager = std::make_unique<BreakpointManager>(_processInfo.hProcess);
    _commandQueue = std::make_unique<DebuggerCommandQueue>(_processExplorer, _debugEventStorage, _breakpointManager.get());
    _debugEventHandler = std::make_unique<MCP::Native::Debugger::DebugEventHandler::DebugEventHandler>(_debugEventStorage, _processExplorer, _breakpointManager.get(), _eventHandler, &_startupWaiter, _commandQueue.get());

    return true;
  }

  bool DebuggerThread::Launch(const std::string& path)
  {
    _processInfo = {};
    STARTUPINFOA startInfo = {};
    startInfo.cb = sizeof(STARTUPINFOA);

    if (CreateProcessA(path.c_str(), NULL, NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE, NULL, NULL, &startInfo, &_processInfo))
    {
      return true;
    }
    return false;
  }

  void DebuggerThread::RunDebugLoop()
  {
    DEBUG_EVENT debugEvent{};
    MCP::Native::Debugger::DebugEventHandler::HandleResult result = { DBG_CONTINUE, false };

    while (!Is(State::Stopping) && !result.ProcessExited)
    {
      if (WaitForDebugEvent(&debugEvent, INFINITE))
      {
        _commandQueue->SetCurrentContext(debugEvent.dwProcessId, debugEvent.dwThreadId);
        _debugEventHandler->HandleDebugEvent(debugEvent, result);
        if (!result.ProcessExited)
        {
          _commandQueue->ProcessCommandsAsync();
          ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, result.ContinueStatus);
        }
      }
    }
  }

  void DebuggerThread::CleanupDebugSession()
  {
    DebugActiveProcessStop(_processInfo.dwProcessId);
    TerminateProcess(_processInfo.hProcess, 0);
    _processInfo = {};
  }

  void DebuggerThread::WaitForDebuggerThread()
  {
    if (_thread.joinable())
    {
      _thread.join();
    }
  }
}