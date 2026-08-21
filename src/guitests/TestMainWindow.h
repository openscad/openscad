#pragma once

#include "UXTest.h"

class TestMainWindow : public UXTest
{
  Q_OBJECT;
private slots:
  void checkOpenTabPropagateToWindow();
  void checkTestResetDoesNotPersistAutoReload();
  void checkOpeningLargeFileDoesNotParseInGui();
  void checkSaveToShouldUpdateWindowTitle();
  void checkEachWindowHasAComputeWorker();
  void checkIsolatedWindowsCanPreviewConcurrently();
  void checkWorkerMessageSeverity();
  void checkProcessIsolationRequiresRestart();
  void checkLegacyModeRendersWithoutComputeWorker();
  void checkUnavailableComputeWorkerDoesNotBlockOrRespawnForever();
  void checkF6UsesComputeWorkerResult();
  void checkRepeatedF6IsServedFromWorkerCache();
  void checkF6UsesCustomizerValues();
  void checkCooperativeCancelKeepsWorker();
  void checkCancelRespawnsWorkerAndPreservesEditor();
  void checkCrashedWorkerRespawns();
  void checkQueuedRequestsAreNotDroppedBeforeWorkerReady();
  void checkWorkerErrorDoesNotMarkSourceRendered();
  void checkPreviewDispatchDoesNotBlockGui();
  void checkIdenticalPreviewRequestIsDebounced();
  void checkEditedPreviewRequestReplacesActivePreview();
  void checkOpenCSGPreparationCanBeCanceled();
  void checkWindowsPrepareOpenCSGConcurrently();
  void checkWorkerCompletionDoesNotFinishPreviewProgress();
  void checkPreviewShowsSeparateGuiProgress();
  void checkPreviewDrawsAfterCanceledOpenCSGPreparation();
  void checkOpenCSGPreparationUsesViewportColorScheme();
  void checkReloadPreviewDispatchDoesNotBlockGui();
  void checkF5UsesComputeWorkerResult();
  void checkPreviewReportsRenderingTime();
  void checkEditedSourcePreviewsOnTheFirstF5();
  void checkF5DuringAnInFlightPreviewShowsTheEditedModel();
  void checkRepeatedEditPreviewCyclesDrawTheEditedModel();
  void checkCustomizerIsUsableAfterAnIsolatedRender();
  void checkUntouchedCustomizerDoesNotOverrideEditedText();
  void checkEditingAPlainTopLevelVariableTakesEffect();
  void checkRightClickAfterIsolatedPreviewDoesNotCrash();
  void checkF6UsesCommandLineDefinitions();
#ifdef ENABLE_PYTHON
  void checkF6UsesTrustedPythonWorker();
#endif
};
