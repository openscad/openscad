#include "gui/ImportUtils.h"

#include <QFileInfo>
#include <QString>

QMap<QString, QString> Importer::knownFileExtensions;

int Importer::init()
{
  const QString importStatement = "from openscad import *\nmodel=osimport(\"%1\");\nmodel.show()\n";
  const QString surfaceStatement = "surface(\"%1\");\n";
  const QString importFunction = "data = import(\"%1\");\n";
  knownFileExtensions["stl"] = importStatement;
  knownFileExtensions["step"] = importStatement;
  knownFileExtensions["stp"] = importStatement;
  knownFileExtensions["obj"] = importStatement;
  knownFileExtensions["3mf"] = importStatement;
  knownFileExtensions["off"] = importStatement;
  knownFileExtensions["dxf"] = importStatement;
  knownFileExtensions["svg"] = importStatement;
  knownFileExtensions["cdr"] = importStatement;
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

QString Importer::effectiveSuffixForOpen(const QString& path)
{
  const QFileInfo fileinfo(path);
  QString suffix = fileinfo.suffix().toLower();
  if (knownFileExtensions.contains(suffix)) {
    return suffix;
  }
  const QString absLower = fileinfo.absoluteFilePath().toLower();
  QString bestExt;
  int bestLen = 0;
  for (auto it = knownFileExtensions.constBegin(); it != knownFileExtensions.constEnd(); ++it) {
    const QString& ext = it.key();
    if (ext.isEmpty()) {
      continue;
    }
    const QString tail = QLatin1Char('.') + ext;
    if (absLower.endsWith(tail) && tail.size() > bestLen) {
      bestLen = tail.size();
      bestExt = ext;
    }
  }
  return bestExt;
}

int forceInit = Importer::init();
