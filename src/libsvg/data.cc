#include "data.h"

namespace libsvg {

const std::string data::name("data");

data::data()
{
}

data::~data()
{
}

void
data::set_attrs(attr_map_t& attrs)
{
	shape::set_attrs(attrs);
	this->text = attrs["text"];
}

const std::string
data::dump() const
{
	std::stringstream s;
	s << get_name()
		<< ": text = '" << this->text << "'";
	return s.str();
}

}