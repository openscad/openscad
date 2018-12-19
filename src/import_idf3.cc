/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2018 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define BOOST_SPIRIT_QI_DEBUG

#include <string>
#include <fstream>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_stream.hpp>

#include "calc.h"
#include "Polygon2d.h"
#include "importnode.h"
#include "printutils.h"

namespace qi = boost::spirit::qi;

struct header_struct
{
	std::string file_type;
	std::string version;
	std::string system;
	std::string date;
	std::string revision;
	std::string board_name;
	std::string units;
};
BOOST_FUSION_ADAPT_STRUCT(header_struct, (std::string, file_type) (std::string, version) (std::string, system) (std::string, date) (std::string, revision) (std::string, board_name) (std::string, units))

struct loop_struct
{
	int label;
	double x;
	double y;
	double angle;
};
BOOST_FUSION_ADAPT_STRUCT(loop_struct, (int, label) (double, x) (double, y) (double, angle))

struct outline_struct
{
	std::string type;
	std::string owner;
	double thickness;
	std::vector<loop_struct> loop_list;
};
BOOST_FUSION_ADAPT_STRUCT(outline_struct, (std::string, type) (std::string, owner) (double, thickness) (std::vector<loop_struct>, loop_list))

struct hole_struct
{
	double diameter;
	double x;
	double y;
	std::string plating_style;
	std::string associated_part;
	std::string hole_type;
	std::string hole_owner;
};
BOOST_FUSION_ADAPT_STRUCT(hole_struct, (double, diameter) (double, x) (double, y) (std::string, plating_style) (std::string, associated_part) (std::string, hole_type) (std::string, hole_owner))

struct idf_struct
{
	header_struct header;
	outline_struct outline;
	std::vector<hole_struct> drilled_holes;
};
BOOST_FUSION_ADAPT_STRUCT(idf_struct, (header_struct, header) (outline_struct, outline) (std::vector<hole_struct>, drilled_holes))

static idf_struct read_file(const std::string &filename) {
	using I = boost::spirit::istream_iterator;
	using rule = qi::rule<I>;
	using srule = qi::rule<I, std::vector<char>>;

	idf_struct result;
	qi::rule<I, idf_struct> idf;
	qi::rule<I, header_struct> header;
	qi::rule<I, outline_struct> outline;
	qi::rule<I, std::vector<hole_struct>> drilled_holes;
	qi::rule<I, loop_struct> loop;
	qi::rule<I, std::vector<loop_struct>> loop_list;
	qi::rule<I, hole_struct> holes;
	qi::rule<I, std::vector<hole_struct>> holes_list;

	srule file_type, outline_type, string, quoted_string, date, unit, owner, version, revision, plating;
	rule sp, nl, outline_end;

	idf = header >> +nl >> outline >> +nl >> -drilled_holes;
	header = ".HEADER" >> nl >> file_type >> sp >> version >> sp >> string >> sp >> date >> sp >> revision >> nl >> string >> sp >> unit >> nl >> ".END_HEADER";

	outline = outline_type  >> sp >> owner >> nl >> qi::double_ >> nl >> loop_list >> nl >> outline_end;
	loop_list = loop % nl;
	loop = -sp >> qi::int_ >> sp >>qi::double_ >> sp >> qi::double_ >> sp >> qi::double_;

	drilled_holes = ".DRILLED_HOLES" >> nl >> holes_list >> nl >> ".END_DRILLED_HOLES";
	holes_list = holes % nl;
	holes = -sp >> qi::double_ >> sp >> qi::double_ >> sp >> qi::double_ >> sp >> plating >> sp >> string >> sp >> string >> sp >> owner;

	file_type = qi::string("BOARD_FILE") | qi::string("PANEL_FILE");
	outline_type = qi::string(".BOARD_OUTLINE") | qi::string(".PANEL_OUTLINE");
	outline_end = qi::lit(".END_BOARD_OUTLINE") | qi::lit(".END_PANEL_OUTLINE");
	unit = qi::string("MM") | qi::string("THOU");
	owner = qi::string("MCAD") | qi::string("ECAD") | qi::string("UNOWNED");
	plating = qi::string("PTH") | qi::string("NPTH");
	
	string = quoted_string | +(qi::char_ - qi::space);
	quoted_string = '"' >> *(qi::char_ - '"') >> '"';
	date = +qi::char_("0123456789/.:");
	version = +qi::char_("0-9") >> qi::char_('.') >> +qi::char_("0-9");
	revision = +qi::char_("0-9");
	nl = qi::skip[+qi::char_("\r\n")];
	sp = qi::skip[+qi::space];

	std::ifstream file(filename, std::ifstream::binary);
	if (file.is_open()) {
		file.unsetf(std::ios::skipws);
		boost::spirit::istream_iterator it{file};
		boost::spirit::istream_iterator end;
		qi::phrase_parse(it, end, idf, qi::space, result);
		file.close();
		if (it != end) {
			PRINTB("ERROR: Can't read IDF file '%s'", filename);
                        std::copy(it, end, std::ostream_iterator<char>(std::cout, ""));
		}
	}
	return result;
}

static void add_circle(Outline2d &outline, const double x, const double y, const double radius, const double fn, const double fa, const double fs)
{
	const int n = Calc::get_fragments_from_r(radius, fn, fs, fa) + 2;
	for (int i = 0; i < n; i++) {
		double a = (2 * M_PI * i) / n;
		outline.vertices.push_back(Vector2d{-std::cos(a) * radius + x, std::sin(a) * radius + y});
	}
}

static void add_loop(Outline2d &outline, const loop_struct &loop, const double fn, const double fa, const double fs, const double scale)
{
	const double x = scale * loop.x;
	const double y = scale * loop.y;
	if (loop.angle == 0.0) {
		outline.vertices.push_back(Vector2d(x, y));
	} else if (loop.angle >= 360.0) {
		const Vector2d &v = outline.vertices.back();
		outline.vertices.pop_back();
		const double radius = std::sqrt(std::pow(x - v.x(), 2.0) + std::pow(y - v.y(), 2.0));
		add_circle(outline, v.x(), v.y(), radius, fn, fa, fs);
	} else {
		const double half_angle = M_PI * abs(loop.angle) / 360.0;
		const Vector2d &v = outline.vertices.back();
		const Vector2d pm{(x + v.x()) / 2.0, (y + v.y()) / 2.0};
		const Vector2d vpm = pm - v;
		const double radius = vpm.norm() / sin(half_angle);
		const Vector2d vpm_rot = radius * cos(half_angle) * Vector2d{-vpm.y(), vpm.x()}.normalized();
		const Vector2d center = vpm_rot + pm;
		const double start_angle = atan2(v.x() - center.x(), v.y() - center.y()) + M_PI_2;

		const double steps = Calc::get_fragments_from_r(radius, fn, fs, fa);
		const int n = std::max(3.0, (steps * half_angle / M_PI) + 1.0) + 2;
		for (int i = 0; i < n; i++) {
			// The direction changes with angle. If angle > 0 then going counter-clockwise
			double a = start_angle - (loop.angle * i / 180.0 * M_PI) / n;
			outline.vertices.push_back(Vector2d{-std::cos(a) * radius + center.x(), std::sin(a) * radius + center.y()});
		}
	}	
}

Geometry * import_idf3(const std::string &filename, const double fn, const double fa, const double fs)
{
	using outline_map_t = std::map<int, Outline2d>;

	const auto idf = read_file(filename);

	double scale = idf.header.units == "MM" ? 1.0 : 0.0254;
	
	auto polygon = new Polygon2d();
	polygon->setSanitized(true);

	Outline2d outline;
	outline_map_t cutouts;
	for (const auto &l : idf.outline.loop_list) {
		if (l.label == 0) {
			add_loop(outline, l, fn, fa, fs, scale);
		} else {
			outline_map_t::iterator it = cutouts.find(l.label);
			if (it == cutouts.end()) {
				it = cutouts.emplace(l.label, Outline2d{false}).first;
			}
			add_loop(it->second, l, fn, fa, fs, scale);
		}
	}
	if (!outline.vertices.empty()) {
		polygon->addOutline(outline);
		for (const auto &h : idf.drilled_holes) {
			Outline2d hole{false};
			add_circle(hole, scale * h.x, scale * h.y, scale * h.diameter / 2.0, fn, fa, fs);
			polygon->addOutline(hole);
		}
		for (const auto &cutout : cutouts) {
			polygon->addOutline(cutout.second);
		}
	}

	return polygon;
}
