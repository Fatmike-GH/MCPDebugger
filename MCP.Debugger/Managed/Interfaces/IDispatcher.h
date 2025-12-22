#pragma once

using namespace System;

namespace MCP::Debugger
{
  public interface class IDispatcher
  {
  public:
    void Dispatch(Action^ action);

    generic <typename T>
    void Dispatch(Action<T>^ action, T args);

    void Shutdown();
  };
}