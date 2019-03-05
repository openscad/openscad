#include "import.h"
#include "Polygon2d.h"
#include "printutils.h"
#include "libsvg/libsvg.h"
#include "clipper-utils.h"
#include "AST.h"

namespace {

constexpr double INCH_TO_MM = 25.4;

struct dim_t {
	double x;
	double y;
};

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

}

Polygon2d *import_svg(const std::string &filename, const double dpi, const bool center, const Location &loc)
{
	try {
		const libsvg::shapes_list_t *shapes = libsvg::libsvg_read_file(filename.c_str());

		double width_mm = 0.0;
		double height_mm = 0.0;

		dim_t min{1.0/0.0, 1.0/0.0};
		dim_t max{-1.0/0.0, -1.0/0.0};
		dim_t scale{0.0, 0.0};
		dim_t align{0.0, 0.0};
		dim_t viewbox{0.0, 0.0};

		for (const auto& shape_ptr : *shapes) {
			const auto page = dynamic_cast<libsvg::svgpage *>(shape_ptr.get());
			if (page) {
				const libsvg::length_t w = page->get_width();
				const libsvg::length_t h = page->get_height();
				const libsvg::alignment_t alignment = page->get_alignment();

				const bool viewbox_valid = page->get_viewbox().is_valid;
				width_mm = to_mm(w, page->get_viewbox().width, viewbox_valid, dpi);
				height_mm = to_mm(h, page->get_viewbox().height, viewbox_valid, dpi);

				scale.x = viewbox_valid ? width_mm / page->get_viewbox().width : 1.0;
				scale.y = viewbox_valid ? height_mm / page->get_viewbox().height : 1.0;

				if (viewbox_valid && (alignment.x != libsvg::align_t::NONE)) {
					if (alignment.meet) {
						// preserve aspect ratio and fit into viewport, so
						// select the smaller of the 2 scale factors
						scale.x = scale.x < scale.y ? scale.x : scale.y;
					} else {
						// preserve aspect ratio and fill viewport, so select
						// the bigger of the 2 scale factors
						scale.x = scale.x > scale.y ? scale.x : scale.y;
					}
					scale.y = scale.x;

					switch (alignment.x) {
					case libsvg::align_t::MID:
						align.x = width_mm / 2.0 - scale.x * page->get_viewbox().width / 2.0;
						break;
					case libsvg::align_t::MAX:
						align.x = width_mm - scale.x * page->get_viewbox().width;
						break;
					default:
						break;
					}

					switch (alignment.y) {
					case libsvg::align_t::MID:
						align.y = height_mm / 2.0 - scale.y * page->get_viewbox().height / 2.0;
						break;
					case libsvg::align_t::MAX:
						align.y = height_mm - scale.y * page->get_viewbox().height;
						break;
					default:
						break;
					}
				}

				viewbox.x = viewbox_valid ? page->get_viewbox().x : 0.0;
				viewbox.y = viewbox_valid ? page->get_viewbox().y : 0.0;
			}

			const auto& s = *shape_ptr;
			for (const auto& p : s.get_path_list()) {
				for (const auto& v : p) {
					double x = scale.x * v.x();
					double y = scale.y * v.y();
					if (x < min.x) {
						min.x = x;
					}
					if (x > max.x) {
						max.x = x;
					}
					if (y < min.y) {
						min.y = y;
					}
					if (y > max.y) {
						max.y = y;
					}
				}
			}
		}

		double cx = center ? (min.x + max.x) / 2 : -align.x;
		double cy = center ? (min.y + max.y) / 2 : height_mm - align.y;

		std::vector<const Polygon2d*> polygons;
		for (const auto& shape_ptr : *shapes) {
			Polygon2d *poly = new Polygon2d();
			const auto& s = *shape_ptr;
			for (const auto& p : s.get_path_list()) {
				Outline2d outline;
				for (const auto& v : p) {
					double x = scale.x * (-viewbox.x + v.x()) - cx;
					double y = scale.y * (-viewbox.y - v.y()) + cy;
					outline.vertices.push_back(Vector2d(x, y));
					outline.positive = true;
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
