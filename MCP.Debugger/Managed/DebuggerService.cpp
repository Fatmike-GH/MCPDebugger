#include <Windows.h>
#include <vcclr.h>
#include <malloc.h>
#include <msclr/marshal_cppstd.h>
#include <msclr/lock.h>

#include "DebuggerService.h"
#include ".\Interfaces\IDispatcher.h"
#include "..\Native\Debugger\Debugger.h"
#include "..\Adapters\DebuggerEventHandlerAdapter.h"

#include "..\Native\ProcessExplorer\ProcessExplorer.h"
#include "..\Native\ProcessExplorer\ModuleInfo.h"

using namespace System;
using namespace System::Runtime::InteropServices;

namespace MCP::Debugger
{
  DebuggerService::DebuggerService(IDispatcher^ dispatcher)
  {
    _adapter = new MCP::Adapters::DebuggerEventHandlerAdapter(this);
    _debugger = new MCP::Native::Debugger::Debugger(_adapter);
    _dispatcher = dispatcher;
  }

  DebuggerService::~DebuggerService() // Dispose: Clean up managed resources
  {
    _dispatcher->Shutdown();
    this->!DebuggerService();
  }

  DebuggerService::!DebuggerService() // Finalizer: Clean up unmanaged resources
  {
    if (_debugger != nullptr)
    {
      delete _debugger;
      _debugger = nullptr;
    }
    if (_adapter != nullptr)
    {
      delete _adapter;
      _adapter = nullptr;
    }
  }

  bool DebuggerService::Start(String^ path, String^% error, System::UInt32% processId, System::UInt32% threadid, System::IntPtr% imageBase, System::IntPtr% entryPoint, MCP::Debugger::Types::Interop::CONTEXT% context)
  {
    std::string nPath = msclr::interop::marshal_as<std::string>(path);
    std::string nError;

    if (!_debugger->Start(nPath, nError))
    {
      error = msclr::interop::marshal_as<System::String^>(nError);
      return false;
    }

    PROCESS_INFORMATION processInformation = _debugger->GetProcessInformation();
    processId = processInformation.dwProcessId;
    threadid = processInformation.dwThreadId;
    imageBase = System::IntPtr(reinterpret_cast<void*>(_debugger->GetImageBase(processInformation.dwProcessId)));
    entryPoint = System::IntPtr(reinterpret_cast<void*>(_debugger->GetEntryPoint(processInformation.dwProcessId)));
    context = GetThreadContext(processInformation.hThread);

    return true;
  }

  void DebuggerService::Stop()
  {
    _debugger->Stop();
  }

  bool DebuggerService::IsRunning()
  {
    return _debugger->IsRunning();
  }

  void DebuggerService::Continue()
  {
    _debugger->Continue();
  }

  bool DebuggerService::StepInto()
  {
    return NativeStepInto(_debugger);
  }

  bool DebuggerService::StepOver()
  {
    return NativeStepOver(_debugger);
  }

  void DebuggerService::SetBreakpoint(System::IntPtr va)
  {
#ifdef _WIN64
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt64());
#else
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt32());
#endif
    _debugger->SetBreakpoint(nVA);
  }

  void DebuggerService::RemoveBreakpoint(System::IntPtr va)
  {
#ifdef _WIN64
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt64());
#else
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt32());
#endif
    _debugger->RemoveBreakpoint(nVA);
  }

  array<System::Byte>^ DebuggerService::ReadMemory(System::IntPtr va, System::UInt32 size)
  {
#ifdef _WIN64
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt64());
#else
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt32());
#endif

    MCP::Native::Debugger::Commands::ReadMemoryResult nresult = _debugger->ReadMemory(nVA, size);
    if (!nresult.IsSuccessful() || !nresult.GetData() || nresult.GetData()->empty())
    {
      return nullptr;
    }

    const uint8_t* nsource = nresult.GetData()->data();
    int length = static_cast<int>(nresult.GetData()->size());

    auto result = gcnew array<System::Byte>(length);
    pin_ptr<System::Byte> target = &result[0];
    std::memcpy(target, nsource, length);

    return result;
  }

  bool DebuggerService::WriteMemory(System::IntPtr va, const array<System::Byte>^ data)
  {
#ifdef _WIN64
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt64());
#else
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt32());
#endif

    size_t length = static_cast<size_t>(data->Length);
    pin_ptr<System::Byte> pdata = &data[0];
    const BYTE* ndata = static_cast<const BYTE*>(pdata);
    MCP::Native::Debugger::Commands::CommandResult result = _debugger->WriteMemory(nVA, ndata, length);

    return result.IsSuccessful();
  }

  IEnumerable<MCP::Debugger::Types::Disassembler::Instruction^>^ DebuggerService::Disassemble(System::IntPtr va, System::UInt32 length)
  {
#ifdef _WIN64
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt64());
#else
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt32());
#endif

    List<MCP::Debugger::Types::Disassembler::Instruction^>^ result = gcnew List<MCP::Debugger::Types::Disassembler::Instruction^>(0);

    MCP::Native::Debugger::Commands::DisassembleResult nresult = _debugger->Disassemble(nVA, length);
    if (!nresult.IsSuccessful() || !nresult.GetInstructions() || nresult.GetInstructions()->empty())
    {
      return result;
    }

    auto instructions = nresult.GetInstructions();
    for (auto& ninstruction : *instructions)
    {
      System::IntPtr virtualAdress = System::IntPtr(static_cast<long long>(ninstruction.VA));
      System::UInt32 instructionLength = ninstruction.InstructionLength;
      System::String^ disassembly = msclr::interop::marshal_as<System::String^>(ninstruction.Disassembly);

      MCP::Debugger::Types::Disassembler::Instruction^ instruction = gcnew MCP::Debugger::Types::Disassembler::Instruction(virtualAdress, instructionLength, disassembly);
      result->Add(instruction);
    }

    return result;
  }

  System::String^ DebuggerService::TryResolveString(System::IntPtr va)
  {
#ifdef _WIN64
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt64());
#else
    DWORD_PTR nVA = static_cast<DWORD_PTR>(va.ToInt32());
#endif

    MCP::Native::Debugger::Commands::TryResolveStringResult nresult = _debugger->TryResolveString(nVA);
    if (!nresult.IsSuccessful())
    {
      return System::String::Empty;
    }

    System::String^ result = msclr::interop::marshal_as<System::String^>(nresult.GetResult());
    return result;
  }

  IEnumerable<MCP::Debugger::Types::Modules::Module^>^ DebuggerService::GetModules(System::UInt32 processId)
  {
    List<MCP::Debugger::Types::Modules::Module^>^ result = gcnew List<MCP::Debugger::Types::Modules::Module^>(0);

    auto processExplorer = _debugger->GetProcessExplorer();
    if (processExplorer == nullptr) return result;

    std::vector<MCP::Native::ProcessExplorer::ModuleInfoPtr> modules;
    processExplorer->GetModules(processId, modules);

    for (const auto& module : modules)
    {
      System::String^ moduleName = msclr::interop::marshal_as<System::String^>(module->GetModuleName());
      System::IntPtr moduleHandle = System::IntPtr(static_cast<void*>(module->GetModuleHandle()));
      System::IntPtr baseAddress = System::IntPtr(static_cast<void*>(module->GetBaseAddress()));
      auto sizeOfImage = module->GetSizeOfImage();

      result->Add(gcnew MCP::Debugger::Types::Modules::Module(processId, moduleName, moduleHandle, baseAddress, sizeOfImage));
    }

    return result;
  }

  IEnumerable<MCP::Debugger::Types::Modules::ModuleImport^>^ DebuggerService::GetModuleImports(System::UInt32 processId, System::IntPtr moduleHandle)
  {
    List<MCP::Debugger::Types::Modules::ModuleImport^>^ result = gcnew List<MCP::Debugger::Types::Modules::ModuleImport^>(0);

    auto processExplorer = _debugger->GetProcessExplorer();
    if (processExplorer == nullptr) return result;

    MCP::Native::ProcessExplorer::ModuleInfoPtr module;
    if (!processExplorer->GetModule(processId, static_cast<HMODULE>(moduleHandle.ToPointer()), module))
    {
      return result;
    }

    const auto& nimports = module->GetImports();
    for (const auto& nimport : nimports)
    {
      MCP::Debugger::Types::Modules::ModuleImport^ import = gcnew MCP::Debugger::Types::Modules::ModuleImport();

      import->ModuleName = msclr::interop::marshal_as<System::String^>(nimport.ModuleName);

      for (const auto& nfunction : nimport.Functions)
      {
        MCP::Debugger::Types::Modules::ModuleImportFunction^ function = gcnew MCP::Debugger::Types::Modules::ModuleImportFunction();
        function->Name = msclr::interop::marshal_as<System::String^>(nfunction.Name);
        function->Ordinal = nfunction.Ordinal;
        function->ImportByName = nfunction.ImportByName;
        function->IATEntryAddress = System::IntPtr(nfunction.IATEntryAddress);

        import->Functions->Add(function);
      }

      result->Add(import);
    }

    return result;
  }

  IEnumerable<MCP::Debugger::Types::Modules::ModuleExport^>^ DebuggerService::GetModuleExports(System::UInt32 processId, System::IntPtr moduleHandle)
  {
    List<MCP::Debugger::Types::Modules::ModuleExport^>^ result = gcnew List<MCP::Debugger::Types::Modules::ModuleExport^>(0);

    auto processExplorer = _debugger->GetProcessExplorer();
    if (processExplorer == nullptr) return result;

    MCP::Native::ProcessExplorer::ModuleInfoPtr module;
    if (!processExplorer->GetModule(processId, static_cast<HMODULE>(moduleHandle.ToPointer()), module))
    {
      return result;
    }

    const auto& nexports = module->GetExports();
    for (const auto& nexport : nexports)
    {
      MCP::Debugger::Types::Modules::ModuleExport^ mexport = gcnew MCP::Debugger::Types::Modules::ModuleExport();

      mexport->Name = msclr::interop::marshal_as<System::String^>(nexport.Name);
      mexport->ForwardedName = msclr::interop::marshal_as<System::String^>(nexport.ForwardedName);
      mexport->Ordinal = nexport.Ordinal;
      mexport->VirtualAddress = System::IntPtr(nexport.VirtualAddress);
      result->Add(mexport);
    }

    return result;
  }

  IEnumerable<MCP::Debugger::Types::Modules::ModuleSection^>^ DebuggerService::GetModuleSections(System::UInt32 processId, System::IntPtr moduleHandle)
  {
    List<MCP::Debugger::Types::Modules::ModuleSection^>^ result = gcnew List<MCP::Debugger::Types::Modules::ModuleSection^>(0);

    auto processExplorer = _debugger->GetProcessExplorer();
    if (processExplorer == nullptr) return result;

    MCP::Native::ProcessExplorer::ModuleInfoPtr module;
    if (!processExplorer->GetModule(processId, static_cast<HMODULE>(moduleHandle.ToPointer()), module))
    {
      return result;
    }

    const auto& nsections = module->GetSections();
    for (const auto& nsection : nsections)
    {
      MCP::Debugger::Types::Modules::ModuleSection^ msection = gcnew MCP::Debugger::Types::Modules::ModuleSection();

      msection->Name = msclr::interop::marshal_as<System::String^>(nsection.Name);
      msection->BaseAddress = System::IntPtr(nsection.BaseAddress);
      msection->Characteristics = nsection.Characteristics;
      msection->VirtualSize = nsection.VirtualSize;

      result->Add(msection);
    }

    return result;
  }

  System::IntPtr DebuggerService::GetRemoteModuleHandle(System::UInt32 processId, String^ moduleName)
  {
    auto processExplorer = _debugger->GetProcessExplorer();
    if (processExplorer == nullptr) return System::IntPtr::Zero;

    std::wstring nmoduleName = msclr::interop::marshal_as<std::wstring>(moduleName);

    auto result = processExplorer->GetRemoteModuleHandle(processId, nmoduleName.c_str());

    return System::IntPtr(reinterpret_cast<void*>(result));
  }

  System::IntPtr DebuggerService::GetRemoteProcAddress(System::UInt32 processId, System::IntPtr moduleHandle, String^ procedureName)
  {
    auto processExplorer = _debugger->GetProcessExplorer();
    if (processExplorer == nullptr) return System::IntPtr::Zero;

    std::string nprocedureName = msclr::interop::marshal_as<std::string>(procedureName);

    auto result = processExplorer->GetRemoteProcAddress(processId, static_cast<HMODULE>(moduleHandle.ToPointer()), nprocedureName.c_str());

    return System::IntPtr(reinterpret_cast<void*>(result));
  }

  void DebuggerService::OnExitProcess(MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^ args)
  {
    auto action = gcnew Action<MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^>(this, &DebuggerService::RaiseExitProcessEvent);
    _dispatcher->Dispatch(action, args);
  }

  void DebuggerService::OnSystemBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    auto action = gcnew Action<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>(this, &DebuggerService::RaiseSystemBreakpointEvent);
    _dispatcher->Dispatch(action, args);
  }

  void DebuggerService::OnEntryPointBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    auto action = gcnew Action<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>(this, &DebuggerService::RaiseEntryPointBreakpointEvent);
    _dispatcher->Dispatch(action, args);
  }

  void DebuggerService::OnBreakpoint(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    auto action = gcnew Action<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>(this, &DebuggerService::RaiseBreakpointEvent);
    _dispatcher->Dispatch(action, args);
  }

  void DebuggerService::OnSingleStep(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    auto action = gcnew Action<MCP::Debugger::Types::EventArgs::BreakpointEventArgs^>(this, &DebuggerService::RaiseSingleStepEvent);
    _dispatcher->Dispatch(action, args);
  }

  MCP::Debugger::Types::Interop::CONTEXT DebuggerService::GetThreadContext(HANDLE threadHandle)
  {
    ::CONTEXT* ncontext = new ::CONTEXT();
    try
    {
      memset(ncontext, 0, sizeof(::CONTEXT));

      ncontext->ContextFlags = CONTEXT_FULL;
      if (::GetThreadContext(threadHandle, ncontext))
      {
        return ConvertThreadContext(*ncontext);
      }
      return MCP::Debugger::Types::Interop::CONTEXT();
    }
    finally
    {
      delete ncontext;
    }
  }

  MCP::Debugger::Types::Interop::CONTEXT DebuggerService::ConvertThreadContext(::CONTEXT& ncontext)
  {
    IntPtr buffer = Marshal::AllocHGlobal(sizeof(::CONTEXT));
    try
    {
      memcpy_s(buffer.ToPointer(), sizeof(::CONTEXT), &ncontext, sizeof(::CONTEXT));
      auto managedContext = safe_cast<MCP::Debugger::Types::Interop::CONTEXT>(Marshal::PtrToStructure(buffer, MCP::Debugger::Types::Interop::CONTEXT::typeid));
      return managedContext;
    }
    finally
    {
      Marshal::FreeHGlobal(buffer);
    }
  }

  void DebuggerService::RaiseExitProcessEvent(MCP::Debugger::Types::EventArgs::ExitProcessEventArgs^ args)
  {
    ExitProcess(this, args);
  }

  void DebuggerService::RaiseSystemBreakpointEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    SystemBreakpoint(this, args);
  }

  void DebuggerService::RaiseEntryPointBreakpointEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    EntryPointBreakpoint(this, args);
  }

  void DebuggerService::RaiseBreakpointEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    Breakpoint(this, args);
  }

  void DebuggerService::RaiseSingleStepEvent(MCP::Debugger::Types::EventArgs::BreakpointEventArgs^ args)
  {
    SingleStep(this, args);
  }

#pragma unmanaged
  bool NativeStepInto(MCP::Native::Debugger::Debugger* debugger)
  {
    MCP::Native::Debugger::Commands::CommandResult nresult = debugger->StepInto();
    return nresult.IsSuccessful();
  }

  bool NativeStepOver(MCP::Native::Debugger::Debugger* debugger)
  {
    MCP::Native::Debugger::Commands::CommandResult nresult = debugger->StepOver();
    return nresult.IsSuccessful();
  }
#pragma managed

}