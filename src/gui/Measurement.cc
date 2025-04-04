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

void Measurement::stopMeasure()
{
  qglview->selected_obj.clear();
  qglview->shown_obj = nullptr;
  qglview->update();
  qglview->measure_state = MEASURE_IDLE;
}

void Measurement::startFindHandle(void)
{
  this->qglview->selected_obj.clear();
  this->qglview->update();
  this->qglview->measure_state=MEASURE_HANDLE1;
}
QString Measurement::statemachine(QPoint mouse) 
{
  if(qglview->measure_state == MEASURE_IDLE) return {};
  qglview->selectPoint(mouse.x(),mouse.y());
  double dist=NAN;
  SelectedObject obj1, obj2, obj3;
  SelectedObject ruler = {
	  .type=SelectionType::SELECTION_INVALID
  };
  auto display_angle = [this] (Vector3d p1, Vector3d p2, Vector3d p3, Vector3d p4) 
  {	
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

        side1=(p2-p1).normalized();
        side2=(p4-p3).normalized();
        double ang=acos(side1.dot(side2))*180.0/G_PI;
        if(!std::isnan(ang))
        {
          return QString("Angle  is %1 Degrees").arg(ang);
        }
        qglview->selected_obj.clear();
        qglview->shown_obj = nullptr;
        qglview->update();
        qglview->measure_state = MEASURE_IDLE;
        return QString("Error during angle calculation");
  };

  switch(qglview->measure_state) {
      case MEASURE_DIST1:
      if(qglview->selected_obj.size() == 1) qglview->measure_state = MEASURE_DIST2;
      break;
      case MEASURE_DIST2:
      if(qglview->selected_obj.size() == 2)
      {
        double lat;
	ruler.type = SelectionType::SELECTION_INVALID;
        obj1=qglview->selected_obj[0];
        obj2=qglview->selected_obj[1];
        if(obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_POINT){
	  ruler = {
            .type = SelectionType::SELECTION_SEGMENT,
          };
	  ruler.pt.push_back(obj1.pt[0]);
	  ruler.pt.push_back(obj2.pt[0]);
	}  
        if(obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_SEGMENT){
	  ruler =calculateLinePointDistance(obj2.pt[0], obj2.pt[1],obj1.pt[0], lat);
	}  
        if(obj1.type == SelectionType::SELECTION_SEGMENT && obj2.type == SelectionType::SELECTION_POINT){
	  ruler =calculateLinePointDistance(obj1.pt[0], obj1.pt[1],obj2.pt[0], lat);
	}  
        if(obj1.type == SelectionType::SELECTION_SEGMENT && obj2.type == SelectionType::SELECTION_SEGMENT){
          ruler =calculateSegSegDistance(obj1.pt[0], obj1.pt[1],obj2.pt[0],obj2.pt[1]);
	}
        if(obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_FACE){
          ruler =calculatePointFaceDistance(obj1.pt[0], obj2.pt[0], obj2.pt[1], obj2.pt[2]);
	}
        if(obj1.type == SelectionType::SELECTION_FACE && obj2.type == SelectionType::SELECTION_POINT){
          ruler =calculatePointFaceDistance(obj2.pt[0], obj1.pt[0], obj1.pt[1], obj1.pt[2]);
	}
        if(obj1.type == SelectionType::SELECTION_FACE && obj2.type == SelectionType::SELECTION_FACE){
	  Vector3d n1=(obj1.pt[1]-obj1.pt[0]).cross(obj1.pt[2]-obj1.pt[0]).normalized();
	  Vector3d n2=(obj2.pt[1]-obj2.pt[0]).cross(obj2.pt[2]-obj2.pt[0]).normalized();
	  if(fabs(n1.dot(n2)) < 0.999)
          	return QString("Faces are not parallel");
          ruler =calculatePointFaceDistance(obj1.pt[0], obj2.pt[0], obj2.pt[1], obj2.pt[2]);
	}

        if(ruler.type != SelectionType::SELECTION_INVALID) {
	  dist =(ruler.pt[1]-ruler.pt[0]).norm();
	  qglview->selected_obj.push_back(ruler);
          return QString("Distance is %1").arg(fabs(dist));
        }
        stopMeasure();
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
        if(obj1.type == SelectionType::SELECTION_SEGMENT && obj2.type == SelectionType::SELECTION_POINT)
	  return display_angle(obj1.pt[0], obj1.pt[1], obj1.pt[1], obj2.pt[0]);
        else if(obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_SEGMENT)
          return display_angle(obj1.pt[0],obj2.pt[0],obj2.pt[0],obj2.pt[1]);
        else if(obj1.type == SelectionType::SELECTION_SEGMENT && obj2.type == SelectionType::SELECTION_SEGMENT)
	  return display_angle(obj1.pt[0],obj1.pt[1],obj2.pt[0],obj2.pt[1]);
        else if(obj1.type == SelectionType::SELECTION_FACE && obj2.type == SelectionType::SELECTION_FACE) {
	  Vector3d n1=(obj1.pt[1]-obj1.pt[0]).cross(obj1.pt[2]-obj1.pt[0]).normalized();
	  Vector3d n2=(obj2.pt[1]-obj2.pt[0]).cross(obj2.pt[2]-obj2.pt[0]).normalized();
          double ang=180-acos(n1.dot(n2))*180.0/G_PI;
          return QString("Angle  is %1 Degrees").arg(ang);
	}
        else qglview->measure_state = MEASURE_ANG3;
      }
      break;
      case MEASURE_ANG3:
      if(qglview->selected_obj.size() == 3){
        obj1=qglview->selected_obj[0];
        obj2=qglview->selected_obj[1];
        obj3=qglview->selected_obj[2];
        if(obj1.type == SelectionType::SELECTION_POINT && obj2.type == SelectionType::SELECTION_POINT && obj3.type == SelectionType::SELECTION_POINT)
	  return display_angle(obj1.pt[0],obj2.pt[0],obj2.pt[0],obj3.pt[0]);
	break;
      }
      break;
  }
  return "";
}
