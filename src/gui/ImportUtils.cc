#include "gui/ImportUtils.h"

QMap<QString, QString> Importer::knownFileExtensions;

int Importer::init()
{
  const QString importStatement = "import(\"%1\");\n";
  const QString surfaceStatement = "surface(\"%1\");\n";
  const QString importFunction = "data = import(\"%1\");\n";
  knownFileExtensions["stl"] = importStatement;
  knownFileExtensions["obj"] = importStatement;
  knownFileExtensions["3mf"] = importStatement;
  knownFileExtensions["off"] = importStatement;
  knownFileExtensions["dxf"] = importStatement;
  knownFileExtensions["svg"] = importStatement;
  knownFileExtensions["amf"] = importStatement;
  knownFileExtensions["dat"] = surfaceStatement;
  knownFileExtensions["png"] = surfaceStatement;
  knownFileExtensions["json"] = importFunction;
  knownFileExtensions["scad"] = "";
#ifdef ENABLE_PYTHON
  knownFileExtensions["py"] = "";
#endif
  knownFileExtensions["csg"] = "";
  return 0;
}

int forceInit = Importer::init();
