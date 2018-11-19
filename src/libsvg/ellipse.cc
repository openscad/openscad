#include <stdlib.h>
#include <iostream>

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