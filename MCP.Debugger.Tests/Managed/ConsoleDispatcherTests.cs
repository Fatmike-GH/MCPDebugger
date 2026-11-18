using System.Diagnostics;

namespace MCP.Debugger.Tests.Managed
{
  /// <summary>
  /// How the console dispatcher runs the actions handed to it: one after the other on a thread of its own,
  /// until it is shut down.
  /// </summary>
  /// <remarks>
  /// The actions of these tests only record what happened and signal the test thread, which asserts. An
  /// assertion inside an action would be lost: the dispatcher swallows the exceptions of its actions.
  /// </remarks>
  public class ConsoleDispatcherTests
  {
    /// <summary>
    /// How long a test waits for the dispatcher thread before it gives up and fails. A wait ends as soon as
    /// the awaited action ran, so the limit costs time in a failing test only.
    /// </summary>
    private static readonly TimeSpan GiveUpAfter = TimeSpan.FromSeconds(1);

    /// <summary>The longest time Shutdown waits for the dispatcher thread before it returns anyway.</summary>
    private static readonly TimeSpan ShutdownBound = TimeSpan.FromMilliseconds(1000);

    /// <summary>The dispatcher under test.</summary>
    private ConsoleDispatcher _dispatcher;

    /// <summary>Creates the dispatcher, which starts its thread.</summary>
    [SetUp]
    public void CreateDispatcher()
    {
      _dispatcher = new ConsoleDispatcher();
    }

    /// <summary>Ends the dispatcher thread; harmless for a dispatcher the test already shut down.</summary>
    [TearDown]
    public void ShutDownDispatcher()
    {
      _dispatcher.Shutdown();
    }

    [Test]
    public void Dispatch_SeveralActions_RunsThemInDispatchOrderOnOneOtherThread()
    {
      // Arrange
      const int actionCount = 20;
      var order = new List<int>();
      var threadIds = new List<int>();
      using var allRan = new CountdownEvent(actionCount);

      // Act
      for (int i = 0; i < actionCount; i++)
      {
        int index = i;
        _dispatcher.Dispatch(() =>
        {
          order.Add(index);
          threadIds.Add(Environment.CurrentManagedThreadId);
          allRan.Signal();
        });
      }
      bool ran = allRan.Wait(GiveUpAfter);

      // Assert
      Assert.That(ran, Is.True, "Not every action ran.");
      Assert.That(order, Is.EqualTo(Enumerable.Range(0, actionCount)));
      Assert.That(threadIds.Distinct().Count(), Is.EqualTo(1));
      Assert.That(threadIds[0], Is.Not.EqualTo(Environment.CurrentManagedThreadId));
    }

    [Test]
    public void Dispatch_ActionWithArgument_PassesTheArgumentToTheAction()
    {
      // Arrange
      string received = null;
      using var ran = new ManualResetEventSlim();

      // Act
      _dispatcher.Dispatch<string>(
        argument =>
        {
          received = argument;
          ran.Set();
        },
        "the argument");
      bool completed = ran.Wait(GiveUpAfter);

      // Assert
      Assert.That(completed, Is.True, "The action did not run.");
      Assert.That(received, Is.EqualTo("the argument"));
    }

    [Test]
    public void Dispatch_ActionThrows_StillRunsTheNextAction()
    {
      // Arrange
      using var nextRan = new ManualResetEventSlim();

      // Act
      _dispatcher.Dispatch(() => throw new InvalidOperationException("The action failed."));
      _dispatcher.Dispatch(() => nextRan.Set());
      bool completed = nextRan.Wait(GiveUpAfter);

      // Assert
      Assert.That(completed, Is.True, "The action after the failing one did not run.");
    }

    [Test]
    public void Shutdown_ActionsAreQueued_RunsThemAndReturnsWithinItsBound()
    {
      // Arrange
      const int actionCount = 5;
      int ran = 0;
      for (int i = 0; i < actionCount; i++)
      {
        _dispatcher.Dispatch(() => Interlocked.Increment(ref ran));
      }
      var stopwatch = Stopwatch.StartNew();

      // Act
      _dispatcher.Shutdown();
      stopwatch.Stop();
      int ranWhenShutdownReturned = Volatile.Read(ref ran);

      // Assert: a shutdown that gave up waiting for the dispatcher thread would have taken the whole bound.
      Assert.That(ranWhenShutdownReturned, Is.EqualTo(actionCount));
      Assert.That(stopwatch.Elapsed, Is.LessThan(ShutdownBound));
    }

    [Test]
    public void Dispatch_AfterShutdown_DoesNotRunTheAction()
    {
      // Arrange
      _dispatcher.Shutdown();
      int ran = 0;

      // Act
      _dispatcher.Dispatch(() => Interlocked.Increment(ref ran));

      // Assert: checked without a wait. The shutdown has ended the dispatcher thread, so nothing is left that
      // could run the action later; and an attempt to queue it would have thrown.
      Assert.That(Volatile.Read(ref ran), Is.EqualTo(0));
    }
  }
}
