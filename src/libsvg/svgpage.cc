#include <stdlib.h>
#include <iostream>

#include "svgpage.h"

namespace libsvg {

const std::string svgpage::name("svg"); 

svgpage::svgpage() : width(0), height(0)
{
}

svgpage::~svgpage()
{
}

void
svgpage::set_attrs(attr_map_t& attrs)
{
	this->x = 0;
	this->y = 0;
	this->width = parse_double(attrs["width"]);
	this->height = parse_double(attrs["height"]);
}

const std::string
svgpage::dump() const
{
	std::stringstream s;
	s << get_name()
		<< ": x = " << this->x
		<< ": y = " << this->y
		<< ": width = " << this->width
		<< ": height = " << this->height;
	return s.str();
}

}