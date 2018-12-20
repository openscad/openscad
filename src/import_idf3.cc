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

#include <map>

#include "calc.h"
#include "Polygon2d.h"
#include "importnode.h"
#include "printutils.h"

#include "libidf/libidf.h"

class section_visitor : public boost::static_visitor<std::vector<Outline2d>> {
public:
	section_visitor(double scale, double fn, double fa, double fs) : scale(scale), fn(fn), fa(fa), fs(fs) {};

	std::vector<Outline2d> operator()(const libidf::outline_struct &section) const
	{
		using outline_map_t = std::map<int, Outline2d>;

		std::vector<Outline2d> result;
		result.emplace_back(Outline2d{});
		outline_map_t cutouts;
		for (const auto &l : section.loop_list) {
			if (l.label == 0) {
				add_loop(result[0], l, fn, fa, fs, scale);
			} else {
				outline_map_t::iterator it = cutouts.find(l.label);
				if (it == cutouts.end()) {
					it = cutouts.emplace(l.label, Outline2d{false}).first;
				}
				add_loop(it->second, l, fn, fa, fs, scale);
			}
		}
		if (!result[0].vertices.empty()) {
			for (const auto &cutout : cutouts) {
				result.push_back(cutout.second);
			}
		}
		return result;
	}

	std::vector<Outline2d> operator()(const libidf::other_outline_struct &) const { return {}; }

	std::vector<Outline2d> operator()(const libidf::drilled_holes_struct &section) const
	{
		std::vector<Outline2d> result;
		for (const auto &h : section) {
			result.emplace_back(Outline2d{false});
			add_circle(result.back(), scale * h.x, scale * h.y, scale * h.diameter / 2.0, fn, fa, fs);
		}
		return result;
	}

	std::vector<Outline2d> operator()(const libidf::route_outline_struct &) const { return {}; }

	std::vector<Outline2d> operator()(const libidf::place_outline_struct &) const { return {}; }

	std::vector<Outline2d> operator()(const libidf::route_keepout_struct &) const { return {}; }

	std::vector<Outline2d> operator()(const libidf::place_keepout_struct &) const { return {}; }

	std::vector<Outline2d> operator()(const libidf::via_keepout_struct &) const { return {}; }

	std::vector<Outline2d> operator()(const libidf::place_region_struct &) const { return {}; }

	std::vector<Outline2d> operator()(const libidf::notes_struct &) const { return {}; }

	std::vector<Outline2d> operator()(const libidf::placement_struct &) const { return {}; }

private:
	void add_circle(Outline2d &outline, const double x, const double y, const double radius, const double fn, const double fa, const double fs) const
	{
		const int n = Calc::get_fragments_from_r(radius, fn, fs, fa) + 2;
		for (int i = 0; i < n; i++) {
			double a = (2 * M_PI * i) / n;
			outline.vertices.push_back(Vector2d{-std::cos(a) * radius + x, std::sin(a) * radius + y});
		}
	}

	void add_loop(Outline2d &outline, const libidf::loop_struct &loop, const double fn, const double fa, const double fs, const double scale) const
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

	const double scale, fn, fa, fs;
};

Geometry * import_idf3(const std::string &filename, const double fn, const double fa, const double fs)
{
	const auto idf = libidf::read_file(filename);

	auto polygon = new Polygon2d();
	polygon->setSanitized(true);

	double scale = idf.header.units == "MM" ? 1.0 : 0.0254;
	for (const auto &section : idf.sections) {
		auto result = boost::apply_visitor(section_visitor{scale, fn, fa, fs}, section);		
		for (auto &outline : result) {
			polygon->addOutline(outline);
		}
	}

	return polygon;
}
