
#pragma once

#include <QPoint>
#include <QString>
#include "gui/QGLView.h"

enum { MEASURE_IDLE, MEASURE_DIST1, MEASURE_DIST2, MEASURE_ANG1, MEASURE_ANG2, MEASURE_ANG3 };

extern double calculateLinePointDistance(const Vector3d &l1, const Vector3d &l2, const Vector3d &pt, double & dist_lat) ;
extern double calculateLineLineDistance(const Vector3d &l1b, const Vector3d &l1e, const Vector3d &l2b, const Vector3d &l2e, double &dist_lat);
extern double calculateSegSegDistance(const Vector3d &l1b, const Vector3d &l1e, const Vector3d &l2b, const Vector3d &l2e, double &dist_lat);

class Measurement
{
  public:
    Measurement(void);
    void setView(QGLView *qglview);
    QString statemachine(QPoint mouse);
    void startMeasureDist(void);
    void startMeasureAngle(void);
  private:
    QGLView *qglview;
};
