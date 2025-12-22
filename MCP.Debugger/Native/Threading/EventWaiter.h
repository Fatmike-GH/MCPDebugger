#pragma once
#include <mutex>
#include <condition_variable>

// Allows one thread to wait until another thread has completed and reported a success/failure result
namespace MCP::Native::Threading
{
  class EventWaiter
  {
  public:
    EventWaiter();
    bool Wait();
    void Wakeup(bool success);
    void Reset();

  private:
    std::mutex _mutex;
    std::condition_variable _cv;
    bool _finished;
    bool _success;
  };
}
