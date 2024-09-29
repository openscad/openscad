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
#include "libsvg/svgpage.h"

#include <sstream>
#include <cstdlib>
#include <string>
#include <iostream>


namespace libsvg {

const std::string svgpage::name("svg");

svgpage::svgpage() : width({0.0, unit_t::UNDEFINED}), height({0.0, unit_t::UNDEFINED})
{
}

void
svgpage::set_attrs(attr_map_t& attrs, void *context)
{
  this->x = 0;
  this->y = 0;
  this->width = parse_length(attrs["width"]);
  this->height = parse_length(attrs["height"]);
  this->viewbox = parse_viewbox(attrs["viewBox"]);
  this->alignment = parse_alignment(attrs["preserveAspectRatio"]);

  const auto *ctx = reinterpret_cast<const fnContext *>(context);
  selected = (ctx->selector) ? ctx->selector(this) : false;
}

const std::string
svgpage::dump() const
{
  std::stringstream s;
  s << get_name()
    << ": x = " << this->x
    << ": y = " << this->y
    << ": width = " << this->width
    << ": height = " << this->height
    << ": viewbox = " << this->viewbox.x
    << "," << this->viewbox.y
    << "," << this->viewbox.width
    << "," << this->viewbox.height
    << (this->viewbox.is_valid ? " (valid)" : " (invalid)")
    << ": alignment = " << this->alignment.x
    << "," << this->alignment.y
    << (this->alignment.meet ? " meet" : " slice");
  return s.str();
}

} // namespace libsvg
