/*
 * The MIT License
 *
 * Copyright (c) 2016-2018, Torsten Paul <torsten.paul@gmx.de>,
 *                          Marius Kintel <marius@kintel.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "libsvg/transformation.h"

#include <sstream>
#include <string>
#include <vector>
#include <iostream>

#include "libsvg/util.h"
#include "utils/degree_trig.h"

namespace libsvg {

void
transformation::add_arg(const std::string& arg)
{
  double d = parse_double(arg);
  args.push_back(d);
}

const std::string
transformation::get_args() const {
  std::ostringstream str;
  for (unsigned int a = 0; a < args.size(); ++a) {
    str << ((a == 0) ? "(" : ", ") << args[a];
  }
  str << ")";
  return str.str();
}

matrix::matrix() : transformation("m", "matrix")
{
}

/**
 * matrix(<a> <b> <c> <d> <e> <f>), which specifies a transformation in
 * the form of a transformation matrix of six values. matrix(a,b,c,d,e,f)
 * is equivalent to applying the transformation matrix [a b c d e f].
 */
std::vector<Eigen::Matrix3d>
matrix::get_matrices()
{
  if (args.size() != 6) {
    std::cout << "invalid arguments for matrix" << std::endl;
    return {};
  }

  Eigen::Matrix3d m;
  m <<
    args[0], args[2], args[4],
    args[1], args[3], args[5],
    0, 0, 1;

  std::vector<Eigen::Matrix3d> result;
  result.push_back(m);
  return result;
}

translate::translate() : transformation("t", "translate")
{
}

/**
 * translate(<tx> [<ty>]), which specifies a translation by tx and ty.
 * If <ty> is not provided, it is assumed to be zero.
 */
std::vector<Eigen::Matrix3d>
translate::get_matrices()
{
  if ((args.size() < 1) || (args.size() > 2)) {
    std::cout << "invalid arguments for " << get_name() << std::endl;
    return {};
  }

  double tx = args[0];
  double ty = args.size() > 1 ? args[1] : 0;

  Eigen::Matrix3d m;
  m <<
    1, 0, tx,
    0, 1, ty,
    0, 0, 1;

  std::vector<Eigen::Matrix3d> result;
  result.push_back(m);
  return result;
}

scale::scale() : transformation("s", "scale")
{
}

/**
 * scale(<sx> [<sy>]), which specifies a scale operation by sx and sy.
 * If <sy> is not provided, it is assumed to be equal to <sx>.
 */
std::vector<Eigen::Matrix3d>
scale::get_matrices()
{
  if ((args.size() < 1) || (args.size() > 2)) {
    std::cout << "invalid arguments for " << get_name() << std::endl;
    return {};
  }

  double sx = args[0];
  double sy = args.size() > 1 ? args[1] : args[0];

  Eigen::Matrix3d m;
  m <<
    sx,  0, 0,
    0, sy, 0,
    0,  0, 1;

  std::vector<Eigen::Matrix3d> result;
  result.push_back(m);
  return result;
}

rotate::rotate() : transformation("r", "rotate")
{
}

/**
 * rotate(<rotate-angle> [<cx> <cy>]), which specifies a rotation by
 * <rotate-angle> degrees about a given point.
 * If optional parameters <cx> and <cy> are not supplied, the rotate is
 * about the origin of the current user coordinate system. The operation
 * corresponds to the matrix [cos(a) sin(a) -sin(a) cos(a) 0 0].
 * If optional parameters <cx> and <cy> are supplied, the rotate is
 * about the point (cx, cy). The operation represents the equivalent of
 * the following specification: translate(<cx>, <cy>) rotate(<rotate-angle>)
 * translate(-<cx>, -<cy>).
 */
std::vector<Eigen::Matrix3d>
rotate::get_matrices()
{
  if ((args.size() != 1) && (args.size() != 3)) {
    std::cout << "invalid arguments for " << get_name() << std::endl;
    return {};
  }

  bool has_center = args.size() == 3;

  double angle = args[0];
  double cx = has_center ? args[1] : 0;
  double cy = has_center ? args[2] : 0;

  std::vector<Eigen::Matrix3d> result;

  if (has_center) {
    Eigen::Matrix3d t;
    t <<
      1, 0, -cx,
      0, 1, -cy,
      0, 0, 1;
    result.push_back(t);
  }

  result.push_back(rotate_degrees(angle));

  if (has_center) {
    Eigen::Matrix3d t;
    t <<
      1, 0, cx,
      0, 1, cy,
      0, 0, 1;
    result.push_back(t);
  }

  return result;
}

skew_x::skew_x() : transformation("x", "skew_x")
{
}

/**
 * skewX(<skew-angle>), which specifies a skew transformation along the x-axis.
 */
std::vector<Eigen::Matrix3d>
skew_x::get_matrices()
{
  if (args.size() != 1) {
    std::cout << "invalid arguments for " << get_name() << std::endl;
    return {};
  }

  double angle = args[0];

  Eigen::Matrix3d m;
  m <<
    1, tan_degrees(angle), 0,
    0, 1,                  0,
    0, 0,                  1;

  std::vector<Eigen::Matrix3d> result;
  result.push_back(m);
  return result;
}

skew_y::skew_y() : transformation("y", "skew_y")
{
}

/**
 * skewY(<skew-angle>), which specifies a skew transformation along the y-axis.
 */
std::vector<Eigen::Matrix3d>
skew_y::get_matrices()
{
  if (args.size() != 1) {
    std::cout << "invalid arguments for " << get_name() << std::endl;
    return {};
  }

  double angle = args[0];

  Eigen::Matrix3d m;
  m <<
    1, 0, 0,
    tan_degrees(angle), 1, 0,
    0, 0, 1;

  std::vector<Eigen::Matrix3d> result;
  result.push_back(m);
  return result;
}

} // namespace libsvg
