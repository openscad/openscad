#pragma once

#include <QMap>
#include <QString>

class Importer
{
public:
  static int init();
  static QMap<QString, QString> knownFileExtensions;
  /** Suffix for opening a path in the editor when QFileInfo::suffix() is empty (e.g. no dot). */
  static QString effectiveSuffixForOpen(const QString& path);
};
