#pragma once

#include ".\Interfaces\IDispatcher.h"

using namespace System;
namespace MCP::Debugger
{
  public ref class UiDispatcher : public IDispatcher
  {
  public:
    UiDispatcher();

    virtual void Dispatch(Action^ action);

    generic <typename T>
    virtual void Dispatch(Action<T>^ action, T args);

    virtual void Shutdown() {}

  private:
    static void PostCallback(Object^ state);

    generic <typename T>
    static void PostCallback(Object^ state);

  private:
    System::Object^ _lock;
    System::Threading::SynchronizationContext^ _synchronizationContext;
  };
}
