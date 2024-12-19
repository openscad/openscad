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
#include "libsvg/text.h"

#include <sstream>
#include <string>
#include "libsvg/util.h"

namespace libsvg {

const std::string text::name("text");

void
text::set_attrs(attr_map_t& attrs, void *context)
{
  shape::set_attrs(attrs, context);
  this->x = parse_double(attrs["x"]);
  this->y = parse_double(attrs["y"]);
  this->dx = parse_double(attrs["dx"]);
  this->dy = parse_double(attrs["dy"]);
}

const std::string
text::dump() const
{
  std::stringstream s;
  s << get_name()
    << ": x = " << this->x
    << ": y = " << this->y
    << ": dx = " << this->dx
    << ": dy = " << this->dy;
  return s.str();
}

} // namespace libsvg
