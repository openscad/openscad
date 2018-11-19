#include <stdlib.h>
#include <iostream>

#include "group.h"

namespace libsvg {

const std::string group::name("g");

group::group()
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

const std::string
group::dump() const
{
	std::stringstream s;
	s << get_name()
		<< ": x = " << this->x
		<< ": y = " << this->y;
	return s.str();
}

}