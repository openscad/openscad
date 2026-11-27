#pragma once

#include "UXTest.h"

class TestMainWindow : public UXTest
{
  Q_OBJECT;
private slots:
  void checkOpenTabPropagateToWindow();
  void checkSaveToShouldUpdateWindowTitle();
  void copyColorName_data();
  void copyColorName();
  void restoreFloatingDockVisibility_data();
  void restoreFloatingDockVisibility();
};
