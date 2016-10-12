#include "line.h"

namespace libsvg {

const std::string line::name("line"); 

line::line()
{
}

line::line(const line& orig) : shape(orig)
{
	x2 = orig.x2;
	y2 = orig.y2;
}

line::~line()
{
}

void
line::set_attrs(attr_map_t& attrs)
{
	shape::set_attrs(attrs);
	this->x = parse_double(attrs["x1"]);
	this->y = parse_double(attrs["y1"]);
	this->x2 = parse_double(attrs["x2"]);
	this->y2 = parse_double(attrs["y2"]);
	
	path_t path;
	path.push_back(Eigen::Vector3d(x, y, 0));
	path.push_back(Eigen::Vector3d(x2, y2, 0));
	offset_path(path_list, path, get_stroke_width(), get_stroke_linecap());
}

void
line::dump()
{
	std::cout << get_name()
		<< ": x1 = " << this->x
		<< ": y1 = " << this->y
		<< ": x2 = " << this->x2
		<< ": y2 = " << this->y2
		<< std::endl;
}

}