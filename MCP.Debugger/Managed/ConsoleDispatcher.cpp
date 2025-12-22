#include "ConsoleDispatcher.h"
#include "ActionWrapper.h"

using namespace System;
using namespace System::Threading;
using namespace System::Threading::Tasks;

namespace MCP::Debugger
{
  ConsoleDispatcher::ConsoleDispatcher()
  {
    _actionQueue = gcnew BlockingCollection<Action^>();
    _queueProcessor = Task::Factory->StartNew(gcnew Action(this, &ConsoleDispatcher::ProcessQueue), TaskCreationOptions::LongRunning);
  }

  void ConsoleDispatcher::Shutdown()
  {
    if (_actionQueue != nullptr && !_actionQueue->IsAddingCompleted)
    {
      _actionQueue->CompleteAdding();
    }

    if (_queueProcessor != nullptr)
    {
      try
      {
        _queueProcessor->Wait(1000);
      }
      catch (...) 
      {
      }
    }
  }

  void ConsoleDispatcher::ProcessQueue()
  {
    try
    {
      // Wait until an item is available
      for each (Action ^ action in _actionQueue->GetConsumingEnumerable())
      {
        try
        {
          action();
        }
        catch (Exception^)
        {
        }
      }
    }
    catch (InvalidOperationException^)
    {
      // Occurs when CompleteAdding() is called during shutdown.
      // This is expected behavior.
    }
  }

  void ConsoleDispatcher::Dispatch(Action^ action)
  {
    if (!_actionQueue->IsAddingCompleted)
    {
      _actionQueue->Add(action);
    }
  }

  generic <typename T>
  void ConsoleDispatcher::Dispatch(Action<T>^ action, T args)
  {
    auto wrapper = gcnew ActionWrapper<T>(action, args);
    Action^ boundAction = gcnew Action(wrapper, &ActionWrapper<T>::Invoke);
    this->Dispatch(boundAction);
  }
}