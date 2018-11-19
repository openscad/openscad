#include "text.h"

namespace libsvg {

const std::string text::name("text"); 

text::text() : dx(0), dy(0), rotate(0), text_length(0), font_size(0)
{
}

text::~text()
{
}

void
text::set_attrs(attr_map_t& attrs)
{
	shape::set_attrs(attrs);
	this->x = parse_double(attrs["x"]);
	this->y = parse_double(attrs["y"]);
	this->dx = parse_double(attrs["dx"]);
	this->dy = parse_double(attrs["dy"]);
}

const std::string
text::dump() const
{
	std::stringstream s;
	s << get_name()
		<< ": x = " << this->x
		<< ": y = " << this->y
		<< ": dx = " << this->dx
		<< ": dy = " << this->dy;
	return s.str();
}

}