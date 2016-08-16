#include <stdlib.h>
#include <iostream>

#include "svgpage.h"

namespace libsvg {

const std::string svgpage::name("svg"); 

svgpage::svgpage()
{
}

svgpage::svgpage(const svgpage& orig) : shape(orig)
{
	width = orig.width;
	height = orig.height;
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

void
svgpage::dump()
{
	std::cout << get_name()
		<< ": x = " << this->x
		<< ": y = " << this->y
		<< ": width = " << this->width
		<< ": height = " << this->height
		<< std::endl;
}

}