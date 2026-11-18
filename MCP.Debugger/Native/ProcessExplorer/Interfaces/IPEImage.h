#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "..\ModuleSection.h"
#include "..\ModuleExport.h"
#include "..\ModuleImport.h"

namespace MCP::Native::ProcessExplorer::Interfaces
{
  /// <summary>
  /// The PE image of one module in the memory of a debuggee process, read when the object was created.
  /// </summary>
  /// <remarks>
  /// Threading: not synchronised; used by the thread that opened the image.
  /// Ownership: the object holds its own copy of what it read; the lists it hands out are borrowed and valid
  /// as long as the object.
  /// </remarks>
  class IPEImage
  {
  public:
    virtual ~IPEImage() = default;

    /// <summary>False when the image could not be read completely; the lists may then be incomplete.</summary>
    virtual bool IsValid() const = 0;

    virtual DWORD GetSizeOfImage() const = 0;
    virtual const std::vector<ModuleSection>& GetSections() const = 0;
    virtual const std::vector<ModuleExport>& GetExports() const = 0;
    virtual const std::vector<ModuleImport>& GetImports() const = 0;
  };
}
