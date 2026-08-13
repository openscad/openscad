#pragma once

#include "UXTest.h"

class TestMainWindow : public UXTest
{
  Q_OBJECT;
private slots:
  void checkOpenTabPropagateToWindow();
  void checkOpeningLargeFileDoesNotParseInGui();
  void checkSaveToShouldUpdateWindowTitle();
  void checkEachWindowHasAComputeWorker();
  void checkProcessIsolationRequiresRestart();
  void checkF6UsesComputeWorkerResult();
  void checkF6UsesCustomizerValues();
  void checkCooperativeCancelKeepsWorker();
  void checkCancelRespawnsWorkerAndPreservesEditor();
  void checkCrashedWorkerRespawns();
  void checkWorkerErrorDoesNotMarkSourceRendered();
  void checkPreviewDispatchDoesNotBlockGui();
  void checkReloadPreviewDispatchDoesNotBlockGui();
  void checkF5UsesComputeWorkerResult();
  void checkF6UsesCommandLineDefinitions();
#ifdef ENABLE_PYTHON
  void checkF6UsesTrustedPythonWorker();
#endif
};
