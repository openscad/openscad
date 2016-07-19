#include <stdlib.h>

#include <string>
#include <iostream>
#include <cmath>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include "path.h"

namespace libsvg {

const std::string path::name("path"); 

/*
PATHSEG_CLOSEPATH z
PATHSEG_MOVETO_ABS M
PATHSEG_MOVETO_REL m
PATHSEG_LINETO_ABS L
PATHSEG_LINETO_REL l
PATHSEG_CURVETO_CUBIC_ABS C
PATHSEG_CURVETO_CUBIC_REL c
PATHSEG_CURVETO_QUADRATIC_ABS Q
PATHSEG_CURVETO_QUADRATIC_REL q
PATHSEG_ARC_ABS A
PATHSEG_ARC_REL a
PATHSEG_LINETO_HORIZONTAL_ABS H
PATHSEG_LINETO_HORIZONTAL_REL h
PATHSEG_LINETO_VERTICAL_ABS V
PATHSEG_LINETO_VERTICAL_REL v
PATHSEG_CURVETO_CUBIC_SMOOTH_ABS S
PATHSEG_CURVETO_CUBIC_SMOOTH_REL s
PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS T
PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL t
*/

path::path()
{
}

path::path(const path& orig) : shape(orig)
{
	data = orig.data;
}

path::~path()
{
}

static double
vector_angle(double ux, double uy, double vx, double vy)
{
	double angle = atan2(vy, vx) - atan2(uy, ux);
	if (angle < 0) {
		angle += 2 * M_PI;
	}
	return angle;
}

void
path::arc_to(path_t& path, double x1, double y1, double rx, double ry, double x2, double y2, double angle, bool large, bool sweep)
{
	// http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
	
	// (F.6.5.1))
        double cos_rad = cos(M_PI * angle / 180.0);
        double sin_rad = sin(M_PI * angle / 180.0);
        double dx = (x1 - x2) / 2;
        double dy = (y1 - y2) / 2;
        double x1_ = cos_rad * dx + sin_rad * dy;
        double y1_ = -sin_rad * dx + cos_rad * dy;

	double d = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);
	if (d > 1) {
		rx = std::fabs(std::sqrt(d) * rx);
		ry = std::fabs(std::sqrt(d) * ry);
	}
	
	// F.6.5.2
	double t1 = rx * rx * ry * ry - rx * rx * y1_ * y1_ - ry * ry * x1_ * x1_;
	double t2 = rx * rx * y1_ * y1_ + ry * ry * x1_ * x1_;
	if (t1 < 0) {
		t1 = 0;
	}
	double t3 = std::sqrt(t1 / t2);
	if (large == sweep) {
		t3 = -t3;
	}
	
	double cx_ = t3 * rx * y1_ / ry;
	double cy_ = t3 * -ry * x1_ / rx;

	// F.6.5.3
	double cx = cos_rad * cx_ - sin_rad * cy_ + (x1 + x2) / 2.0;
	double cy = sin_rad * cx_ + cos_rad * cy_ + (y1 + y2) / 2.0;

	// F.6.5.4
        double ux = (x1_ - cx_) / rx;
        double uy = (y1_ - cy_) / ry;
	double vx = (-x1_ - cx_) / rx;
	double vy = (-y1_ - cy_) / ry;
	
	double theta = vector_angle(1, 0, ux, uy);
	double delta = vector_angle(ux, uy, vx, vy);
	if (!sweep) {
            delta -= 2 * M_PI;
	}
	
	int steps = std::fabs(delta) * 10.0 / M_PI + 4;
	for (int a = 0;a <= steps;a++) {
	        double phi = theta + delta * a / steps;

		double xx = cos_rad * cos(phi) * rx - sin_rad * sin(phi) * ry;
		double yy = sin_rad * cos(phi) * rx + cos_rad * sin(phi) * ry;
		
		path.push_back(Eigen::Vector3d(xx + cx, yy + cy, 0));
	}
}

void
path::curve_to(path_t& path, double x, double y, double cx1, double cy1, double x2, double y2)
{
	unsigned long fn = 20;
	for (unsigned long idx = 1;idx <= fn;idx++) {
		const double a = idx * (1.0 / (double)fn);
		const double xx = x * t(a, 2) + cx1 * 2 * t(a, 1) * a + x2 * a * a;
		const double yy = y * t(a, 2) + cy1 * 2 * t(a, 1) * a + y2 * a * a;
		path.push_back(Eigen::Vector3d(xx, yy, 0));
	}
}

void
path::curve_to(path_t& path, double x, double y, double cx1, double cy1, double cx2, double cy2, double x2, double y2)
{
	unsigned long fn = 20;
	for (unsigned long idx = 1;idx <= fn;idx++) {
		const double a = idx * (1.0 / (double)fn);
		const double xx = x * t(a, 3) + cx1 * 3 * t(a, 2) * a + cx2 * 3 * t(a, 1) * a * a + x2 * a * a * a;
		const double yy = y * t(a, 3) + cy1 * 3 * t(a, 2) * a + cy2 * 3 * t(a, 1) * a * a + y2 * a * a * a;
		path.push_back(Eigen::Vector3d(xx, yy, 0));
	}
}

void
path::set_attrs(attr_map_t& attrs)
{
	std::string commands = "-zmlcqahvstZMLCQAHVST";
	
	shape::set_attrs(attrs);
	this->data = attrs["d"];

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ,", commands.c_str());
	tokenizer tokens(this->data, sep);

	double x = 0;
	double y = 0;
	double xx = 0;
	double yy = 0;
	double rx = 0;
	double ry = 0;
	double cx1 = 0;
	double cy1 = 0;
	double cx2 = 0;
	double cy2 = 0;
	double angle = 0;
	bool large = false;
	bool sweep = false;
	bool last_cmd_cubic_bezier = false;
	bool last_cmd_quadratic_bezier = false;
	char cmd = ' ';
	int point = 0;
	
	bool negate = false;
	bool path_closed = false;
	std::string exp;
	path_list.push_back(path_t());
	for (tokenizer::iterator it = tokens.begin();it != tokens.end();++it) {
		std::string v = (*it);

		double p = 0;
		if ((v.length() == 1) && (commands.find(v) != std::string::npos)) {
			if (v[0] == '-') {
				negate = true;
				continue;
			}
			point = -1;
			cmd = v[0];
		} else {
			if (*v.rbegin() == 'e') {
				exp = negate ? std::string("-") + v : v;
				negate = false;
				continue;
			}
			if (exp.empty()) {
				p = parse_double(v);
				p = negate ? -p : p;
			} else {
				p = parse_double(exp + (negate ? "-" : "") + v);
				exp = "";
			}
			negate = false;
		}
		
		switch (cmd) {
		case 'a':
		case 'A':
			//(rx ry x-axis-rotation large-arc-flag sweep-flag x y)
			switch (point) {
			case 0:
				rx = std::fabs(p);
				break;
			case 1:
				ry = std::fabs(p);
				break;
			case 2:
				angle = p;
				break;
			case 3:
				large = p > 0.5;
				break;
			case 4:
				sweep = p > 0.5;
				break;
			case 5:
				xx = cmd == 'a' ? x + p : p;
				break;
			case 6:
				yy = cmd == 'a' ? y + p : p;
				arc_to(path_list.back(), x, y, rx, ry, xx, yy, angle, large, sweep);
				x = xx;
				y = yy;
				point = -1;
				last_cmd_cubic_bezier = false;
				last_cmd_quadratic_bezier = false;
				break;
			}
			break;
		case 'l':
		case 'L':
			switch (point) {
			case 0:
				xx = cmd == 'l' ? x + p : p;
				break;
			case 1:
				yy = cmd == 'l' ? y + p : p;
				path_list.back().push_back(Eigen::Vector3d(xx, yy, 0));
				x = xx;
				y = yy;
				point = -1;
				last_cmd_cubic_bezier = false;
				last_cmd_quadratic_bezier = false;
				break;
			}
			break;
		case 'c':
		case 'C':
			switch (point) {
			case 0:
				cx1 = p;
				break;
			case 1:
				cy1 = p;
				break;
			case 2:
				cx2 = p;
				break;
			case 3:
				cy2 = p;
				break;
			case 4:
				xx = cmd == 'c' ? x + p : p;
				break;
			case 5:
				yy = cmd == 'c' ? y + p : p;
				cx1 = cmd == 'c' ? x + cx1 : cx1;
				cy1 = cmd == 'c' ? y + cy1 : cy1;
				cx2 = cmd == 'c' ? x + cx2 : cx2;
				cy2 = cmd == 'c' ? y + cy2 : cy2;
				curve_to(path_list.back(), x, y, cx1, cy1, cx2, cy2, xx, yy);
				x = xx;
				y = yy;
				point = -1;
				last_cmd_cubic_bezier = true;
				last_cmd_quadratic_bezier = false;
				break;
			}
			break;
		case 's':
		case 'S':
			switch (point) {
			case 0:
				if (last_cmd_cubic_bezier) {
					Eigen::Vector2d old_control_point(cx2, cy2);
					Eigen::Vector2d current_point(x, y);
					Eigen::Vector2d new_control_point = current_point + (current_point - old_control_point);
					cx1 = new_control_point.x();
					cy1 = new_control_point.y();
				} else {
					cx1 = x;
					cy1 = y;
				}
				cx2 = p;
				break;
			case 1:
				cy2 = p;
				break;
			case 2:
				xx = cmd == 's' ? x + p : p;
				break;
			case 3:
				yy = cmd == 's' ? y + p : p;
				cx2 = cmd == 's' ? x + cx2 : cx2;
				cy2 = cmd == 's' ? y + cy2 : cy2;
				curve_to(path_list.back(), x, y, cx1, cy1, cx2, cy2, xx, yy);
				x = xx;
				y = yy;
				point = -1;
				last_cmd_cubic_bezier = true;
				last_cmd_quadratic_bezier = false;
				break;
			}
			break;
		case 'q':
		case 'Q':
			switch (point) {
			case 0:
				cx1 = p;
				break;
			case 1:
				cy1 = p;
				break;
			case 2:
				xx = cmd == 'q' ? x + p : p;
				break;
			case 3:
				yy = cmd == 'q' ? y + p : p;
				cx1 = cmd == 'q' ? x + cx1 : cx1;
				cy1 = cmd == 'q' ? y + cy1 : cy1;
				curve_to(path_list.back(), x, y, cx1, cy1, xx, yy);
				x = xx;
				y = yy;
				point = -1;
				last_cmd_cubic_bezier = false;
				last_cmd_quadratic_bezier = true;
				break;
			}
			break;
		case 't':
		case 'T':
			switch (point) {
			case 0:
				if (last_cmd_quadratic_bezier) {
					Eigen::Vector2d old_control_point(cx1, cy1);
					Eigen::Vector2d current_point(x, y);
					Eigen::Vector2d new_control_point = current_point + (current_point - old_control_point);
					cx1 = new_control_point.x();
					cy1 = new_control_point.y();
				} else {
					cx1 = x;
					cy1 = y;
				}
				xx = cmd == 't' ? x + p : p;
				break;
			case 1:
				yy = cmd == 't' ? y + p : p;
				curve_to(path_list.back(), x, y, cx1, cy1, xx, yy);
				x = xx;
				y = yy;
				point = -1;
				last_cmd_cubic_bezier = false;
				last_cmd_quadratic_bezier = true;
				break;
			}
			break;
		case 'm':
		case 'M':
			switch (point) {
			case 0:
				xx = cmd == 'm' ? x + p : p;
				break;
			case 1:
				yy = cmd == 'm' ? y + p : p;
				cmd = cmd == 'm' ? 'l' : 'L';
				
				path_t path = path_list.back();
				if (!path_list.back().empty()) {
					if (is_open_path(path)) {
						path_list.pop_back();
						offset_path(path_list, path, get_stroke_width(), get_stroke_linecap());
					}
					path_list.push_back(path_t());
				}
				
				path_list.back().push_back(Eigen::Vector3d(xx, yy, 0));
				x = xx;
				y = yy;
				point = -1;
				last_cmd_cubic_bezier = false;
				last_cmd_quadratic_bezier = false;
			}
			break;
		case 'v':
		case 'V':
			switch (point) {
			case 0:
				y = cmd == 'v' ? y + p : p;
				path_list.back().push_back(Eigen::Vector3d(x, y, 0));
				point = -1;
				last_cmd_cubic_bezier = false;
				last_cmd_quadratic_bezier = false;
				break;
			}
			break;
		case 'h':
		case 'H':
			switch (point) {
			case 0:
				x = cmd == 'h' ? x + p : p;
				path_list.back().push_back(Eigen::Vector3d(x, y, 0));
				point = -1;
				last_cmd_cubic_bezier = false;
				last_cmd_quadratic_bezier = false;
				break;
			}
			break;
		case 'z':
		case 'Z':
			if (!path_list.back().empty()) {
				Eigen::Vector3d p = path_list.back()[0];
				path_list.back().push_back(p);
				x = p.x();
				y = p.y();
			}
			path_list.push_back(path_t());
			path_closed = true;
			last_cmd_cubic_bezier = false;
			last_cmd_quadratic_bezier = false;
			break;
		}
		
		point++;
	}
	
	while (!path_list.empty() && path_list.back().empty()) {
		path_list.pop_back();
	}
	
	if (!path_closed && !path_list.empty()) {
		path_t path = path_list.back();
		if (is_open_path(path)) {
			path_list.pop_back();
			offset_path(path_list, path, get_stroke_width(), get_stroke_linecap());
		}
	}
}

bool
path::is_open_path(path_t& path)
{
	const Eigen::Vector3d p1 = path[0];
	const Eigen::Vector3d p2 = path.back();
	double distance = pow(pow(p1.x() - p2.x(), 2) + pow(p1.y() - p2.y(), 2) + pow(p1.z() - p2.z(), 2), 0.5);
	return distance > 0.1;
}

void
path::dump()
{
	std::cout << get_name()
		<< ": x = " << this->x
		<< ", y = " << this->y;
	for (path_list_t::iterator it = path_list.begin();it != path_list.end();it++) {
		path_t& p = *it;
		std::cout << "[";
		for (path_t::iterator it2 = p.begin();it2 != p.end();it2++) {
			Eigen::Vector3d& v = *it2;
			std::cout << " (" << v.x() << ", " << v.y() << ")";
		}
		std::cout << "]";
	}
	std::cout << std::endl;
}

}
