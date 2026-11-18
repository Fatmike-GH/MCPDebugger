#pragma once
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <mutex>
#include <utility>
#include <gtest/gtest.h>

// Helpers for tests in which several threads meet. Such a test never sleeps and never polls: it waits on a
// synchronisation object until the other thread reached a known point, and every wait has the same upper
// bound, so a broken test fails instead of blocking the test run.
namespace MCP::Native::Tests::TestHelpers
{
  /// <summary>
  /// Upper bound of every wait on another thread. A passing test is released long before; only a failing one
  /// reaches the bound.
  /// </summary>
  inline constexpr std::chrono::seconds CrossThreadWaitBound{ 10 };

  /// <summary>
  /// Reports the expired wait as a failure of the running test and ends the test process with a failure exit
  /// code. Going on is not possible: the thread that did not arrive is still blocked, cannot be joined, and
  /// would keep the test run from ever finishing.
  /// </summary>
  [[noreturn]] inline void EndTestRunAfterExpiredWait(const char* awaited)
  {
    ADD_FAILURE() << "Waited " << CrossThreadWaitBound.count() << " s in vain for " << awaited
                  << ". The test process ends here, because a thread that is still blocked cannot be joined.";
    std::fflush(nullptr);
    std::_Exit(EXIT_FAILURE);
  }

  /// <summary>
  /// Counts signals given by other threads, so that a test can wait until something happened a known number
  /// of times. Signalling only takes a lock and notifies: it is safe inside the action of a mock.
  /// </summary>
  class SignalCounter
  {
  public:
    void Signal()
    {
      {
        std::lock_guard<std::mutex> lock(_mutex);
        _count++;
      }
      _changed.notify_all();
    }

    /// <summary>
    /// Blocks until at least the given number of signals arrived. Ends the test run when that takes longer
    /// than the bound; the text names what was awaited.
    /// </summary>
    void WaitForCount(int count, const char* awaited)
    {
      std::unique_lock<std::mutex> lock(_mutex);
      if (!_changed.wait_for(lock, CrossThreadWaitBound, [&] { return _count >= count; }))
      {
        EndTestRunAfterExpiredWait(awaited);
      }
    }

    int Count()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return _count;
    }

  private:
    std::mutex _mutex;
    std::condition_variable _changed;
    int _count = 0;
  };

  /// <summary>
  /// Runs a blocking operation on a thread of its own, like a client thread does. The destructor of the
  /// returned future joins that thread, so the test must make sure the operation returns.
  /// </summary>
  template <typename TOperation>
  [[nodiscard]] auto RunOnOtherThread(TOperation operation)
  {
    return std::async(std::launch::async, std::move(operation));
  }

  /// <summary>
  /// Returns what the operation behind the future returned. Ends the test run when the operation has not
  /// returned within the bound; the text names what was awaited.
  /// </summary>
  template <typename TResult>
  TResult WaitForResult(std::future<TResult>& future, const char* awaited)
  {
    if (future.wait_for(CrossThreadWaitBound) != std::future_status::ready)
    {
      EndTestRunAfterExpiredWait(awaited);
    }
    return future.get();
  }
}
