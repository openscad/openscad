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
#include "utils/degree_trig.h"

//
// Trigonometry function taking degrees, accurate for 30, 45, 60 and 90, etc.
//
#include <cmath>
#include <limits>


static inline double rad2deg(double x)
{
  return x * M_RAD2DEG;
}

static inline double deg2rad(double x)
{
  return x * M_DEG2RAD;
}

// this limit assumes 26+26=52 bits mantissa
// comment/undefine it to disable domain check
#define TRIG_HUGE_VAL ((1L << 26) * 360.0 * (1L << 26))

double sin_degrees(double x)
{
  // use positive tests because of possible Inf/NaN
  if (x < 360.0 && x >= 0.0) {
    // Ok for now
  } else
#ifdef TRIG_HUGE_VAL
  if (x < TRIG_HUGE_VAL && x > -TRIG_HUGE_VAL)
#endif
  {
    double revolutions = floor(x / 360.0);
    x -= 360.0 * revolutions;
  }
#ifdef TRIG_HUGE_VAL
  else {
    // total loss of computational accuracy
    // the result would be meaningless
    return std::numeric_limits<double>::quiet_NaN();
  }
#endif
  bool oppose = x >= 180.0;
  if (oppose) x -= 180.0;
  if (x > 90.0) x = 180.0 - x;
  if (x < 45.0) {
    if (x == 30.0) x = 0.5;
    else x = sin(deg2rad(x));
  } else if (x == 45.0) {
    x = M_SQRT1_2;
  } else if (x == 60.0) {
    x = M_SQRT3_4;
  } else { // Inf/Nan would fall here
    x = cos(deg2rad(90.0 - x));
  }
  return oppose ? -x : x;
}

double cos_degrees(double x)
{
  // use positive tests because of possible Inf/NaN
  if (x < 360.0 && x >= 0.0) {
    // Ok for now
  } else
#ifdef TRIG_HUGE_VAL
  if (x < TRIG_HUGE_VAL && x > -TRIG_HUGE_VAL)
#endif
  {
    double revolutions = floor(x / 360.0);
    x -= 360.0 * revolutions;
  }
#ifdef TRIG_HUGE_VAL
  else {
    // total loss of computational accuracy
    // the result would be meaningless
    return std::numeric_limits<double>::quiet_NaN();
  }
#endif
  bool oppose = x >= 180.0;
  if (oppose) x -= 180.0;
  if (x > 90.0) {
    x = 180.0 - x;
    oppose = !oppose;
  }
  if (x > 45.0) {
    if (x == 60.0) x = 0.5;
    else x = sin(deg2rad(90.0 - x));
  } else if (x == 45.0) {
    x = M_SQRT1_2;
  } else if (x == 30.0) {
    x = M_SQRT3_4;
  } else { // Inf/Nan would fall here
    x = cos(deg2rad(x));
  }
  return oppose ? -x : x;
}

double tan_degrees(double x)
{
  int cycles = floor((x) / 180.0);
  // use positive tests because of possible Inf/NaN
  if (x < 180.0 && x >= 0.0) {
    // Ok for now
  } else
#ifdef TRIG_HUGE_VAL
  if (x < TRIG_HUGE_VAL && x > -TRIG_HUGE_VAL)
#endif
  {
    x -= 180.0 * cycles;
  }
#ifdef TRIG_HUGE_VAL
  else {
    // total loss of computational accuracy
    // the result would be meaningless
    return std::numeric_limits<double>::quiet_NaN();
  }
#endif
  bool oppose = x > 90.0;
  if (oppose) x = 180.0 - x;
  if (x == 0.0) {
    x = (cycles % 2) == 0 ? 0.0 : -0.0;
  } else if (x == 30.0) {
    x = M_SQRT1_3;
  } else if (x == 45.0) {
    x = 1.0;
  } else if (x == 60.0) {
    x = M_SQRT3;
  } else if (x == 90.0) {
    x = (cycles % 2) == 0 ?
      std::numeric_limits<double>::infinity() :
      -std::numeric_limits<double>::infinity();
  } else {
    x = tan(deg2rad(x));
  }
  return oppose ? -x : x;
}
//
// Inverse trig
//
double asin_degrees(double x)
{
  const auto degs = rad2deg(asin(x));
  const auto whole = round(degs);
  if (sin_degrees(whole) == x) return whole;
  return degs;
}
double acos_degrees(double x)
{
  const auto degs = rad2deg(acos(x));
  const auto whole = round(degs);
  if (cos_degrees(whole) == x) return whole;
  return degs;
}
double atan_degrees(double x)
{
  const auto degs = rad2deg(atan(x));
  const auto whole = round(degs);
  if (tan_degrees(whole) == x) return whole;
  return degs;
}
double atan2_degrees(double y, double x)
{
  const auto degs = rad2deg(atan2(y, x));
  const auto whole = round(degs);
  if (fabs(degs - whole) < 3.0E-14) return whole;
  return degs;
}
//
// Rotation_matrix_from_axis_and_angle
//
Matrix3d angle_axis_degrees(double a, Vector3d v)
{
  Matrix3d M{Matrix3d::Identity()};
  // Formula from https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
  // We avoid dividing by the square root of the magnitude as much as possible
  // to minimise rounding errors.
  const auto s = sin_degrees(a);
  const auto c = cos_degrees(a);
  const auto m = v.squaredNorm();
  if (m > 0) {
    const Vector3d Cv = v * ((1 - c) / m);
    const Vector3d us = v.normalized() * s;
    M << Cv[0] * v[0] + c,     Cv[1] * v[0] - us[2], Cv[2] * v[0] + us[1],
      Cv[0] * v[1] + us[2], Cv[1] * v[1] + c,     Cv[2] * v[1] - us[0],
      Cv[0] * v[2] - us[1], Cv[1] * v[2] + us[0], Cv[2] * v[2] + c;
  }
  return M;
}
//
// 2D rotation matrix from angle in degrees
//
Matrix3d rotate_degrees(double angle)
{
  Eigen::Matrix3d m;
  const auto s = sin_degrees(angle);
  const auto c = cos_degrees(angle);
  m <<
    c, -s,  0,
    s,  c,  0,
    0,  0,  1;
  return m;
}
