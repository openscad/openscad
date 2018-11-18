#include <boost/tokenizer.hpp>

#include "polygon.h"

namespace libsvg {

const std::string polygon::name("polygon"); 

polygon::polygon()
{
}

polygon::~polygon()
{
}

void
polygon::set_attrs(attr_map_t& attrs)
{
	shape::set_attrs(attrs);
	this->points = attrs["points"];
	
	using tokenizer = boost::tokenizer<boost::char_separator<char> >;
	boost::char_separator<char> sep(" ,");
	tokenizer tokens(this->points, sep);

	double x = 0.0;
	path_t path;
	bool first = true;
	for (const auto& v : tokens) {
		double p = parse_double(v);
		
		if (first) {
			x = p;
		} else {
			path.push_back(Eigen::Vector3d(x, p, 0));
		}
		first = !first;
	}
	if (!path.empty()) {
		path.push_back(path[0]);
	}
	path_list.push_back(path);
}

}