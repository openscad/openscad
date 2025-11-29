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
#include "gui/Measurement.h"
#include "utils/vector_math.h"

#include <QPoint>
#include <QString>
#include <cmath>
#include <sstream>

Measurement::Measurement() {}

void Measurement::setView(QGLView *qglview)
{
  this->qglview = qglview;
  this->qglview->measure_state = MEASURE_IDLE;
}

void Measurement::startMeasureDistance(void)
{
  this->qglview->selected_obj.clear();
  this->qglview->measure_state = MEASURE_DIST1;
  this->qglview->handle_mode = false;
  this->qglview->update();
}

void Measurement::startMeasureAngle(void)
{
  this->qglview->selected_obj.clear();
  this->qglview->measure_state = MEASURE_ANG1;
  this->qglview->handle_mode = false;
  this->qglview->update();
}

bool Measurement::stopMeasure()
{
  bool ret = qglview->measure_state != MEASURE_DIRTY;
  qglview->selected_obj.clear();
  qglview->shown_obj = nullptr;
  qglview->update();
  qglview->measure_state = MEASURE_IDLE;
  return ret;
}

/**
 * Advance the Measurement state machine.
 * @return When non-empty, is reverse-ordered list of responses
 */
void Measurement::startFindHandle(void)
{
  this->qglview->selected_obj.clear();
  this->qglview->measure_state = MEASURE_HANDLE1;
  this->qglview->handle_mode = true;
  this->qglview->update();
}
std::vector<QString> Measurement::statemachine(QPoint mouse)
{
  if (qglview->measure_state == MEASURE_IDLE || qglview->measure_state == MEASURE_DIRTY) return {};
  qglview->selectPoint(mouse.x(), mouse.y());
  double ang = NAN;
  double dist = NAN;
  SelectedObject obj1, obj2, obj3;
  std::vector<QString> ret;
  SelectedObject ruler = {.type = SelectionType::SELECTION_INVALID};
  auto display_angle = [this](Vector3d p1, Vector3d p2, Vector3d p3, Vector3d p4) {
    SelectedObject ruler;
    Vector3d side1, side2;
    ruler = {
      .type = SelectionType::SELECTION_SEGMENT,
    };
    ruler.pt.push_back(p1);
    ruler.pt.push_back(p2);
    this->qglview->selected_obj.push_back(ruler);
    ruler = {
      .type = SelectionType::SELECTION_SEGMENT,
    };
    ruler.pt.clear();
    ruler.pt.push_back(p3);
    ruler.pt.push_back(p4);
    this->qglview->selected_obj.push_back(ruler);

    side1 = (p2 - p1).normalized();
    side2 = (p4 - p3).normalized();
    double ang = acos(side1.dot(side2)) * 180.0 / G_PI;
    if (!std::isnan(ang)) {
      return QString("Angle  is %1 Degrees").arg(ang);
    }
    qglview->selected_obj.clear();
    qglview->shown_obj = nullptr;
    qglview->update();
    qglview->measure_state = MEASURE_IDLE;
    return QString("Error during angle calculation");
  };

  switch (qglview->measure_state) {
  case MEASURE_DIST1:
    if (qglview->selected_obj.size() == 1) qglview->measure_state = MEASURE_DIST2;
    break;
  case MEASURE_DIST2:
    if (qglview->selected_obj.size() == 2) {
      QString extra;
      ruler.type = SelectionType::SELECTION_INVALID;
      obj1 = qglview->selected_obj[0];
      obj2 = qglview->selected_obj[1];
      if (obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_POINT) {
        const auto diff = obj2.pt[0] - obj1.pt[0];
        dist = diff.norm();
        ret.push_back(QStringLiteral("dx: %1  dy: %2  dz: %3").arg(diff[0]).arg(diff[1]).arg(diff[2]));
      } else if ((obj1.type == SelectionType::SELECTION_POINT &&
                  obj2.type == SelectionType::SELECTION_SEGMENT) ||
                 (obj2.type == SelectionType::SELECTION_POINT &&
                  obj1.type == SelectionType::SELECTION_SEGMENT)) {
        SelectedObject pt = obj1.type == SelectionType::SELECTION_POINT ? obj1 : obj2;
        SelectedObject ln = obj1.type == SelectionType::SELECTION_SEGMENT ? obj1 : obj2;
        const Eigen::Vector3d& P = pt.pt[0];
        const Eigen::Vector3d& A = ln.pt[0];
        const Eigen::Vector3d& B = ln.pt[1];

        // 1. Line direction vector D
        Eigen::Vector3d D = B - A;

        // 2. Vector from A to P (V)
        Eigen::Vector3d V = P - A;

        // Calculate components for projection
        double D_squared_norm = D.squaredNorm();

        if (D_squared_norm > 1e-6) {  // Check if line is not a single point
          // 3. Scalar projection parameter 't'
          double t = V.dot(D) / D_squared_norm;

          // 4. Projection vector V_proj (from A to the closest point C on the line)
          Eigen::Vector3d V_proj = t * D;

          // 5. Shortest distance vector V_dist (P - C)
          Eigen::Vector3d V_dist = V - V_proj;
          ret.push_back(
            QStringLiteral("Perpendicular distance to (infinite) line: dx: %1  dy: %2  dz: %3")
              .arg(V_dist.x())
              .arg(V_dist.y())
              .arg(V_dist.z()));

          // perp_dist = V_dist.norm();
        }

        double dont_care;
        SelectedObject ruler = calculateLinePointDistance(A, B, P, dont_care);
        dist = (ruler.pt[0] - ruler.pt[1]).norm();
        auto diff = B - P;
        ret.push_back(QStringLiteral("Point to Line Endpoint2: dx: %1  dy: %2  dz: %3")
                        .arg(diff[0])
                        .arg(diff[1])
                        .arg(diff[2]));
        auto diff2 = A - P;
        ret.push_back(QStringLiteral("Point to Line Endpoint1: dx: %1  dy: %2  dz: %3")
                        .arg(diff2[0])
                        .arg(diff2[1])
                        .arg(diff2[2]));

      } else if (obj1.type == SelectionType::SELECTION_SEGMENT &&
                 obj2.type == SelectionType::SELECTION_SEGMENT) {
        ruler = calculateSegSegDistance(obj1.pt[0], obj1.pt[1], obj2.pt[0], obj2.pt[1]);
        dist = (ruler.pt[0] - ruler.pt[1]).norm();
      } else if (obj1.type == SelectionType::SELECTION_POINT &&
                 obj2.type == SelectionType::SELECTION_FACE) {
        ruler = calculatePointFaceDistance(obj1.pt[0], obj2.pt[0], obj2.pt[1], obj2.pt[2]);
      } else if (obj1.type == SelectionType::SELECTION_FACE &&
                 obj2.type == SelectionType::SELECTION_POINT) {
        ruler = calculatePointFaceDistance(obj2.pt[0], obj1.pt[0], obj1.pt[1], obj1.pt[2]);
      } else if (obj1.type == SelectionType::SELECTION_FACE &&
                 obj2.type == SelectionType::SELECTION_FACE) {
        Vector3d n1 = (obj1.pt[1] - obj1.pt[0]).cross(obj1.pt[2] - obj1.pt[0]).normalized();
        Vector3d n2 = (obj2.pt[1] - obj2.pt[0]).cross(obj2.pt[2] - obj2.pt[0]).normalized();
        if (fabs(n1.dot(n2)) < 0.999) {
          ret.push_back(QString("Faces are not parallel"));
          break;
        } else {
          ret.push_back("Only coded to handle lines and points; sorry");
        }
      }
      if (std::isnan(dist)) {
        ret.push_back("Got Not-a-Number when calculating distance; sorry");
        return ret;
      }
      ret.push_back(QStringLiteral("Distance is %1").arg(std::fabs(dist)));
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
      if (obj1.type == SelectionType::SELECTION_SEGMENT && obj2.type == SelectionType::SELECTION_POINT) {
        side1 = (obj1.pt[1] - obj1.pt[0]).normalized();
        side2 = (obj1.pt[1] - obj2.pt[0]).normalized();
        ang = acos(side1.dot(side2)) * 180.0 / 3.14159265359;
        goto display_angle;
      } else if (obj1.type == SelectionType::SELECTION_POINT &&
                 obj2.type == SelectionType::SELECTION_SEGMENT) {
        side1 = (obj2.pt[1] - obj2.pt[0]).normalized();
        side2 = (obj2.pt[1] - obj1.pt[0]).normalized();
        ang = acos(side1.dot(side2)) * 180.0 / 3.14159265359;
        goto display_angle;
      } else if (obj1.type == SelectionType::SELECTION_SEGMENT &&
                 obj2.type == SelectionType::SELECTION_SEGMENT) {
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

        permutation(obj1.pt[0], obj1.pt[1], obj2.pt[0], obj2.pt[1]);
        permutation(obj1.pt[1], obj1.pt[0], obj2.pt[0], obj2.pt[1]);
        permutation(obj1.pt[0], obj1.pt[1], obj2.pt[1], obj2.pt[0]);
        permutation(obj1.pt[1], obj1.pt[0], obj2.pt[1], obj2.pt[0]);

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
        Vector3d side1 = (obj2.pt[0] - obj1.pt[0]).normalized();
        Vector3d side2 = (obj2.pt[0] - obj3.pt[0]).normalized();
        ang = acos(side1.dot(side2)) * 180.0 / 3.14159265359;
      } else {
        ret.push_back("If selecting three things, they must all be points");
        return ret;
      }
    display_angle:
      if (std::isnan(ang)) {
        ret.push_back("Got Not-a-Number when calculating angle; sorry");
        return ret;
      }
      ret.push_back(QStringLiteral("Angle is %1 Degrees").arg(ang));
    }
    break;
  }
  return ret;
}
