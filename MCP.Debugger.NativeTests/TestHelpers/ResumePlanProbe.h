#pragma once
#include "Mocks/Debugger/Interfaces/MockStopSite.h"
#include "Native/Debugger/Interfaces/IResumePlan.h"
#include <Windows.h>
#include <gmock/gmock.h>
#include <optional>
#include <ostream>

// Tells the plans of the resuming commands apart by what they do, for tests that get a plan as the interface
// only and have to say which command it came from. The plan classes themselves are tested on their own.
namespace MCP::Native::Tests::TestHelpers
{
  /// <summary>The stop address the probe begins a plan at.</summary>
  inline constexpr DWORD_PTR ProbedStopAddress = 0x401010;

  /// <summary>The length of the call the probe lets the plan find at the stop address.</summary>
  inline constexpr size_t ProbedCallLength = 5;

  /// <summary>What a plan did when it was begun at a stop on a call.</summary>
  struct ResumePlanBehaviour
  {
    /// <summary>Where the plan placed a breakpoint; empty when it placed none.</summary>
    std::optional<DWORD_PTR> BreakpointPlacedAt;

    /// <summary>Whether the debuggee halts as soon as the call instruction itself has run.</summary>
    bool HaltsAfterFirstInstruction = false;

    bool operator==(const ResumePlanBehaviour&) const = default;
  };

  /// <summary>Continue: nothing is placed and the debuggee runs on.</summary>
  inline constexpr ResumePlanBehaviour BehaviourOfContinue{ std::nullopt, false };

  /// <summary>Step into: nothing is placed and the debuggee halts inside the called function.</summary>
  inline constexpr ResumePlanBehaviour BehaviourOfStepInto{ std::nullopt, true };

  /// <summary>Step over: a breakpoint behind the call, and the debuggee runs on to it.</summary>
  inline constexpr ResumePlanBehaviour BehaviourOfStepOver{ ProbedStopAddress + ProbedCallLength, false };

  /// <summary>Prints the behaviour in words; without it Google Test would dump the bytes of the struct.</summary>
  inline void PrintTo(const ResumePlanBehaviour& behaviour, std::ostream* output)
  {
    if (behaviour.BreakpointPlacedAt)
    {
      *output << "breakpoint placed at 0x" << std::hex << *behaviour.BreakpointPlacedAt << std::dec;
    }
    else
    {
      *output << "no breakpoint placed";
    }
    *output << (behaviour.HaltsAfterFirstInstruction ? ", halts after the first instruction" : ", runs on");
  }

  /// <summary>
  /// Begins the plan at a stop on a call and reports what it did there. A plan is begun once, so the probe
  /// uses the plan up.
  /// </summary>
  inline ResumePlanBehaviour ProbeAtACall(MCP::Native::Debugger::Interfaces::IResumePlan& plan)
  {
    ResumePlanBehaviour behaviour;
    ::testing::NiceMock<MCP::Native::Tests::Mocks::Debugger::Interfaces::MockStopSite> stopSite;
    ON_CALL(stopSite, GetStopAddress()).WillByDefault(::testing::Return(ProbedStopAddress));
    ON_CALL(stopSite, TryGetCallLength()).WillByDefault(::testing::Return(std::optional<size_t>(ProbedCallLength)));
    ON_CALL(stopSite, SetBreakpoint(::testing::_)).WillByDefault([&behaviour](DWORD_PTR address) { behaviour.BreakpointPlacedAt = address; });

    plan.Begin(stopSite);

    behaviour.HaltsAfterFirstInstruction = plan.HaltsAfterFirstInstruction();
    return behaviour;
  }
}
