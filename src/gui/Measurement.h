
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
  MEASURE_HANDLE1,
  MEASURE_DIRTY
};

extern SelectedObject calculateLinePointDistance(const Vector3d& l1, const Vector3d& l2,
                                                 const Vector3d& pt, double& dist_lat);
extern double calculateLineLineDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                                        const Vector3d& l2e, double& dist_lat);
extern SelectedObject calculateSegSegDistance(const Vector3d& l1b, const Vector3d& l1e,
                                              const Vector3d& l2b, const Vector3d& l2e);
extern SelectedObject calculatePointFaceDistance(const Vector3d& pt, const Vector3d& p1,
                                                 const Vector3d& p2, const Vector3d& p3);

class Measurement
{
public:
  Measurement(void);
  void setView(QGLView *qglview);
  std::vector<QString> statemachine(QPoint mouse);
  void startMeasureDist(void);
  void startMeasureAngle(void);
  void startFindHandle(void);
  void stopMeasure();

private:
  QGLView *qglview;
};
