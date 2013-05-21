#include "PlatformUtils.h"
#include "boosty.h"

std::string PlatformUtils::libraryPath()
{
  return boosty::stringy(fs::path(PlatformUtils::documentsPath()) / "OpenSCAD" / "libraries");
}
