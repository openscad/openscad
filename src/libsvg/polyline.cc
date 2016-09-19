#include <boost/tokenizer.hpp>

#include "polyline.h"

namespace libsvg {

const std::string polyline::name("polyline"); 

polyline::polyline()
{
}

polyline::polyline(const polyline& orig) : shape(orig)
{
	points = orig.points;
}

polyline::~polyline()
{
}

void
polyline::set_attrs(attr_map_t& attrs)
{
	shape::set_attrs(attrs);
	this->points = attrs["points"];
	
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ,");
	tokenizer tokens(this->points, sep);

	double x = 0.0;
	path_t path;
	bool first = true;
	for (tokenizer::iterator it = tokens.begin();it != tokens.end();++it) {
		std::string v = (*it);
		double p = parse_double(v);
		
		if (first) {
			x = p;
		} else {
			path.push_back(Eigen::Vector3d(x, p, 0));
		}
		first = !first;
	}
	
	offset_path(path_list, path, get_stroke_width(), get_stroke_linecap());
}

}