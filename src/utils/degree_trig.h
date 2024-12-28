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
//
// Trigonometry function taking degrees, accurate for 30, 45, 60 and 90, etc.
//
#pragma once
#include "geometry/linalg.h"

constexpr double M_SQRT3 = 1.73205080756887719318;    /* sqrt(3)   */
constexpr double M_SQRT3_4 = 0.86602540378443859659;  /* sqrt(3/4) == sqrt(3)/2 */
constexpr double M_SQRT1_3 = 0.57735026918962573106;  /* sqrt(1/3) == sqrt(3)/3 */
constexpr double M_RAD2DEG = 57.2957795130823208767;  /* 180/PI */
constexpr double M_DEG2RAD = 0.017453292519943295769; /* PI/180 */

double sin_degrees(double x);
double cos_degrees(double x);
double tan_degrees(double x);
double asin_degrees(double x);
double acos_degrees(double x);
double atan_degrees(double x);
double atan2_degrees(double y, double x);

Matrix3d angle_axis_degrees(double a, Vector3d v);
Matrix3d rotate_degrees(double angle);
