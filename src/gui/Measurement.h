
#pragma once

#include <QPoint>
#include <QString>
#include <optional>
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

class Measurement
{
public:
  Measurement(void);
  void setView(QGLView *qglview);

  struct Distance {
    double distance;
    int line_count;
    QString codingError;
    std::optional<Eigen::Vector3d> ptDiff, toInfiniteLine, toEndpoint1, toEndpoint2;
  };

  struct Result {
    enum class Status { NoChange, Success, Error };

    Status status;

    /**
     * Reverse-ordered list of responses.
     */
    std::vector<QString> messages;
  };

  /**
   * Advance the Measurement state machine.
   * @return When success or error, has reverse-ordered list of responses in `messages`.
   */
  Result statemachine(QPoint mouse);

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
  Distance distMeasurement(SelectedObject& obj1, SelectedObject& obj2);
  friend class MeasurementTest;
  // Should break out angle measurement for testing too!
  // Then un-reverse the messages because it's entirely unnecessary once all values are accessible.
};
