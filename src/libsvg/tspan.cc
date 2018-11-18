#include "tspan.h"

namespace libsvg {

const std::string tspan::name("tspan"); 

tspan::tspan() : dx(0), dy(0), rotate(0), text_length(0), font_size(0)
{
}

tspan::~tspan()
{
}

void
tspan::set_attrs(attr_map_t& attrs)
{
	shape::set_attrs(attrs);
	this->x = parse_double(attrs["x"]);
	this->y = parse_double(attrs["y"]);
	this->dx = parse_double(attrs["dx"]);
	this->dy = parse_double(attrs["dy"]);
}

void
tspan::dump()
{
	std::cout << get_name()
		<< ": x = " << this->x
		<< ": y = " << this->y
		<< ": dx = " << this->dx
		<< ": dy = " << this->dy
		<< std::endl;
}

}