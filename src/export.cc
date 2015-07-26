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
#include "printutils.h"
#include "polyset.h"
#include "polyset-utils.h"
#include "dxfdata.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

#include "CSGIF.h"

#include <fstream>

struct triangle {
    std::string vs1;
    std::string vs2;
    std::string vs3;
};

void exportFile(const class Geometry *root_geom, std::ostream &output, FileFormat format)
{
#ifdef ENABLE_CSGIF
	if (const CSGIF_polyhedron *N = dynamic_cast<const CSGIF_polyhedron *>(root_geom)) {

		switch (format) {
		case OPENSCAD_STL:
			export_stl(N, output);
			break;
		case OPENSCAD_OFF:
			export_off(N, output);
			break;
		case OPENSCAD_AMF:
			export_amf(N, output);
			break;
		case OPENSCAD_DXF:
			assert(false && "Export Nef polyhedron as DXF not supported");
			break;
		default:
			assert(false && "Unknown file format");
		}
	}
	else
#endif // ENABLE_CSGIF
        {
		if (const PolySet *ps = dynamic_cast<const PolySet *>(root_geom)) {
			switch (format) {
			case OPENSCAD_STL:
				export_stl(*ps, output);
				break;
			case OPENSCAD_OFF:
				export_off(*ps, output);
				break;
			case OPENSCAD_AMF:
				export_amf(*ps, output);
				break;
			default:
				assert(false && "Unsupported file format");
			}
		}
		else if (const Polygon2d *poly = dynamic_cast<const Polygon2d *>(root_geom)) {
			switch (format) {
			case OPENSCAD_SVG:
				export_svg(*poly, output);
				break;
			case OPENSCAD_DXF:
				export_dxf(*poly, output);
				break;
			default:
				assert(false && "Unsupported file format");
			}
		} else {
			assert(false && "Not implemented");
		}
	}
}

void exportFileByName(const class Geometry *root_geom, FileFormat format,
	const char *name2open, const char *name2display)
{
	std::ofstream fstream(name2open);
	if (!fstream.is_open()) {
		PRINTB(_("Can't open file \"%s\" for export"), name2display);
	} else {
		bool onerror = false;
		fstream.exceptions(std::ios::badbit|std::ios::failbit);
		try {
			exportFile(root_geom, fstream, format);
		} catch (std::ios::failure x) {
			onerror = true;
		}
		try { // make sure file closed - resources released
			fstream.close();
		} catch (std::ios::failure x) {
			onerror = true;
		}
		if (onerror) {
			PRINTB(_("ERROR: \"%s\" write error. (Disk full?)"), name2display);
		}
	}
}

void export_stl(const PolySet &ps, std::ostream &output)
{
	PolySet triangulated(3);
	PolysetUtils::tessellate_faces(ps, triangulated);

	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
	output << "solid OpenSCAD_Model\n";
	BOOST_FOREACH(const Polygon &p, triangulated.polygons) {
		assert(p.size() == 3); // STL only allows triangles
		std::stringstream stream;
		stream << p[0][0] << " " << p[0][1] << " " << p[0][2];
		std::string vs1 = stream.str();
		stream.str("");
		stream << p[1][0] << " " << p[1][1] << " " << p[1][2];
		std::string vs2 = stream.str();
		stream.str("");
		stream << p[2][0] << " " << p[2][1] << " " << p[2][2];
		std::string vs3 = stream.str();
		if (vs1 != vs2 && vs1 != vs3 && vs2 != vs3) {
			// The above condition ensures that there are 3 distinct vertices, but
			// they may be collinear. If they are, the unit normal is meaningless
			// so the default value of "1 0 0" can be used. If the vertices are not
			// collinear then the unit normal must be calculated from the
			// components.
			output << "  facet normal ";

			Vector3d normal = (p[1] - p[0]).cross(p[2] - p[0]);
			normal.normalize();
			if (is_finite(normal) && !is_nan(normal)) {
				output << normal[0] << " " << normal[1] << " " << normal[2] << "\n";
			}
			else {
				output << "0 0 0\n";
			}
			output << "    outer loop\n";
		
			BOOST_FOREACH(const Vector3d &v, p) {
				output << "      vertex " << v[0] << " " << v[1] << " " << v[2] << "\n";
			}
			output << "    endloop\n";
			output << "  endfacet\n";
		}
	}
	output << "endsolid OpenSCAD_Model\n";
	setlocale(LC_NUMERIC, "");      // Set default locale
}


void export_off(const class PolySet &ps, std::ostream &output)
{
#ifdef ENABLE_CSGIF
	// FIXME: Implement this without creating a CSG polyhedron
	CSGIF_polyhedron *N = CSGIF_Utils::createCsgPolyhedronFromGeometry(ps);
	export_off(N, output);
	delete N;
#endif // ENABLE_CSGIF
}

void export_amf(const class PolySet &ps, std::ostream &output)
{
#ifdef ENABLE_CSGIF
	// FIXME: Implement this without creating a Csg polyhedron
	CSGIF_polyhedron *N = CSGIF_Utils::createCsgPolyhedronFromGeometry(ps);
	export_amf(N, output);
	delete N;
#endif // ENABLE_CSGIF
}

/*!
	Saves the current Polygon2d as DXF to the given absolute filename.
 */
void export_dxf(const Polygon2d &poly, std::ostream &output)
{
	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
	// Some importers (e.g. Inkscape) needs a BLOCKS section to be present
	output << "  0\n"
				 <<	"SECTION\n"
				 <<	"  2\n"
				 <<	"BLOCKS\n"
				 <<	"  0\n"
				 << "ENDSEC\n"
				 << "  0\n"
				 << "SECTION\n"
				 << "  2\n"
				 << "ENTITIES\n";

	BOOST_FOREACH(const Outline2d &o, poly.outlines()) {
		for (unsigned int i=0;i<o.vertices.size();i++) {
			const Vector2d &p1 = o.vertices[i];
			const Vector2d &p2 = o.vertices[(i+1)%o.vertices.size()];
			double x1 = p1[0];
			double y1 = p1[1];
			double x2 = p2[0];
			double y2 = p2[1];
			output << "  0\n"
						 << "LINE\n";
			// Some importers (e.g. Inkscape) needs a layer to be specified
			output << "  8\n"
						 << "0\n"
						 << " 10\n"
						 << x1 << "\n"
						 << " 20\n"
						 << y1 << "\n"
						 << " 11\n"
						 << x2 << "\n"
						 << " 21\n"
						 << y2 << "\n";
		}
	}

	output << "  0\n"
				 << "ENDSEC\n";

	// Some importers (e.g. Inkscape) needs an OBJECTS section with a DICTIONARY entry
	output << "  0\n"
				 << "SECTION\n"
				 << "  2\n"
				 << "OBJECTS\n"
				 << "  0\n"
				 << "DICTIONARY\n"
				 << "  0\n"
				 << "ENDSEC\n";

	output << "  0\n"
				 <<"EOF\n";

	setlocale(LC_NUMERIC, "");      // Set default locale
}

void export_svg(const Polygon2d &poly, std::ostream &output)
{
	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
	
	BoundingBox bbox = poly.getBoundingBox();
	int minx = floor(bbox.min().x());
	int miny = floor(-bbox.max().y());
	int maxx = ceil(bbox.max().x());
	int maxy = ceil(-bbox.min().y());

	int width = maxx - minx;
	int height = maxy - miny;
	output
		<< "<?xml version=\"1.0\" standalone=\"no\"?>\n"
		<< "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
		<< "<svg width=\"" << width << "\" height=\"" << height
		<< "\" viewBox=\"" << minx << " " << miny << " " << width << " " << height
		<< "\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"
		<< "<title>OpenSCAD Model</title>\n";

	output << "<path d=\"\n";
	BOOST_FOREACH(const Outline2d &o, poly.outlines()) {
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
	output << "\" stroke=\"black\" fill=\"lightgray\" stroke-width=\"0.5\"/>";

	output << "</svg>\n";	

	setlocale(LC_NUMERIC, "");      // Set default locale
}

