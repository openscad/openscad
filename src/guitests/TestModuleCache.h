#pragma once

#include "UXTest.h"

class TestModuleCache : public UXTest
{
  Q_OBJECT;

private slots:
  void testBasicCache();
  void testMCAD();

private:
  QStringList files;
};
