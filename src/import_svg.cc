/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2019 Clifford Wolf <clifford@clifford.at> and
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

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "import.h"
#include "Polygon2d.h"
#include "printutils.h"
#include "libsvg/libsvg.h"
#include "clipper-utils.h"
#include "AST.h"
#include "boost-utils.h"

namespace {

constexpr double INCH_TO_MM = 25.4;

double to_mm(const libsvg::length_t& length, const double viewbox, const bool viewbox_valid, const double dpi)
{
	switch (length.unit) {
	case libsvg::unit_t::NONE:
		return INCH_TO_MM * length.number / dpi;
	case libsvg::unit_t::PX:
		return INCH_TO_MM * length.number / 96.0;
	case libsvg::unit_t::PT:
		return INCH_TO_MM * length.number / 72.0;
	case libsvg::unit_t::PC:
		return INCH_TO_MM * length.number / 6.0;
	case libsvg::unit_t::IN:
		return INCH_TO_MM * length.number;
	case libsvg::unit_t::CM:
		return 10 * length.number;
	case libsvg::unit_t::MM:
		return length.number;
	case libsvg::unit_t::UNDEFINED:
		// If no width/height given, but viewbox is set, then rely on
		// the DPI value (e.g. Adobe Illustrator does that in older
		// versions)
		return viewbox_valid ? INCH_TO_MM * viewbox / dpi : 0.0;
	default:
		return viewbox_valid ? viewbox : 0.0;
	}
}

double calc_alignment(const libsvg::align_t alignment, double page_mm, double scale, double viewbox)
{
	switch (alignment) {
	case libsvg::align_t::MID:
		return page_mm / 2.0 - scale * viewbox / 2.0;
	case libsvg::align_t::MAX:
		return page_mm - scale * viewbox;
	default:
		return 0.0;
	}
}

}

Polygon2d *import_svg(const std::string &filename, const double dpi, const bool center, const Location &loc)
{
	try {
		const auto shapes = libsvg::libsvg_read_file(filename.c_str());

		double width_mm = 0.0;
		double height_mm = 0.0;

		Eigen::AlignedBox<double, 2> bbox{2};

		Eigen::Vector2d scale{1.0, 1.0};
		Eigen::Vector2d align{0.0, 0.0};
		Eigen::Vector2d viewbox{0.0, 0.0};

		for (const auto& shape_ptr : *shapes) {
			const auto page = dynamic_cast<libsvg::svgpage *>(shape_ptr.get());
			if (page) {
				const auto w = page->get_width();
				const auto h = page->get_height();
				const auto alignment = page->get_alignment();

				const bool viewbox_valid = page->get_viewbox().is_valid;
				width_mm = to_mm(w, page->get_viewbox().width, viewbox_valid, dpi);
				height_mm = to_mm(h, page->get_viewbox().height, viewbox_valid, dpi);

				if (viewbox_valid) {
					viewbox << page->get_viewbox().x,
							   page->get_viewbox().y;

					scale << width_mm / page->get_viewbox().width,
							 height_mm / page->get_viewbox().height;

					if (alignment.x != libsvg::align_t::NONE) {
						double scaling;
						if (alignment.meet) {
							// preserve aspect ratio and fit into viewport, so
							// select the smaller of the 2 scale factors
							scaling = scale.x() < scale.y() ? scale.x() : scale.y();
						} else {
							// preserve aspect ratio and fill viewport, so select
							// the bigger of the 2 scale factors
							scaling = scale.x() > scale.y() ? scale.x() : scale.y();
						}
						scale = Eigen::Vector2d{scaling, scaling};

						align << calc_alignment(alignment.x, width_mm, scale.x(), page->get_viewbox().width),
								 calc_alignment(alignment.y, height_mm, scale.y(), page->get_viewbox().height);
					}
				}
			}

			const auto& s = *shape_ptr;
			for (const auto& p : s.get_path_list()) {
				for (const auto& v : p) {
					bbox.extend(Eigen::Vector2d{scale.x() * v.x(), scale.y() * v.y()});
				}
			}
		}
		double cx = center ? bbox.center().x() : -align.x();
		double cy = center ? bbox.center().y() : height_mm - align.y();

		std::vector<const Polygon2d*> polygons;
		for (const auto& shape_ptr : *shapes) {
			Polygon2d *poly = new Polygon2d();
			const auto& s = *shape_ptr;
			for (const auto& p : s.get_path_list()) {
				Outline2d outline;
				for (const auto& v : p) {
					double x = scale.x() * (-viewbox.x() + v.x()) - cx;
					double y = scale.y() * (-viewbox.y() - v.y()) + cy;
					outline.vertices.push_back(Vector2d(x, y));
					outline.positive = true;
				}
				poly->addOutline(outline);
			}
			polygons.push_back(poly);
		}
		return ClipperUtils::apply(polygons, ClipperLib::ctUnion);
	} catch (const std::exception& e) {
		LOG(message_group::Error,Location::NONE,"","%1$s, import() at line %2$d",e.what(),loc.firstLine());
		return new Polygon2d();
	}
}
