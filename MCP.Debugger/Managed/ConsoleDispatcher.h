#pragma once

#include ".\Interfaces\IDispatcher.h"

using namespace System;
using namespace System::Collections::Concurrent;
using namespace System::Threading::Tasks;

namespace MCP::Debugger
{
  public ref class ConsoleDispatcher : public IDispatcher
  {
  public:
    ConsoleDispatcher();

    virtual void Dispatch(Action^ action);

    generic <typename T>
    virtual void Dispatch(Action<T>^ action, T args);

    virtual void Shutdown();

  private:
    void ProcessQueue();

  private:
    BlockingCollection<Action^>^ _actionQueue;
    Task^ _queueProcessor;
  };
}