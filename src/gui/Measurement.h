
#pragma once

#include <vector>
#include <QPoint>
#include <QString>
#include "geometry/linalg.h"
#include "core/Selection.h"
#include "gui/QGLView.h"

namespace Measurement {

enum {
  MEASURE_IDLE,
  MEASURE_DIST1,
  MEASURE_DIST2,
  MEASURE_ANG1,
  MEASURE_ANG2,
  MEASURE_ANG3,
  MEASURE_HANDLE1,
  MEASURE_DIRTY
};

class Measurement
{
public:
  Measurement(void);
  void setView(QGLView *qglview);

  /**
   * Advance the Measurement state machine.
   * @return When non-empty, is reverse-ordered list of responses. Errors always produce response(s).
   */
  std::vector<QString> statemachine(QPoint mouse);

  void startMeasureDistance(void);
  void startMeasureAngle(void);
  void startFindHandle(void);

  /**
   * Stops and cleans up measurement, if any in progress.
   * Safe to call multiple times.
   * @return True if measurement was in progress.
   */
  bool stopMeasure();

private:
  QGLView *qglview;
};

};  // namespace Measurement
