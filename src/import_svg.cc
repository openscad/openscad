#include "import.h"
#include "Polygon2d.h"
#include "printutils.h"
#include "libsvg/libsvg.h"
#include "clipper-utils.h"
#include "AST.h"

Polygon2d *import_svg(const std::string &filename, const bool center, const Location &loc)
{
	try {
		const libsvg::shapes_list_t *shapes = libsvg::libsvg_read_file(filename.c_str());

		double x_min = 1.0/0.0;
		double x_max = -1.0/0.0;
		double y_min = 1.0/0.0;
		double y_max = -1.0/0.0;
		double width = 0;
		double height = 0;
		for (const auto& shape_ptr : *shapes) {
			PRINTD("SVG shape");
			const auto page = dynamic_cast<libsvg::svgpage *>(shape_ptr.get());
			if (page) {
				width = page->get_width();
				height = page->get_height();
			}
			const auto& s = *shape_ptr;
			for (const auto& p : s.get_path_list()) {
				PRINTD("SVG path");
				for (const auto& v : p) {
					if (v.x() < x_min) {
						x_min = v.x();
					}
					if (v.x() > x_max) {
						x_max = v.x();
					}
					if (v.y() < y_min) {
						y_min = v.y();
					}
					if (v.y() > y_max) {
						y_max = v.y();
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
					double x = v.x() - cx;
					double y = -v.y() + cy;
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
	
