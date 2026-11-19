#pragma once
#include <cassert>
#include <memory>
#include <utility>
#include "IResumePlan.h"

namespace MCP::Native::Debugger::Interfaces
{
  /// <summary>
  /// How the processing of a stop ended: a command resumed the debuggee and left the plan for it, or a
  /// command ended the session. A value that can be moved but not copied, because it owns the plan.
  /// </summary>
  class StopOutcome
  {
  public:
    /// <summary>The debuggee is resumed as the plan says. The plan must not be null.</summary>
    [[nodiscard]] static StopOutcome Resumed(std::unique_ptr<IResumePlan> plan)
    {
      // Whoever takes the plan of a resumed debuggee drives it without asking whether there is one
      assert(plan != nullptr);
      return StopOutcome(std::move(plan), false);
    }

    /// <summary>The session ends: the debuggee is not resumed.</summary>
    [[nodiscard]] static StopOutcome SessionEnded()
    {
      return StopOutcome(nullptr, true);
    }

    StopOutcome(StopOutcome&&) noexcept = default;
    StopOutcome& operator=(StopOutcome&&) noexcept = default;
    StopOutcome(const StopOutcome&) = delete;
    StopOutcome& operator=(const StopOutcome&) = delete;
    ~StopOutcome() = default;

    /// <summary>True when the session ended; there is no plan then.</summary>
    [[nodiscard]] bool IsSessionEnded() const
    {
      return _sessionEnded;
    }

    /// <summary>
    /// Hands out the plan of a resumed debuggee. The first call returns it, every further call and a call
    /// on an ended session return null.
    /// </summary>
    [[nodiscard]] std::unique_ptr<IResumePlan> TakePlan()
    {
      return std::move(_plan);
    }

  private:
    StopOutcome(std::unique_ptr<IResumePlan> plan, bool sessionEnded)
      : _plan(std::move(plan)),
        _sessionEnded(sessionEnded)
    {
    }

    std::unique_ptr<IResumePlan> _plan;
    bool _sessionEnded;
  };
}
