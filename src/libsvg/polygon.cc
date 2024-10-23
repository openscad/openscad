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
#include "libsvg/polygon.h"

#include <boost/tokenizer.hpp>

#include "libsvg/util.h"

#include <string>

namespace libsvg {

const std::string polygon::name("polygon");

void
polygon::set_attrs(attr_map_t& attrs, void *context)
{
  shape::set_attrs(attrs, context);
  this->points = attrs["points"];

  using tokenizer = boost::tokenizer<boost::char_separator<char>>;
  boost::char_separator<char> sep(" ,");
  tokenizer tokens(this->points, sep);

  double x = 0.0;
  path_t path;
  bool first = true;
  for (const auto& v : tokens) {
    double p = parse_double(v);

    if (first) {
      x = p;
    } else {
      path.push_back(Eigen::Vector3d(x, p, 0));
    }
    first = !first;
  }
  if (!path.empty()) {
    path.push_back(path[0]);
  }
  path_list.push_back(path);
}

} // namespace libsvg
