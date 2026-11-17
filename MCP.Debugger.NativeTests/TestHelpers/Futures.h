#pragma once
#include <chrono>
#include <future>

namespace MCP::Native::Tests::TestHelpers
{
  /// <summary>
  /// Tells whether the promise behind a future was completed, without waiting for it:
  /// a completed promise makes the future ready at once, and the zero timeout never blocks.
  /// </summary>
  template <typename TResult>
  bool IsCompleted(const std::future<TResult>& future)
  {
    return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
  }
}
