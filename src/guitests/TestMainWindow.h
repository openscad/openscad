#pragma once

#include "UXTest.h"

class TestMainWindow : public UXTest
{
  Q_OBJECT;
private slots:
  void checkOpenTabPropagateToWindow();
  void checkSaveToShouldUpdateWindowTitle();
  void checkEachWindowHasAComputeWorker();
  void checkF6UsesComputeWorkerResult();
  void checkCancelRespawnsWorkerAndPreservesEditor();
  void checkCrashedWorkerRespawns();
};
