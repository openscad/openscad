#pragma once

#include "UXTest.h"

class TestMainWindow : public UXTest
{
  Q_OBJECT;
private slots:
  void checkOpenTabPropagateToWindow();
  void checkSaveToShouldUpdateWindowTitle();
  void checkChangingColorSchemeRecolorsPreparedPreview();
  void checkChangingColorSchemeRecolorsThrownTogether();
  void checkChangingColorSchemeRecolorsRender();
  //! A repeat preview of an unchanged model with more products than the old fixed cap builds nothing.
  void checkRepeatPreviewOfManyProductsReusesCachedBuffers();
};
