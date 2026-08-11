#pragma once

#include "UXTest.h"

class TestExportImage : public UXTest
{
  Q_OBJECT;
private slots:
  void checkGrabFrameIsOpaqueByDefault();
  void checkTransparentGrabHasTransparentBackground();
  void checkTransparentGrabIsRestoredAfterwards();
  void checkRepeatedGrabsDoNotAccumulate();
  void checkCompositingKeepsTheDefaultGrabOpaqueAndUnchanged();
};
