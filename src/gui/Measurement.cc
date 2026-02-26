/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "geometry/linalg.h"
#include "core/Selection.h"
#include "gui/Measurement.h"
#include "utils/vector_math.h"

#include <cassert>
#include <limits>
#include <QPoint>
#include <QString>
#include <cmath>
#include <sstream>

namespace Measurement {

/**
 * Converts an Eigen::Vector3d to a QString in the format "[x, y, z]".
 * Uses full double precision by default (usually 17 digits).
 * FIXME: I can probably be used more places, should be in a header somewhere.
 */
inline QString Vector3dtoQString(const Eigen::Vector3d& vec,
                                 int precision = std::numeric_limits<double>::max_digits10)
{
  return QString("[%1, %2, %3]")
    .arg(vec.x(), 0, 'g', precision)
    .arg(vec.y(), 0, 'g', precision)
    .arg(vec.z(), 0, 'g', precision);
}

template <typename TView>
Template<TView>::Template()
{
}

template <typename TView>
void Template<TView>::setView(TView *view)
{
  this->qglview = view;
  this->qglview->measure_state = MEASURE_IDLE;
}

template <typename TView>
void Template<TView>::startMeasureDist(void)
{
  this->qglview->selected_obj.clear();
  this->qglview->update();
  this->qglview->measure_state = MEASURE_DIST1;
}

template <typename TView>
void Template<TView>::startMeasureAngle(void)
{
  this->qglview->selected_obj.clear();
  this->qglview->update();
  this->qglview->measure_state = MEASURE_ANG1;
}

template <typename TView>
bool Template<TView>::stopMeasure()
{
  if (qglview->measure_state == MEASURE_IDLE) return false;
  bool ret = qglview->measure_state != MEASURE_DIRTY;
  qglview->selected_obj.clear();
  qglview->shown_obj.clear();
  qglview->update();
  qglview->measure_state = MEASURE_IDLE;
  return ret;
}

template <typename TView>
Result Template<TView>::statemachine(QPoint mouse)
{
  Result ret{Result::Status::NoChange};
  if (qglview->measure_state == MEASURE_IDLE || qglview->measure_state == MEASURE_DIRTY) return ret;
  qglview->selectPoint(mouse.x(), mouse.y());
  double ang = NAN;
  SelectedObject obj1, obj2, obj3;
  switch (qglview->measure_state) {
  case MEASURE_DIST1:
    if (qglview->selected_obj.size() == 1) qglview->measure_state = MEASURE_DIST2;
    break;
  case MEASURE_DIST2:
    if (qglview->selected_obj.size() == 2) {
      obj1 = qglview->selected_obj[0];
      obj2 = qglview->selected_obj[1];
      Distance res = distMeasurement(obj1, obj2);

      auto obj2s = QString::fromStdString(obj2.toString());
      ret.addText(QStringLiteral("Second selection %1 is at %2")
                    .arg(QString::fromStdString(SelectionTypeToString(obj2.type)))
                    .arg(obj2s),
                  obj2s);

      auto obj1s = QString::fromStdString(obj1.toString());
      ret.addText(QStringLiteral("First selection %1 is at %2")
                    .arg(QString::fromStdString(SelectionTypeToString(obj1.type)))
                    .arg(obj1s),
                  obj1s);

      if (res.ptDiff) {
        auto s = Vector3dtoQString(*res.ptDiff);
        ret.addText(QStringLiteral("Δ = %1").arg(s), s);
      }

      if (res.toInfiniteLine) {
        auto s = Vector3dtoQString(*res.toInfiniteLine);
        ret.addText(QStringLiteral("Perpendicular to (infinite) line%1 Δ = %2")
                      .arg(res.line_count == 1 ? "" : "s")
                      .arg(s),
                    s);
      }

      if (res.toEndpoint2) {
        auto s = Vector3dtoQString(*res.toEndpoint2);
        ret.addText(QStringLiteral("Point to Line Endpoint2 Δ = %1").arg(s), s);
      }

      if (res.toEndpoint1) {
        auto s = Vector3dtoQString(*res.toEndpoint1);
        ret.addText(QStringLiteral("Point to Line Endpoint1 Δ = %1").arg(s), s);
      }

      if (std::isnan(res.distance)) {
        ret.addText("Got Not-a-Number when calculating distance; sorry");
        ret.status = Result::Status::Error;
        return ret;
      }
      ret.addText(QStringLiteral("Distance is %1").arg(std::fabs(res.distance)),
                  QStringLiteral("%1").arg(std::fabs(res.distance)));
      ret.status = Result::Status::Success;
    }
    break;
  case MEASURE_ANG1:
    if (qglview->selected_obj.size() == 1) qglview->measure_state = MEASURE_ANG2;
    break;
  case MEASURE_ANG2:
    if (qglview->selected_obj.size() == 2) {
      obj1 = qglview->selected_obj[0];
      obj2 = qglview->selected_obj[1];
      Vector3d side1, side2;
      if (obj1.type == SelectionType::SELECTION_LINE && obj2.type == SelectionType::SELECTION_POINT) {
        side1 = (obj1.p2 - obj1.p1).normalized();
        side2 = (obj1.p2 - obj2.p1).normalized();
        ang = acos(side1.dot(side2)) * 180.0 / 3.14159265359;
        goto display_angle;
      } else if (obj1.type == SelectionType::SELECTION_POINT &&
                 obj2.type == SelectionType::SELECTION_LINE) {
        side1 = (obj2.p2 - obj2.p1).normalized();
        side2 = (obj2.p2 - obj1.p1).normalized();
        ang = acos(side1.dot(side2)) * 180.0 / 3.14159265359;
        goto display_angle;
      } else if (obj1.type == SelectionType::SELECTION_LINE &&
                 obj2.type == SelectionType::SELECTION_LINE) {
        // Check all 4 permutations of the lines' directions and use the one where the starting points
        // are closest to one another as the corner point for the angle
        double nearestDist = INFINITY;
        auto permutation = [&nearestDist, &side1, &side2](const Vector3d& s1s, const Vector3d& s1e,
                                                          const Vector3d& s2s, const Vector3d& s2e) {
          double dist = (s1s - s2s).squaredNorm();
          if (dist < nearestDist) {
            nearestDist = dist;
            side1 = (s1e - s1s).normalized();
            side2 = (s2e - s2s).normalized();
          }
        };

        permutation(obj1.p1, obj1.p2, obj2.p1, obj2.p2);
        permutation(obj1.p2, obj1.p1, obj2.p1, obj2.p2);
        permutation(obj1.p1, obj1.p2, obj2.p2, obj2.p1);
        permutation(obj1.p2, obj1.p1, obj2.p2, obj2.p1);

        ang = acos(side1.dot(side2)) * 180.0 / 3.14159265359;
        goto display_angle;
      } else qglview->measure_state = MEASURE_ANG3;
    }
    break;
  case MEASURE_ANG3:
    if (qglview->selected_obj.size() == 3) {
      obj1 = qglview->selected_obj[0];
      obj2 = qglview->selected_obj[1];
      obj3 = qglview->selected_obj[2];
      if (obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_POINT &&
          obj3.type == SelectionType::SELECTION_POINT) {
        Vector3d side1 = (obj2.p1 - obj1.p1).normalized();
        Vector3d side2 = (obj2.p1 - obj3.p1).normalized();
        ang = acos(side1.dot(side2)) * 180.0 / 3.14159265359;
      } else {
        ret.addText("If selecting three things, they must all be points");
        ret.status = Result::Status::Error;
        return ret;
      }
    display_angle:
      if (std::isnan(ang)) {
        ret.addText("Got Not-a-Number when calculating angle; sorry");
        ret.status = Result::Status::Error;
        return ret;
      }
      ret.addText(QStringLiteral("Angle is %1 Degrees").arg(ang));
      ret.status = Result::Status::Success;
    }
    break;
  }
  return ret;
}

template <typename TView>
typename Template<TView>::Distance Template<TView>::distMeasurement(SelectedObject& obj1,
                                                                    SelectedObject& obj2)
{
  Distance ret{NAN};
  if (obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_POINT) {
    ret.ptDiff = obj2.p1 - obj1.p1;
    ret.distance = ret.ptDiff->norm();
    ret.line_count = 0;
  } else if ((obj1.type == SelectionType::SELECTION_POINT &&
              obj2.type == SelectionType::SELECTION_LINE) ||
             (obj2.type == SelectionType::SELECTION_POINT &&
              obj1.type == SelectionType::SELECTION_LINE)) {
    SelectedObject pt = obj1.type == SelectionType::SELECTION_POINT ? obj1 : obj2;
    SelectedObject ln = obj1.type == SelectionType::SELECTION_LINE ? obj1 : obj2;
    const Eigen::Vector3d& P = pt.p1;
    const Eigen::Vector3d& A = ln.p1;
    const Eigen::Vector3d& B = ln.p2;

    // 1. Line direction vector D
    Eigen::Vector3d D = B - A;

    // 2. Vector from A to P (V)
    Eigen::Vector3d V = P - A;

    // Calculate components for projection
    double D_squared_norm = D.squaredNorm();

    ret.line_count = 0;
    if (D_squared_norm > 1e-6) {  // Check if line is not a single point
      ret.line_count = 1;
      // 3. Scalar projection parameter 't'
      double t = V.dot(D) / D_squared_norm;

      // 4. Projection vector V_proj (from A to the closest point C on the line)
      Eigen::Vector3d V_proj = t * D;

      // 5. Shortest distance vector V_dist (P - C)
      Eigen::Vector3d V_dist = V - V_proj;
      ret.toInfiniteLine = V_dist;
    }

    double dont_care;
    ret.distance = calculateLinePointDistance(A, B, P, dont_care);
    ret.toEndpoint2 = B - P;

    ret.toEndpoint1 = A - P;
  } else if (obj1.type == SelectionType::SELECTION_LINE && obj2.type == SelectionType::SELECTION_LINE) {
    ret.line_count = 2;
    ret.distance = calculateSegSegDistance(obj1.p1, obj1.p2, obj2.p1, obj2.p2);
    double dummy1, sd;
    if (Vector3d inf = calculateLineLineVector(obj1.p1, obj1.p2, obj2.p1, obj2.p2, dummy1, sd);
        !std::isnan(sd) && (inf[0] != 0 || inf[1] != 0 || inf[2] != 0)) {
      ret.toInfiniteLine = inf;
    }
  } else {
    assert("It should not have been possible to select something other than a point and a line" &&
           false);
  }
  return ret;
}

template class Template<QGLView>;
template class Template<FakeGLView>;
};  // namespace Measurement
