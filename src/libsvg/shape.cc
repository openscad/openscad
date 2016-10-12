#include <stdio.h>
#include <string>
#include <vector>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/qi.hpp>

#include "shape.h"
#include "circle.h"
#include "ellipse.h"
#include "line.h"
#include "polygon.h"
#include "polyline.h"
#include "rect.h"
#include "svgpage.h"
#include "path.h"
#include "group.h"

#include "transformation.h"

namespace libsvg {

shape::shape() : parent(NULL), x(0), y(0)
{
}

shape::shape(const shape& orig)
{
}

shape::~shape()
{
}

shape *
shape::create_from_name(const char *name)
{
	if (circle::name == name) {
		return new circle();
	} else if (ellipse::name == name) {
		return new ellipse();
	} else if (line::name == name) {
		return new line();
	} else if (polygon::name == name) {
		return new polygon();
	} else if (polyline::name == name) {
		return new polyline();
	} else if (rect::name == name) {
		return new rect();
	} else if (svgpage::name == name) {
		return new svgpage();
	} else if (path::name == name) {
		return new path();
	} else if (group::name == name) {
		return new group();
	} else {
		return NULL;
	}
}

void
shape::set_attrs(attr_map_t& attrs)
{
	this->id = attrs["id"];
	this->transform = attrs["transform"];
	this->stroke_width = attrs["stroke-width"];
	this->stroke_linecap = attrs["stroke-linecap"];
	this->style = attrs["style"];
}

std::string
shape::get_style(std::string name)
{
	std::vector<std::string> styles;
	boost::split(styles, this->style, boost::is_any_of(";"));
	
	for (std::vector<std::string>::iterator it = styles.begin();it != styles.end();it++) {
		std::vector<std::string> values;
		boost::split(values, *it, boost::is_any_of(":"));
		if (values.size() != 2) {
			continue;
		}
		if (name == values[0]) {
			return values[1];
		}
	}
	return std::string();
}

double
shape::get_stroke_width()
{
	double stroke_width;
	if (this->stroke_width.empty()) {
		stroke_width = parse_double(get_style("stroke-width"));
	} else {
		stroke_width = parse_double(this->stroke_width);
	}
	return stroke_width < 0.01 ? 1 : stroke_width;
}

ClipperLib::EndType
shape::get_stroke_linecap()
{
	std::string cap;
	if (this->stroke_linecap.empty()) {
		cap = get_style("stroke-linecap");
	} else {
		cap = this->stroke_linecap;
	}
	
	if (cap == "butt") {
		return ClipperLib::etOpenButt;
	} else if (cap == "round") {
		return ClipperLib::etOpenRound;
	} else if (cap == "square") {
		return ClipperLib::etOpenSquare;
	}
	return ClipperLib::etOpenSquare;
}

void
shape::collect_transform_matrices(std::vector<Eigen::Matrix3d>& matrices, shape *s)
{
	std::string transform_arg(s->transform);

	boost::replace_all(transform_arg, "matrix", "m");
	boost::replace_all(transform_arg, "translate", "t");
	boost::replace_all(transform_arg, "scale", "s");
	boost::replace_all(transform_arg, "rotate", "r");
	boost::replace_all(transform_arg, "skewX", "x");
	boost::replace_all(transform_arg, "skewY", "y");

	std::string commands = "mtsrxy";
	
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ,()", commands.c_str());
	tokenizer tokens(transform_arg, sep);

	transformation *t = NULL;
	std::vector<transformation *> transformations;
	for (tokenizer::iterator it = tokens.begin();it != tokens.end();++it) {
		std::string v = (*it);
		if ((v.length() == 1) && (commands.find(v) != std::string::npos)) {
			if (t != NULL) {
				transformations.push_back(t);
				t = NULL;
			}
			switch (v[0]) {
			case 'm':
				t = new matrix();
				break;
			case 't':
				t = new translate();
				break;
			case 's':
				t = new scale();
				break;
			case 'r':
				t = new rotate();
				break;
			case 'x':
				t = new skew_x();
				break;
			case 'y':
				t = new skew_y();
				break;
			default:
				std::cout << "unknown transform op " << v << std::endl;
				t = NULL;
			}
		} else {
			if (t) {
				t->add_arg(v);
			}
		}
	}
	if (t != NULL) {
		transformations.push_back(t);
	}
	
	for (std::vector<transformation *>::reverse_iterator it = transformations.rbegin();it != transformations.rend();it++) {
		transformation *t = *it;
		std::vector<Eigen::Matrix3d> m = t->get_matrices();
		matrices.insert(matrices.begin(), m.rbegin(), m.rend());
		delete t;
	}
}

void
shape::apply_transform()
{
	std::vector<Eigen::Matrix3d> matrices;
	for (shape *s = this;s->get_parent() != NULL;s = s->get_parent()) {
		collect_transform_matrices(matrices, s);
	}

	path_list_t result_list;
	for (path_list_t::iterator it = path_list.begin();it != path_list.end();it++) {
		path_t& p = *it;
		
		result_list.push_back(path_t());
		for (path_t::iterator it2 = p.begin();it2 != p.end();it2++) {
			Eigen::Vector3d result((*it2).x(), (*it2).y(), 1);
			for (std::vector<Eigen::Matrix3d>::reverse_iterator it3 = matrices.rbegin();it3 != matrices.rend();it3++) {
				result = *it3 * result;
			}
			
			result_list.back().push_back(result);
		}
	}
	path_list = result_list;
}

void
shape::offset_path(path_list_t& path_list, path_t& path, double stroke_width, ClipperLib::EndType stroke_linecap) {
	ClipperLib::Path line;
	ClipperLib::Paths result;
	for (path_t::iterator it = path.begin();it != path.end();it++) {
		Eigen::Vector3d& v = *it;
		line << ClipperLib::IntPoint(v.x() * 10000, v.y() * 10000);
	}

	ClipperLib::ClipperOffset co;
	co.AddPath(line, ClipperLib::jtMiter, stroke_linecap);
	co.Execute(result, stroke_width * 5000.0);

	for (ClipperLib::Paths::iterator it = result.begin();it != result.end();it++) {
		ClipperLib::Path& p = *it;
		path_list.push_back(path_t());
		for (ClipperLib::Path::iterator it2 = p.begin();it2 != p.end();it2++) {
			ClipperLib::IntPoint& point = *it2;
			path_list.back().push_back(Eigen::Vector3d(point.X / 10000.0, point.Y / 10000.0, 0));
		}
		path_list.back().push_back(Eigen::Vector3d(p[0].X / 10000.0, p[0].Y / 10000.0, 0));
	}
}

void
shape::draw_ellipse(path_t& path, double x, double y, double rx, double ry) {
	unsigned long fn = 40;
	for (unsigned long idx = 1;idx <= fn;idx++) {
		const double a = idx * (2 * M_PI / (double)fn);
		const double xx = rx * sin(a) + x;
		const double yy = ry * cos(a) + y;
		path.push_back(Eigen::Vector3d(xx, yy, 0));
	}
}

std::ostream & operator<<(std::ostream &os, const shape& s)
{
    return os << s.get_name() << " | id = '" << s.id << "', transform = '" << s.transform << "'";
}

}