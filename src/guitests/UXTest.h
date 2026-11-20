#pragma once

#include <QObject>
#include <QString>

#include "gui/MainWindow.h"

class UXTest : public QObject
{
  Q_OBJECT;

public:
  void setWindow(MainWindow *window);

protected:
  // Test fixtures live under tests/data/ in the source tree, and are copied into
  // Contents/Resources/tests/data/ of the macOS bundle. Both layouts are reached
  // through resourceBasePath(), so every test must go through here.
  static QString fixturePath(const QString& relative);

  void restoreWindowInitialState();

  MainWindow *window;
};
