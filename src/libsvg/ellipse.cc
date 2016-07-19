#include <stdlib.h>
#include <iostream>

#include "ellipse.h"

namespace libsvg {

const std::string ellipse::name("ellipse"); 

ellipse::ellipse()
{
}

ellipse::ellipse(const ellipse& orig) : shape(orig)
{
	rx = orig.rx;
	ry = orig.ry;
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

void
ellipse::dump()
{
	std::cout << get_name()
		<< ": x = " << this->x
		<< ": y = " << this->y
		<< ": rx = " << this->rx
		<< ": ry = " << this->ry
		<< std::endl;
}

}