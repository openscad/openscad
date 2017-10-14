/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
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

#include "export.h"
#include "polyset.h"
#include "polyset-utils.h"

static void append_svg(const Polygon2d &poly, std::ostream &output)
{
	output << "<path d=\"\n";
	for(const auto &o : poly.outlines()) {
		if (o.vertices.empty()) {
			continue;
		}
		
		const Eigen::Vector2d& p0 = o.vertices[0];
		output << "M " << p0.x() << "," << -p0.y();
		for (unsigned int idx = 1;idx < o.vertices.size();idx++) {
			const Eigen::Vector2d& p = o.vertices[idx];
			output << " L " << p.x() << "," << -p.y();
			if ((idx % 6) == 5) {
				output << "\n";
			}
		}
		output << " z\n";
	}
	output << "\" stroke=\"black\" fill=\"lightgray\" stroke-width=\"0.5\"/>\n";

}

static void append_svg(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	if (dynamic_cast<const PolySet *>(geom.get())) {
		assert(false && "Unsupported file format");
	}
	else if (const Polygon2d *poly = dynamic_cast<const Polygon2d *>(geom.get())) {
		append_svg(*poly, output);
	} else {
		assert(false && "Export as SVG for this geometry type is not supported");
	}
}

void export_svg(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
	
	BoundingBox bbox = geom->getBoundingBox();
	int minx = floor(bbox.min().x());
	int miny = floor(-bbox.max().y());
	int maxx = ceil(bbox.max().x());
	int maxy = ceil(-bbox.min().y());
	int width = maxx - minx;
	int height = maxy - miny;

	output
		<< "<?xml version=\"1.0\" standalone=\"no\"?>\n"
		<< "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
		<< "<svg width=\"" << width << "mm\" height=\"" << height
		<< "mm\" viewBox=\"" << minx << " " << miny << " " << width << " " << height
		<< "\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"
		<< "<title>OpenSCAD Model</title>\n";

	append_svg(geom, output);

	output << "</svg>\n";	
	setlocale(LC_NUMERIC, "");      // Set default locale
}
