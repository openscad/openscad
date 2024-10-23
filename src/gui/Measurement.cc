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

#include "gui/Measurement.h"

#include <QPoint>
#include <QString>
#include <cmath>
#include <sstream>

Measurement::Measurement()
{
}

void Measurement::setView(QGLView *qglview) {
  this->qglview=qglview;
  this->qglview->measure_state = MEASURE_IDLE;
}

void Measurement::startMeasureDist(void)
{
  this->qglview->selected_obj.clear();
  this->qglview->update();
  this->qglview->measure_state=MEASURE_DIST1;
}

void Measurement::startMeasureAngle(void)
{
  this->qglview->selected_obj.clear();
  this->qglview->update();
  this->qglview->measure_state=MEASURE_ANG1;
}
QString Measurement::statemachine(QPoint mouse)
{
  if(qglview->measure_state == MEASURE_IDLE) return "";
  qglview->selectPoint(mouse.x(),mouse.y());
  double ang=NAN;
  double dist=NAN;
  SelectedObject obj1, obj2, obj3;
  switch(qglview->measure_state) {
      case MEASURE_DIST1:
      if(qglview->selected_obj.size() == 1) qglview->measure_state = MEASURE_DIST2;
      break;
      case MEASURE_DIST2:
      if(qglview->selected_obj.size() == 2)
      {
        double lat;
        obj1=qglview->selected_obj[0];
        obj2=qglview->selected_obj[1];
        if(obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_POINT) dist =(obj2.p1-obj1.p1).norm();
        if(obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_LINE) dist =calculateLinePointDistance(obj2.p1, obj2.p2,obj1.p1,lat);
        if(obj1.type == SelectionType::SELECTION_LINE && obj2.type == SelectionType::SELECTION_POINT) dist =calculateLinePointDistance(obj1.p1, obj1.p2,obj2.p1,lat);
        if(obj1.type == SelectionType::SELECTION_LINE && obj2.type == SelectionType::SELECTION_LINE) dist =calculateSegSegDistance(obj1.p1, obj1.p2,obj2.p1,obj2.p2,lat);
        if(!std::isnan(dist)) {
          return QString("Distance is %1").arg(fabs(dist));
        }
        qglview->selected_obj.clear();
        qglview->shown_obj.clear();
        qglview->update();
        qglview->measure_state = MEASURE_IDLE;
      }
      break;
      case MEASURE_ANG1:
      if(qglview->selected_obj.size() == 1) qglview->measure_state = MEASURE_ANG2;
      break;
      case MEASURE_ANG2:
      if(qglview->selected_obj.size() == 2)
      {
        obj1=qglview->selected_obj[0];
        obj2=qglview->selected_obj[1];
        Vector3d side1, side2;
        if(obj1.type == SelectionType::SELECTION_LINE && obj2.type == SelectionType::SELECTION_POINT)
        {
          side1=(obj1.p2-obj1.p1).normalized();
          side2=(obj1.p2-obj2.p1).normalized();
          ang=acos(side1.dot(side2))*180.0/3.14159265359;
          goto display_angle;
        }
        else if(obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_LINE)
        {
          side1=(obj2.p2-obj2.p1).normalized();
          side2=(obj2.p2-obj1.p1).normalized();
          ang=acos(side1.dot(side2))*180.0/3.14159265359;
          goto display_angle;
        }
        else if(obj1.type == SelectionType::SELECTION_LINE && obj2.type == SelectionType::SELECTION_LINE)
        {
          if(obj1.p2 == obj2.p1) {
            side1=(obj2.p1-obj1.p1).normalized();
            side2=(obj2.p1-obj2.p2).normalized();
          }
          else if(obj2.p2 == obj1.p1) {
            side1=(obj1.p1-obj2.p1).normalized();
            side2=(obj1.p1-obj1.p2).normalized();
          } else {
            side1=(obj1.p2-obj1.p1).normalized();
            side2=(obj2.p2-obj2.p1).normalized();
          }
          ang=acos(side1.dot(side2))*180.0/3.14159265359;
          goto display_angle;
        } else
          qglview->measure_state = MEASURE_ANG3;
      }
      break;
      case MEASURE_ANG3:
      if(qglview->selected_obj.size() == 3){
        obj1=qglview->selected_obj[0];
        obj2=qglview->selected_obj[1];
        obj3=qglview->selected_obj[2];
        if(obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_POINT && obj3.type == SelectionType::SELECTION_POINT)
        {
          Vector3d side1=(obj2.p1-obj1.p1).normalized();
          Vector3d side2=(obj2.p1-obj3.p1).normalized();
          ang=acos(side1.dot(side2))*180.0/3.14159265359;
        }
display_angle:
        if(!std::isnan(ang))
        {
          return QString("Angle  is %1 Degrees").arg(ang);
        }
        qglview->selected_obj.clear();
        qglview->shown_obj.clear();
        qglview->update();
        qglview->measure_state = MEASURE_IDLE;
      }
      break;
  }
  return "";
}
