#include <msclr/lock.h>
#include "UiDispatcher.h"

using namespace System;
using namespace System::Runtime::InteropServices;

namespace MCP::Debugger
{
  UiDispatcher::UiDispatcher()
  {
    _lock = gcnew System::Object();
    _synchronizationContext = System::Threading::SynchronizationContext::Current;
  }

  void UiDispatcher::Dispatch(Action^ action)
  {
    System::Threading::SynchronizationContext^ context = nullptr;
    {
      msclr::lock lock(_lock);
      context = _synchronizationContext;
    }

    if (context != nullptr)
    {
      context->Post(gcnew System::Threading::SendOrPostCallback(&UiDispatcher::PostCallback), action);
    }
    else
    {
      action();
    }
  }

  void UiDispatcher::PostCallback(Object^ state)
  {
    Action^ action = static_cast<Action^>(state);
    action();
  }

  generic <typename T>
  void UiDispatcher::Dispatch(Action<T>^ action, T args)
  {
    auto state = gcnew System::Tuple<Action<T>^, T>(action, args);

    System::Threading::SynchronizationContext^ context = nullptr;
    {
      msclr::lock lock(_lock);
      context = _synchronizationContext;
    }

    if (context != nullptr)
    {
      context->Post(gcnew System::Threading::SendOrPostCallback(&UiDispatcher::PostCallback<T>), state);
    }
    else
    {
      action(args);
    }
  }

  generic <typename T>
  void UiDispatcher::PostCallback(Object^ state)
  {
    auto tuple = static_cast<System::Tuple<Action<T>^, T>^>(state);
    Action<T>^ action = tuple->Item1;
    T args = tuple->Item2;
    action(args);
  }
}