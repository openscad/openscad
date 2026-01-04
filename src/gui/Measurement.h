
#pragma once

#include <QPoint>
#include <QString>
#include "geometry/linalg.h"
#include "gui/QGLView.h"

enum {
  MEASURE_IDLE,
  MEASURE_DIST1,
  MEASURE_DIST2,
  MEASURE_ANG1,
  MEASURE_ANG2,
  MEASURE_ANG3,
  MEASURE_DIRTY
};

struct MeasurementResult {
  enum class Status { NoChange, Success, Error };

  Status status;

  /**
   * Reverse-ordered list of responses.
   */
  std::vector<QString> messages;
};

class Measurement
{
public:
  Measurement(void);
  void setView(QGLView *qglview);

  /**
   * Advance the Measurement state machine.
   * @return When success or error, has reverse-ordered list of responses in `messages`.
   */
  MeasurementResult statemachine(QPoint mouse);

  void startMeasureDist(void);
  void startMeasureAngle(void);

  /**
   * Stops and cleans up measurement, if any in progress.
   * Safe to call multiple times.
   * @return True if measurement was in progress.
   */
  bool stopMeasure();

private:
  QGLView *qglview;
};
