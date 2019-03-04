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
#include <stdlib.h>
#include <nowide/iostream.hpp>

#include "ellipse.h"

namespace libsvg {

const std::string ellipse::name("ellipse"); 

ellipse::ellipse() : rx(0), ry(0)
{
}

ellipse::~ellipse()
{
}

void
ellipse::set_attrs(attr_map_t& attrs)
{
	shape::set_attrs(attrs);
	this->x = parse_double(attrs["cx"]);
	this->y = parse_double(attrs["cy"]);
	this->rx = parse_double(attrs["rx"]);
	this->ry = parse_double(attrs["ry"]);

	path_t path;
	draw_ellipse(path, get_x(), get_y(), get_radius_x(), get_radius_y());
	path_list.push_back(path);
}

const std::string
ellipse::dump() const
{
	std::stringstream s;
	s << get_name()
		<< ": x = " << this->x
		<< ": y = " << this->y
		<< ": rx = " << this->rx
		<< ": ry = " << this->ry;
	return s.str();
}

}