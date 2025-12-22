#pragma once

using namespace System;

namespace MCP::Debugger
{
  generic <typename T>
  ref class ActionWrapper
  {
  public:
    ActionWrapper(Action<T>^ target, T args)
    {
      _target = target;
      _args = args;
    }

    void Invoke()
    {
      _target(_args);
    }

  private:
    Action<T>^ _target;
    T _args;
  };
}