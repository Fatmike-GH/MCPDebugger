#include "Mocks/Debugger/DebugEventHandler/Interfaces/MockDebugEventHandler.h"
#include "Native/Debugger/DebugEventHandler/DebugEventHandler.h"
#include "Native/Debugger/DebugEventHandler/HandleResult.h"
#include "Native/Debugger/DebugEventHandler/Interfaces/IDebugEventDispatcher.h"
#include "Native/Debugger/DebugEventHandler/Interfaces/IDebugEventHandler.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <utility>

// The production names are written out from the root namespace: inside the test namespace "Debugger" and
// "DebugEventHandler" name the test namespaces, and "DebugEventHandler" is a namespace and a class at once.
namespace MCP::Native::Tests::Debugger::DebugEventHandler
{
  using MCP::Native::Tests::Mocks::Debugger::DebugEventHandler::Interfaces::MockDebugEventHandler;
  using ::testing::Ref;
  using ::testing::Return;
  using ::testing::StrictMock;

  using HandleResult = MCP::Native::Debugger::DebugEventHandler::HandleResult;
  using IDebugEventDispatcher = MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventDispatcher;
  using IDebugEventHandler = MCP::Native::Debugger::DebugEventHandler::Interfaces::IDebugEventHandler;
  using Router = MCP::Native::Debugger::DebugEventHandler::DebugEventHandler;

  /// <summary>
  /// Tests the routing of debug events with doubles of the handlers: two handlers registered for an event code
  /// each and the handler for every other code. The doubles are strict, so a test fails when an event reaches
  /// a handler it does not expect.
  /// </summary>
  class DebugEventHandlerTests : public ::testing::Test
  {
  protected:
    static constexpr DWORD FirstCode = EXCEPTION_DEBUG_EVENT;
    static constexpr DWORD SecondCode = EXIT_PROCESS_DEBUG_EVENT;
    static constexpr DWORD CodeWithoutHandler = RIP_EVENT;

    DebugEventHandlerTests()
    {
      auto firstHandler = std::make_unique<StrictMock<MockDebugEventHandler>>();
      auto secondHandler = std::make_unique<StrictMock<MockDebugEventHandler>>();
      auto defaultHandler = std::make_unique<StrictMock<MockDebugEventHandler>>();
      _firstHandler = firstHandler.get();
      _secondHandler = secondHandler.get();
      _defaultHandler = defaultHandler.get();

      std::map<DWORD, std::unique_ptr<IDebugEventHandler>> eventHandlers;
      eventHandlers[FirstCode] = std::move(firstHandler);
      eventHandlers[SecondCode] = std::move(secondHandler);
      _dispatcher = std::make_unique<Router>(std::move(eventHandlers), std::move(defaultHandler));
    }

    static DEBUG_EVENT MakeDebugEvent(DWORD eventCode)
    {
      DEBUG_EVENT debugEvent = {};
      debugEvent.dwDebugEventCode = eventCode;
      debugEvent.dwProcessId = 1000;
      debugEvent.dwThreadId = 2000;
      return debugEvent;
    }

  protected:
    // Owned by the router under test.
    StrictMock<MockDebugEventHandler>* _firstHandler = nullptr;
    StrictMock<MockDebugEventHandler>* _secondHandler = nullptr;
    StrictMock<MockDebugEventHandler>* _defaultHandler = nullptr;

    // The router is used the way the debug thread's services use it: through its routing contract.
    std::unique_ptr<IDebugEventDispatcher> _dispatcher;
  };

  TEST_F(DebugEventHandlerTests, HandleDebugEventPassesAnEventToTheHandlerOfItsCodeAndReturnsItsResult)
  {
    // Arrange
    DEBUG_EVENT debugEvent = MakeDebugEvent(FirstCode);
    EXPECT_CALL(*_firstHandler, HandleEvent(Ref(debugEvent)))
      .WillOnce(Return(HandleResult{ DBG_EXCEPTION_NOT_HANDLED, false }));

    // Act
    HandleResult result = _dispatcher->HandleDebugEvent(debugEvent);

    // Assert
    EXPECT_EQ(DBG_EXCEPTION_NOT_HANDLED, result.ContinueStatus);
    EXPECT_FALSE(result.ProcessExited);
  }

  TEST_F(DebugEventHandlerTests, HandleDebugEventReturnsTheProcessExitTheHandlerOfTheCodeReports)
  {
    // Arrange
    DEBUG_EVENT debugEvent = MakeDebugEvent(SecondCode);
    EXPECT_CALL(*_secondHandler, HandleEvent(Ref(debugEvent)))
      .WillOnce(Return(HandleResult{ DBG_CONTINUE, true }));

    // Act
    HandleResult result = _dispatcher->HandleDebugEvent(debugEvent);

    // Assert
    EXPECT_EQ(DBG_CONTINUE, result.ContinueStatus);
    EXPECT_TRUE(result.ProcessExited);
  }

  TEST_F(DebugEventHandlerTests, HandleDebugEventPassesAnEventWithoutOwnHandlerToTheDefaultHandler)
  {
    // Arrange
    DEBUG_EVENT debugEvent = MakeDebugEvent(CodeWithoutHandler);
    EXPECT_CALL(*_defaultHandler, HandleEvent(Ref(debugEvent)))
      .WillOnce(Return(HandleResult{ DBG_EXCEPTION_NOT_HANDLED, true }));

    // Act
    HandleResult result = _dispatcher->HandleDebugEvent(debugEvent);

    // Assert
    EXPECT_EQ(DBG_EXCEPTION_NOT_HANDLED, result.ContinueStatus);
    EXPECT_TRUE(result.ProcessExited);
  }
}
