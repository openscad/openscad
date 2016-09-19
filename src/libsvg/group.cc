#include <stdlib.h>
#include <iostream>

#include "group.h"

namespace libsvg {

const std::string group::name("g");

group::group()
{
}

group::group(const group& orig) : shape(orig)
{
}

group::~group()
{
}

void
group::set_attrs(attr_map_t& attrs)
{
	shape::set_attrs(attrs);
}

void
group::dump()
{
	std::cout << get_name()
		<< ": x = " << this->x
		<< ": y = " << this->y
		<< std::endl;
}

}