#pragma once

#include <QMap>
#include <QString>

class Importer
{
public:
  static int init();
  static QMap<QString, QString> knownFileExtensions;
};
