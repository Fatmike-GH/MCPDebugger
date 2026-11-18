#include "DebuggeeBreakIn.h"

namespace MCP::Native::Debugger
{
  void DebuggeeBreakIn::RequestBreakIn(HANDLE processHandle)
  {
    // The result is ignored on purpose: a process that cannot be interrupted (already exited, null handle)
    // leaves nothing to do here.
    DebugBreakProcess(processHandle);
  }
}
