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
#include "linalg.h"

// increased precision for intermediate calculations involving trig
typedef Eigen::Matrix<long double, 3, 1> Vector3ld;

// long double constants, determined to precision of 50 digits using PARI/GP `\p 50`
constexpr long double LM_SQRT3   = 1.7320508075688772935274463415058723669428052538104L;  /* sqrt(3)   */
constexpr long double LM_SQRT3_4 = 0.86602540378443864676372317075293618347140262690519L; /* sqrt(3/4) == sqrt(3) / 2 */
constexpr long double LM_SQRT1_3 = 0.57735026918962576450914878050195745564760175127013L; /* sqrt(1/3) == sqrt(3) / 3 == 1 / sqrt(3) */

constexpr long double LM_PI      = 3.1415926535897932384626433832795028841971693993751L;   /* PI */
constexpr long double LM_RAD2DEG = 57.295779513082320876798154814105170332405472466564L;  /* 180/PI */
constexpr long double LM_DEG2RAD = 0.017453292519943295769236907684886127134428718885417L;/* PI/180 */

constexpr long double LM_SQRT1_2 = 0.70710678118654752440084436210484903928483593768847L; /* sqrt(1/2) == 1 / sqrt(2) */

inline long double rad2deg(long double x)
{
  return x * LM_RAD2DEG;
}

inline long double deg2rad(long double x)
{
  return x * LM_DEG2RAD;
}

long double sin_degrees(long double x);
long double cos_degrees(long double x);
long double tan_degrees(long double x);
long double asin_degrees(long double x);
long double acos_degrees(long double x);
long double atan_degrees(long double x);
long double atan2_degrees(long double y, long double x);

Matrix3d angle_axis_degrees(double a, Vector3d v);
Matrix3d rotate_degrees(double angle);
