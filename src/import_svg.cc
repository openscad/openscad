#include "import.h"
#include "Polygon2d.h"
#include "printutils.h"
#include "libsvg/libsvg.h"
#include "clipper-utils.h"

Polygon2d *import_svg(const std::string &filename)
{
	libsvg::shapes_list_t *shapes = libsvg::libsvg_read_file(filename.c_str());
	double x_min = 1.0/0.0;
	double x_max = -1.0/0.0;
	double y_min = 1.0/0.0;
	double y_max = -1.0/0.0;
	for (libsvg::shapes_list_t::iterator it = shapes->begin();it != shapes->end();it++) {
		PRINTD("SVG shape");
		libsvg::shape *s = (*it);
		for (libsvg::path_list_t::iterator it = s->get_path_list().begin();it != s->get_path_list().end();it++) {
			PRINTD("SVG path");
			libsvg::path_t& p = *it;
			for (libsvg::path_t::iterator it2 = p.begin();it2 != p.end();it2++) {
				Eigen::Vector3d& v = *it2;
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
	
	double cx = (x_min + x_max) / 2;
	double cy = (y_min + y_max) / 2;
	
	std::vector<const Polygon2d*> polygons;
	for (libsvg::shapes_list_t::iterator it = shapes->begin();it != shapes->end();it++) {
		Polygon2d *poly = new Polygon2d();
		libsvg::shape *s = (*it);
		for (libsvg::path_list_t::iterator it = s->get_path_list().begin();it != s->get_path_list().end();it++) {
			libsvg::path_t& p = *it;
			
			Outline2d outline;
			for (libsvg::path_t::iterator it2 = p.begin();it2 != p.end();it2++) {
				Eigen::Vector3d& v = *it2;
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
}
	
