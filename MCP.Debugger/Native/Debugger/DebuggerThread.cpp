#include "DebuggerThread.h"
#include ".\Interfaces\IDebuggerCommandQueue.h"
#include <memory>
#include <utility>

namespace MCP::Native::Debugger
{
  DebuggerThread::DebuggerThread(std::unique_ptr<Interfaces::IDebugThreadServicesFactory> servicesFactory)
    : _servicesFactory(std::move(servicesFactory))
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
    if (_services == nullptr) return Commands::CommandResult(false);
    if (Is(State::Stopped))
    {
      WaitForDebuggerThread();
      return Commands::CommandResult(false);
    }
    else
    {
      SetState(State::Stopping);

      auto result = _services->GetCommandQueue().Stop();
      WaitForDebuggerThread();

      SetState(State::Stopped);

      return result;
    }    
  }

  Commands::CommandResult DebuggerThread::Continue()
  {
    if (_services == nullptr) return Commands::CommandResult(false);
    auto result = _services->GetCommandQueue().Continue();
    return result;
  }

  Commands::CommandResult DebuggerThread::StepInto()
  {
    if (_services == nullptr) return Commands::CommandResult(false);
    auto result = _services->GetCommandQueue().StepInto();
    return result;
  }

  Commands::CommandResult DebuggerThread::StepOver()
  {
    if (_services == nullptr) return Commands::CommandResult(false);
    auto result = _services->GetCommandQueue().StepOver();
    return result;
  }

  Commands::CommandResult DebuggerThread::SetBreakpoint(DWORD_PTR va)
  {
    if (_services == nullptr) return Commands::CommandResult(false);
    auto result = _services->GetCommandQueue().SetBreakpoint(va);
    return result;
  }

  Commands::CommandResult DebuggerThread::RemoveBreakpoint(DWORD_PTR va)
  {
    if (_services == nullptr) return Commands::CommandResult(false);
    auto result = _services->GetCommandQueue().RemoveBreakpoint(va);
    return result;
  }

  Commands::ReadMemoryResult DebuggerThread::ReadMemory(DWORD_PTR va, size_t size)
  {
    if (_services == nullptr) return Commands::ReadMemoryResult::ReadMemoryResult(false);
    auto result = _services->GetCommandQueue().ReadMemory(va, size);
    return result;
  }

  Commands::CommandResult DebuggerThread::WriteMemory(DWORD_PTR va, const BYTE* data, size_t size)
  {
    if (_services == nullptr) return Commands::CommandResult(false);
    auto result = _services->GetCommandQueue().WriteMemory(va, data, size);
    return result;
  }

  Commands::DisassembleResult DebuggerThread::Disassemble(DWORD_PTR va, size_t length)
  {
    if (_services == nullptr) return Commands::DisassembleResult(false);
    auto result = _services->GetCommandQueue().Disassemble(va, length);
    return result;
  }

  Commands::TryResolveStringResult DebuggerThread::TryResolveString(DWORD_PTR va)
  {
    if (_services == nullptr) return Commands::TryResolveStringResult(false, std::string());
    auto result = _services->GetCommandQueue().TryResolveString(va);
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
    _services.reset();
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

    // Created here, on the debug thread: what serves the debuggee uses the debug API, which belongs to the
    // thread that launched the process.
    _services = _servicesFactory->Create(_processInfo.hProcess, _startupWaiter);

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

    Interfaces::IDebuggerCommandQueue& commandQueue = _services->GetCommandQueue();

    while (!Is(State::Stopping) && !result.ProcessExited)
    {
      if (WaitForDebugEvent(&debugEvent, INFINITE))
      {
        commandQueue.SetCurrentContext(debugEvent.dwProcessId, debugEvent.dwThreadId);
        result = _services->HandleDebugEvent(debugEvent);
        if (!result.ProcessExited)
        {
          commandQueue.ProcessCommandsAsync();
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