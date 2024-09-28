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
#include "libsvg/use.h"

#include <sstream>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "libsvg/util.h"

namespace libsvg {

const std::string use::name("use");

void
use::set_attrs(attr_map_t& attrs, void *context)
{
  shape::set_attrs(attrs, context);
  this->x = parse_double(attrs["x"]);
  this->y = parse_double(attrs["y"]);

  //Note: width, and height have no effect on use elements, unless the element referenced has a viewbox - i.e. they only have an effect when use refers to a svg or symbol element.
  //Lets store them, but I am not going to use them.
  this->width = parse_double(attrs["width"]);
  this->height = parse_double(attrs["height"]);

  std::string temp_href = attrs["href"];
  if (attrs["href"].empty() && !attrs["xlink:href"].empty()) {
    temp_href = attrs["xlink:href"];
  }

  if (this->href != temp_href) {
    this->href = temp_href;
    if (href.rfind('#', 0) != 0) {
      printf("<use> can only use references to ids in the href field (starting with #). Error in element type %s with id: %s\n", this->get_name().c_str(), this->get_id_or_default().c_str());
    }
  }

  //apply the x/y coordinates to all the children by using a transform
  std::stringstream s;
  s << this->transform << " translate(" << this->x << "," << this->y << ")";
  this->transform = s.str();
}

const std::string
use::get_href_id() const
{
  if (href.rfind('#', 0) != 0) {
    return {};
  }
  return href.substr(1); //remove the #
}

std::vector<std::shared_ptr<shape>>
use::set_clone_child(shape *child)
{
  shape *copy = child->clone();
  copy->set_parent(this);
  auto cloned_objects = copy->clone_children();
  cloned_objects.insert(cloned_objects.begin(), std::shared_ptr<shape>(copy));
  return cloned_objects;
}

const std::string
use::dump() const
{
  std::stringstream s;
  s << get_name()
    << ": x = " << this->x
    << ": y = " << this->y;
  return s.str();
}

} // namespace libsvg
