#include "import.h"
#include "Polygon2d.h"
#include "printutils.h"
#include "libsvg/libsvg.h"
#include "clipper-utils.h"
#include "AST.h"

Polygon2d *import_svg(const std::string &filename, const double dpi, const bool center, const Location &loc)
{
	try {
		const libsvg::shapes_list_t *shapes = libsvg::libsvg_read_file(filename.c_str());

		double scale = 1.0;

		double x_min = 1.0/0.0;
		double x_max = -1.0/0.0;
		double y_min = 1.0/0.0;
		double y_max = -1.0/0.0;
		double height = 0;
		for (const auto& shape_ptr : *shapes) {
			PRINTD("SVG shape");
			const auto page = dynamic_cast<libsvg::svgpage *>(shape_ptr.get());
			double unit_scale = 1.0;
			if (page) {
				const libsvg::length_t h = page->get_height();
				switch (h.unit) {
				case libsvg::unit_t::UNDEFINED:
				case libsvg::unit_t::NONE:
					scale = 25.4 / dpi;
					height = scale * h.number;
					break;
				case libsvg::unit_t::PT:
					height = 25.4 / 72.0 * h.number;
					break;
				case libsvg::unit_t::PC:
					height = 25.4 / 6.0 * h.number;
					break;
				case libsvg::unit_t::IN:
					height = 25.4 * h.number;
					break;
				case libsvg::unit_t::CM:
					height = 10.0 * h.number;
					break;
				case libsvg::unit_t::MM:
					height = h.number;
					break;
				}
			}

			const auto& s = *shape_ptr;
			for (const auto& p : s.get_path_list()) {
				PRINTD("SVG path");
				for (const auto& v : p) {
					double x = scale * v.x();
					double y = scale * v.y();
					if (x < x_min) {
						x_min = x;
					}
					if (x > x_max) {
						x_max = x;
					}
					if (y < y_min) {
						y_min = y;
					}
					if (y > y_max) {
						y_max = y;
					}
				}
			}
		}

		double cx = center ? (x_min + x_max) / 2 : 0;
		double cy = center ? (y_min + y_max) / 2 : height;

		std::vector<const Polygon2d*> polygons;
		for (const auto& shape_ptr : *shapes) {
			Polygon2d *poly = new Polygon2d();
			const auto& s = *shape_ptr;
			for (const auto& p : s.get_path_list()) {
				Outline2d outline;
				for (const auto& v : p) {
					double x = scale * v.x() - cx;
					double y = -scale * v.y() + cy;
					outline.vertices.push_back(Vector2d(x, y));
					outline.positive=true;
				}
				poly->addOutline(outline);
			}
			polygons.push_back(poly);
		}
		return ClipperUtils::apply(polygons, ClipperLib::ctUnion);
	} catch (const std::exception& e) {
		PRINTB("ERROR: %s, import() at line %d", e.what() % loc.firstLine());
		return new Polygon2d();
	}
}
	
