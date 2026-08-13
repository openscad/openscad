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
  void checkIsolatedWindowsCanPreviewConcurrently();
  void checkWorkerMessageSeverity();
  void checkProcessIsolationRequiresRestart();
  void checkLegacyModeRendersWithoutComputeWorker();
  void checkUnavailableComputeWorkerDoesNotBlockOrRespawnForever();
  void checkF6UsesComputeWorkerResult();
  void checkF6UsesCustomizerValues();
  void checkCooperativeCancelKeepsWorker();
  void checkCancelRespawnsWorkerAndPreservesEditor();
  void checkCrashedWorkerRespawns();
  void checkWorkerErrorDoesNotMarkSourceRendered();
  void checkPreviewDispatchDoesNotBlockGui();
  void checkOpenCSGPreparationCanBeCanceled();
  void checkPreviewDrawsAfterCanceledOpenCSGPreparation();
  void checkOpenCSGPreparationUsesViewportColorScheme();
  void checkReloadPreviewDispatchDoesNotBlockGui();
  void checkF5UsesComputeWorkerResult();
  void checkF6UsesCommandLineDefinitions();
#ifdef ENABLE_PYTHON
  void checkF6UsesTrustedPythonWorker();
#endif
};
