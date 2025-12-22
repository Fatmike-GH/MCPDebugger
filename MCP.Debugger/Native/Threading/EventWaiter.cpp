#pragma once
#include "EventWaiter.h"

namespace MCP::Native::Threading
{
  EventWaiter::EventWaiter()
  {
    _finished = false;
    _success = false;
  }

  bool EventWaiter::Wait()
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait(lock, [this] { return _finished; });

    return _success;
  }

  void EventWaiter::Wakeup(bool success)
  {
    {
      std::lock_guard<std::mutex> lock(_mutex);
      _success = success;
      _finished = true;
    } // Release lock

    // Notify the one waiting thread.
    _cv.notify_one();
  }

  void EventWaiter::Reset()
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _finished = false;
    _success = false;
  }
}