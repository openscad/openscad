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
#pragma once

#include <cmath>
#include <string>
#include "libsvg/shape.h"

namespace libsvg {

class path : public shape
{
protected:
  std::string data;

private:
  [[nodiscard]] inline double t(double t, int exp) const {
    return std::pow(1.0 - t, exp);
  }

  bool is_open_path(path_t& path) const;
  void arc_to(path_t& path, double x, double y, double rx, double ry, double x2, double y2, double angle, bool large, bool sweep, void *context);
  void curve_to(path_t& path, double x, double y, double cx1, double cy1, double x2, double y2, void *context);
  void curve_to(path_t& path, double x, double y, double cx1, double cy1, double cx2, double cy2, double x2, double y2, void *context);

public:
  path() = default;

  void set_attrs(attr_map_t& attrs, void *context) override;
  [[nodiscard]] const std::string dump() const override;
  [[nodiscard]] const std::string& get_name() const override { return path::name; }

  static const std::string name;

  [[nodiscard]] shape *clone() const override { return new path(*this); }
};

} // namespace libsvg
