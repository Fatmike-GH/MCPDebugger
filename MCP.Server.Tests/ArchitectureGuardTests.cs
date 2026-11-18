using System.Reflection;
using System.Runtime.CompilerServices;
using MCP.Debugger.Types.Interop;
using MCP.Server.McpTools;

namespace MCP.Server.Tests
{
  /// <summary>
  /// Structure rules of the server assembly that the compiler does not check: where its interfaces live and
  /// what its classes may be given through their constructors.
  /// </summary>
  public class ArchitectureGuardTests
  {
    /// <summary>The namespace all interfaces of the server live in, directly or in a sub-namespace.</summary>
    private const string InterfacesNamespace = "MCP.Server.Interfaces";

    /// <summary>The assembly the rules are checked on.</summary>
    private static readonly Assembly ServerAssembly = typeof(DebuggerTools).Assembly;

    /// <summary>The assembly of the plain data types that are passed between debugger, server and tools.</summary>
    private static readonly Assembly DataTypesAssembly = typeof(CONTEXT).Assembly;

    [Test]
    public void Interfaces_OfTheServer_LiveInTheInterfacesNamespace()
    {
      // Arrange
      List<Type> interfaces = ServerAssembly.GetTypes().Where(type => type.IsInterface).ToList();

      // Act
      List<string> misplaced = interfaces.Where(type => !IsInInterfacesNamespace(type))
                                         .Select(type => type.FullName)
                                         .ToList();

      // Assert
      Assert.That(interfaces, Is.Not.Empty, "No interface found: the rule was checked on nothing.");
      Assert.That(misplaced, Is.Empty, "Interfaces outside " + InterfacesNamespace);
    }

    [Test]
    public void ConstructorParameters_OfTheServerClasses_AreInterfacesOrData()
    {
      // Arrange
      List<ParameterInfo> parameters = ServerAssembly.GetTypes()
                                                     .Where(type => type.IsClass && !IsCompilerGenerated(type))
                                                     .SelectMany(ConstructorParametersOf)
                                                     .ToList();

      // Act
      List<string> violations = parameters.Where(parameter => !IsInterfaceOrData(parameter.ParameterType))
                                          .Select(Describe)
                                          .ToList();

      // Assert
      Assert.That(parameters, Is.Not.Empty, "No constructor parameter found: the rule was checked on nothing.");
      Assert.That(violations, Is.Empty, "A class must receive a collaborator as an interface, not as a class");
    }

    /// <summary>Tells whether a type lives in the interfaces namespace or in one below it.</summary>
    /// <param name="type">The type to check.</param>
    /// <returns>True when it does.</returns>
    private static bool IsInInterfacesNamespace(Type type)
    {
      return type.Namespace != null &&
             (type.Namespace == InterfacesNamespace ||
              type.Namespace.StartsWith(InterfacesNamespace + ".", StringComparison.Ordinal));
    }

    /// <summary>
    /// Tells whether the compiler created a type (anonymous types, closures, state machines) - directly or as
    /// a type nested in such a type. Nobody writes their constructors, so the rule does not apply to them.
    /// </summary>
    /// <param name="type">The type to check.</param>
    /// <returns>True when the type or a type it is nested in was created by the compiler.</returns>
    private static bool IsCompilerGenerated(Type type)
    {
      for (Type current = type; current != null; current = current.DeclaringType)
      {
        if (current.IsDefined(typeof(CompilerGeneratedAttribute), inherit: false))
        {
          return true;
        }
      }

      return false;
    }

    /// <summary>Lists the parameters of all instance constructors of a class, public or not.</summary>
    /// <param name="type">The class.</param>
    /// <returns>The parameters of its constructors.</returns>
    private static IEnumerable<ParameterInfo> ConstructorParametersOf(Type type)
    {
      const BindingFlags InstanceConstructors = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.DeclaredOnly;

      return type.GetConstructors(InstanceConstructors).SelectMany(constructor => constructor.GetParameters());
    }

    /// <summary>
    /// Tells whether a type may be passed into a class: an interface, a value, a string or one of the shared
    /// data types. Everything else is a class with behaviour, which must be passed as its interface.
    /// </summary>
    /// <param name="parameterType">Type of a constructor parameter.</param>
    /// <returns>True when the type is allowed.</returns>
    private static bool IsInterfaceOrData(Type parameterType)
    {
      return parameterType.IsInterface ||
             parameterType.IsValueType ||
             parameterType == typeof(string) ||
             parameterType.Assembly == DataTypesAssembly;
    }

    /// <summary>Names a constructor parameter for a failure message.</summary>
    /// <param name="parameter">The parameter.</param>
    /// <returns>Class, parameter type and parameter name.</returns>
    private static string Describe(ParameterInfo parameter)
    {
      return $"{parameter.Member.DeclaringType.FullName}({parameter.ParameterType.Name} {parameter.Name})";
    }
  }
}
