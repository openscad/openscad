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
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>

#include "degree_trig.h"

// this limit assumes 26+26=52 bits mantissa
// comment/undefine it to disable domain check
#define TRIG_HUGE_VAL ((1L << 26) * 360.0L * (1L << 26))

long double sin_degrees(long double x)
{
  // use positive tests because of possible Inf/NaN
  if (x < 360.0L && x >= 0.0L) {
    // Ok for now
  } else
#ifdef TRIG_HUGE_VAL
  if (x < TRIG_HUGE_VAL && x > -TRIG_HUGE_VAL)
#endif
  {
    long double revolutions = floorl(x / 360.0L);
    x = std::fmal(-360.0L, revolutions, x);
  }
#ifdef TRIG_HUGE_VAL
  else {
    // total loss of computational accuracy
    // the result would be meaningless
    return std::numeric_limits<double>::quiet_NaN();
  }
#endif
  bool oppose = x >= 180.0L;
  if (oppose) x -= 180.0L;
  if (x > 90.0L) x = 180.0L - x;
  if (x < 45.0L) {
    if (x == 30.0L) x = 0.5L;
    else x = sinl(deg2rad(x));
  } else if (x == 45.0L) {
    x = LM_SQRT1_2;
  } else if (x == 60.0L) {
    x = LM_SQRT3_4;
  } else { // Inf/Nan would fall here
    x = cos(deg2rad(90.0L - x));
  }
  return oppose ? -x : x;
}

long double cos_degrees(long double x)
{
  // use positive tests because of possible Inf/NaN
  if (x < 360.0L && x >= 0.0L) {
    // Ok for now
  } else
#ifdef TRIG_HUGE_VAL
  if (x < TRIG_HUGE_VAL && x > -TRIG_HUGE_VAL)
#endif
  {
    long double revolutions = floorl(x / 360.0L);
    x = std::fmal(-360.0L, revolutions, x);
  }
#ifdef TRIG_HUGE_VAL
  else {
    // total loss of computational accuracy
    // the result would be meaningless
    return std::numeric_limits<double>::quiet_NaN();
  }
#endif
  bool oppose = x >= 180.0L;
  if (oppose) x -= 180.0L;
  if (x > 90.0L) {
    x = 180.0L - x;
    oppose = !oppose;
  }
  if (x > 45.0L) {
    if (x == 60.0L) x = 0.5L;
    else x = sinl(deg2rad(90.0L - x));
  } else if (x == 45.0L) {
    x = LM_SQRT1_2;
  } else if (x == 30.0L) {
    x = LM_SQRT3_4;
  } else { // Inf/Nan would fall here
    x = cosl(deg2rad(x));
  }
  return oppose ? -x : x;
}

long double tan_degrees(long double x)
{
  int cycles = floorl(x / 180.0L);
  // use positive tests because of possible Inf/NaN
  if (x < 180.0L && x >= 0.0L) {
    // Ok for now
  } else
#ifdef TRIG_HUGE_VAL
  if (x < TRIG_HUGE_VAL && x > -TRIG_HUGE_VAL)
#endif
  {
    x -= 180.0L * cycles;
  }
#ifdef TRIG_HUGE_VAL
  else {
    // total loss of computational accuracy
    // the result would be meaningless
    return std::numeric_limits<double>::quiet_NaN();
  }
#endif
  bool oppose = x > 90.0L;
  if (oppose) x = 180.0L - x;
  if (x == 0.0L) {
    x = (cycles % 2) == 0L ? 0.0L : -0.0L;
  } else if (x == 30.0L) {
    x = LM_SQRT1_3;
  } else if (x == 45.0L) {
    x = 1.0L;
  } else if (x == 60.0L) {
    x = LM_SQRT3;
  } else if (x == 90.0L) {
    x = (cycles % 2) == 0 ?
      std::numeric_limits<double>::infinity() :
      -std::numeric_limits<double>::infinity();
  } else {
    x = tanl(deg2rad(x));
  }
  return oppose ? -x : x;
}
//
// Inverse trig
//
long double asin_degrees(long double x)
{
  const auto degs = rad2deg(asinl(x));
  const auto whole = roundl(degs);
  return (double(sin_degrees(whole)) == double(x)) ? whole : degs;
}
long double acos_degrees(long double x)
{
  const auto degs = rad2deg(acosl(x));
  const auto whole = roundl(degs);
  return (double(cos_degrees(whole)) == double(x)) ? whole : degs;
}
long double atan_degrees(long double x)
{
  const auto degs = rad2deg(atanl(x));
  const auto whole = roundl(degs);
  return (double(tan_degrees(whole)) == double(x)) ? whole : degs;
}
long double atan2_degrees(long double y, long double x)
{
  const auto degs = rad2deg(atan2l(y, x));
  const auto whole = roundl(degs);
  if (fabsl(degs - whole) < 3.0E-14L) return whole;
  return degs;
}
//
// Rotation_matrix_from_axis_and_angle
//
Matrix3d angle_axis_degrees(double a, Vector3d _v)
{
  Vector3ld v = _v.cast<long double>();
  Matrix3d M{Matrix3d::Identity()};
  // Formula from https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
  // We avoid dividing by the square root of the magnitude as much as possible
  // to minimise rounding errors.
  const auto s = sin_degrees(a);
  const auto c = cos_degrees(a);
  const auto m = std::fmal(v[0], v[0], std::fmal(v[1], v[1], std::fmal(v[2], v[2], 0) ) );
  if (m > 0) {
    const Vector3ld Cv = v * ((1.0L - c) / m);
    const Vector3ld us = v.normalized() * s;
    M << std::fmal(Cv[0], v[0],      c), std::fmal(Cv[1], v[0], -us[2]), std::fmal(Cv[2], v[0],  us[1]),
         std::fmal(Cv[0], v[1],  us[2]), std::fmal(Cv[1], v[1],      c), std::fmal(Cv[2], v[1], -us[0]),
         std::fmal(Cv[0], v[2], -us[1]), std::fmal(Cv[1], v[2],  us[0]), std::fmal(Cv[2], v[2],      c);
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
