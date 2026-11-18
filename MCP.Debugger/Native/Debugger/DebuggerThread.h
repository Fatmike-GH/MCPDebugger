#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include ".\Interfaces\IDebuggerThread.h"
#include ".\Interfaces\IDebugThreadServices.h"
#include ".\Interfaces\IDebugThreadServicesFactory.h"
#include ".\Commands\Base\ICommand.h"
#include ".\Commands\CommandResult.h"
#include ".\Commands\DisassembleResult.h"
#include ".\Commands\ReadMemoryResult.h"
#include ".\Commands\TryResolveStringResult.h"
#include ".\DebugEventHandler\HandleResult.h"
#include "..\Threading\EventWaiter.h"

namespace MCP::Native::Debugger
{
  class DebuggerThread : public Interfaces::IDebuggerThread
  {
  public:
    /// <summary>
    /// Takes ownership of the factory. The thread asks it on the debug thread, after every launch, for the
    /// objects that serve the launched debuggee.
    /// </summary>
    explicit DebuggerThread(std::unique_ptr<Interfaces::IDebugThreadServicesFactory> servicesFactory);
    ~DebuggerThread() override;

    bool Start(const std::string& path) override;
    
    Commands::CommandResult Stop() override;
    Commands::CommandResult Continue() override;
    Commands::CommandResult StepInto() override;
    Commands::CommandResult StepOver() override;
    Commands::CommandResult SetBreakpoint(DWORD_PTR va) override;
    Commands::CommandResult RemoveBreakpoint(DWORD_PTR va) override;
    Commands::ReadMemoryResult ReadMemory(DWORD_PTR va, size_t size) override;
    Commands::CommandResult WriteMemory(DWORD_PTR va, const BYTE* data, size_t size) override;
    Commands::DisassembleResult Disassemble(DWORD_PTR va, size_t length) override;
    Commands::TryResolveStringResult TryResolveString(DWORD_PTR va) override;

    PROCESS_INFORMATION GetProcessInformation() override;
    bool IsRunning() const override { return Is(State::Running); }  

  private:
    enum class State
    {
      Stopped,
      Starting,
      Running,
      Stopping
    };

    void ResetMembers();
    bool LaunchThread(const std::string& path);
    bool WaitForInitialization();
    void WaitForDebuggerThread();

    void ThreadMain(std::string path);
    bool CreateDebugSession(const std::string& path);
    bool Launch(const std::string& path);
    void RunDebugLoop();
    void CleanupDebugSession();

    void SetState(State s) { _state.store(s, std::memory_order_release); }
    State GetState() const { return _state.load(std::memory_order_acquire); }
    bool Is(State s) const { return GetState() == s; }

  private:
    std::atomic<State> _state{ State::Stopped };

    std::thread _thread;
    MCP::Native::Threading::EventWaiter _startupWaiter;

    std::unique_ptr<Interfaces::IDebugThreadServicesFactory> _servicesFactory;
    PROCESS_INFORMATION _processInfo;

    // Created on the debug thread after the launch, released on a client thread once the debug thread ended.
    // Declared behind the startup waiter: the services borrow it and must be destroyed first.
    std::unique_ptr<Interfaces::IDebugThreadServices> _services;
  };
}