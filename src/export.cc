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

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

struct triangle {
    std::string vs1;
    std::string vs2;
    std::string vs3;
};

void exportFile(const class Geometry *root_geom, std::ostream &output, FileFormat format)
{
	if (const CGAL_Nef_polyhedron *N = dynamic_cast<const CGAL_Nef_polyhedron *>(root_geom)) {

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
	else {
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
		PRINTB("Can't open file \"%s\" for export", name2display);
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
			PRINTB("ERROR: \"%s\" write error. (Disk full?)", name2display);
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
			Vector3d normal = (p[1] - p[0]).cross(p[2] - p[0]);
			normal.normalize();
			output << "  facet normal " << normal[0] << " " << normal[1] << " " << normal[2] << "\n";
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

/*!
	Saves the given CGAL Polyhedon2 as STL to the given file.
	The file must be open.
 */
static void export_stl(const CGAL_Polyhedron &P, std::ostream &output)
{
	typedef CGAL_Polyhedron::Vertex                                 Vertex;
	typedef CGAL_Polyhedron::Vertex_const_iterator                  VCI;
	typedef CGAL_Polyhedron::Facet_const_iterator                   FCI;
	typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output

	output << "solid OpenSCAD_Model\n";

	for (FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
		HFCC hc = fi->facet_begin();
		HFCC hc_end = hc;
		Vertex v1, v2, v3;
		v1 = *VCI((hc++)->vertex());
		v3 = *VCI((hc++)->vertex());
		do {
			v2 = v3;
			v3 = *VCI((hc++)->vertex());
			double x1 = CGAL::to_double(v1.point().x());
			double y1 = CGAL::to_double(v1.point().y());
			double z1 = CGAL::to_double(v1.point().z());
			double x2 = CGAL::to_double(v2.point().x());
			double y2 = CGAL::to_double(v2.point().y());
			double z2 = CGAL::to_double(v2.point().z());
			double x3 = CGAL::to_double(v3.point().x());
			double y3 = CGAL::to_double(v3.point().y());
			double z3 = CGAL::to_double(v3.point().z());
			std::stringstream stream;
			stream << x1 << " " << y1 << " " << z1;
			std::string vs1 = stream.str();
			stream.str("");
			stream << x2 << " " << y2 << " " << z2;
			std::string vs2 = stream.str();
			stream.str("");
			stream << x3 << " " << y3 << " " << z3;
			std::string vs3 = stream.str();
			if (vs1 != vs2 && vs1 != vs3 && vs2 != vs3) {
				// The above condition ensures that there are 3 distinct vertices, but
				// they may be collinear. If they are, the unit normal is meaningless
				// so the default value of "1 0 0" can be used. If the vertices are not
				// collinear then the unit normal must be calculated from the
				// components.
				if (!CGAL::collinear(v1.point(),v2.point(),v3.point())) {
					CGAL_Polyhedron::Traits::Vector_3 normal = CGAL::normal(v1.point(),v2.point(),v3.point());
					output << "  facet normal "
								 << CGAL::sign(normal.x()) * sqrt(CGAL::to_double(normal.x()*normal.x()/normal.squared_length()))
								 << " "
								 << CGAL::sign(normal.y()) * sqrt(CGAL::to_double(normal.y()*normal.y()/normal.squared_length()))
								 << " "
								 << CGAL::sign(normal.z()) * sqrt(CGAL::to_double(normal.z()*normal.z()/normal.squared_length()))
								 << "\n";
				}
				else output << "  facet normal 1 0 0\n";
				output << "    outer loop\n";
				output << "      vertex " << vs1 << "\n";
				output << "      vertex " << vs2 << "\n";
				output << "      vertex " << vs3 << "\n";
				output << "    endloop\n";
				output << "  endfacet\n";
			}
		} while (hc != hc_end);
	}

	output << "endsolid OpenSCAD_Model\n";
	setlocale(LC_NUMERIC, "");      // Set default locale
}

/*!
	Saves the current 3D CGAL Nef polyhedron as STL to the given file.
	The file must be open.
 */
void export_stl(const CGAL_Nef_polyhedron *root_N, std::ostream &output)
{
	if (!root_N->p3->is_simple()) {
		PRINT("WARNING: Exported object may not be a valid 2-manifold and may need repair");
	}

	bool usePolySet = true;
	if (usePolySet) {
		PolySet ps(3);
		bool err = CGALUtils::createPolySetFromNefPolyhedron3(*(root_N->p3), ps);
		if (err) { PRINT("ERROR: Nef->PolySet failed"); }
		else {
			export_stl(ps, output);
		}
	}
	else {
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			CGAL_Polyhedron P;
			//root_N->p3->convert_to_Polyhedron(P);
			bool err = nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>( *(root_N->p3), P );
			if (err) {
				PRINT("ERROR: CGAL NefPolyhedron->Polyhedron conversion failed");
				return;
			}
			export_stl(P, output);
		}
		catch (const CGAL::Assertion_exception &e) {
			PRINTB("ERROR: CGAL error in CGAL_Nef_polyhedron3::convert_to_Polyhedron(): %s", e.what());
		}
		catch (...) {
			PRINT("ERROR: CGAL unknown error in CGAL_Nef_polyhedron3::convert_to_Polyhedron()");
		}
		CGAL::set_error_behaviour(old_behaviour);
	}
}

void export_off(const class PolySet &ps, std::ostream &output)
{
	// FIXME: Implement this without creating a Nef polyhedron
	CGAL_Nef_polyhedron *N = CGALUtils::createNefPolyhedronFromGeometry(ps);
	export_off(N, output);
	delete N;
}

void export_off(const CGAL_Nef_polyhedron *root_N, std::ostream &output)
{
	if (!root_N->p3->is_simple()) {
		PRINT("WARNING: Export failed, the object isn't a valid 2-manifold.");
		return;
	}
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CGAL_Polyhedron P;
		root_N->p3->convert_to_Polyhedron(P);
		output << P;
	}
	catch (const CGAL::Assertion_exception &e) {
		PRINTB("ERROR: CGAL error in CGAL_Nef_polyhedron3::convert_to_Polyhedron(): %s", e.what());
	}
	CGAL::set_error_behaviour(old_behaviour);
}

void export_amf(const class PolySet &ps, std::ostream &output)
{
	// FIXME: Implement this without creating a Nef polyhedron
	CGAL_Nef_polyhedron *N = CGALUtils::createNefPolyhedronFromGeometry(ps);
	export_amf(N, output);
	delete N;
}

/*!
    Saves the current 3D CGAL Nef polyhedron as AMF to the given file.
    The file must be open.
 */
void export_amf(const CGAL_Nef_polyhedron *root_N, std::ostream &output)
{
	if (!root_N->p3->is_simple()) {
		PRINT("WARNING: Export failed, the object isn't a valid 2-manifold.");
		return;
	}
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CGAL_Polyhedron P;
		root_N->p3->convert_to_Polyhedron(P);

		typedef CGAL_Polyhedron::Vertex Vertex;
		typedef CGAL_Polyhedron::Vertex_const_iterator VCI;
		typedef CGAL_Polyhedron::Facet_const_iterator FCI;
		typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

		setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output

		std::vector<std::string> vertices;
		std::vector<triangle> triangles;

		for (FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
			HFCC hc = fi->facet_begin();
			HFCC hc_end = hc;
			Vertex v1, v2, v3;
			v1 = *VCI((hc++)->vertex());
			v3 = *VCI((hc++)->vertex());
			do {
				v2 = v3;
				v3 = *VCI((hc++)->vertex());
				double x1 = CGAL::to_double(v1.point().x());
				double y1 = CGAL::to_double(v1.point().y());
				double z1 = CGAL::to_double(v1.point().z());
				double x2 = CGAL::to_double(v2.point().x());
				double y2 = CGAL::to_double(v2.point().y());
				double z2 = CGAL::to_double(v2.point().z());
				double x3 = CGAL::to_double(v3.point().x());
				double y3 = CGAL::to_double(v3.point().y());
				double z3 = CGAL::to_double(v3.point().z());
				std::stringstream stream;
				stream << x1 << " " << y1 << " " << z1;
				std::string vs1 = stream.str();
				stream.str("");
				stream << x2 << " " << y2 << " " << z2;
				std::string vs2 = stream.str();
				stream.str("");
				stream << x3 << " " << y3 << " " << z3;
				std::string vs3 = stream.str();
				if (std::find(vertices.begin(), vertices.end(), vs1) == vertices.end())
					vertices.push_back(vs1);
				if (std::find(vertices.begin(), vertices.end(), vs2) == vertices.end())
					vertices.push_back(vs2);
				if (std::find(vertices.begin(), vertices.end(), vs3) == vertices.end())
					vertices.push_back(vs3);

				if (vs1 != vs2 && vs1 != vs3 && vs2 != vs3) {
					// The above condition ensures that there are 3 distinct vertices, but
					// they may be collinear. If they are, the unit normal is meaningless
					// so the default value of "1 0 0" can be used. If the vertices are not
					// collinear then the unit normal must be calculated from the
					// components.
					triangle tri = {vs1, vs2, vs3};
					triangles.push_back(tri);
				}
			} while (hc != hc_end);
		}

		output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
			<< "<amf unit=\"millimeter\">\r\n"
			<< " <metadata type=\"producer\">OpenSCAD " << QUOTED(OPENSCAD_VERSION)
#ifdef OPENSCAD_COMMIT
			<< " (git " << QUOTED(OPENSCAD_COMMIT) << ")"
#endif
			<< "</metadata>\r\n"
			<< " <object id=\"0\">\r\n"
			<< "  <mesh>\r\n";
		output << "   <vertices>\r\n";
		for (size_t i = 0; i < vertices.size(); i++) {
			std::string s = vertices[i];
			output << "    <vertex><coordinates>\r\n";
			char* chrs = new char[s.length() + 1];
			strcpy(chrs, s.c_str());
			std::string coords = strtok(chrs, " ");
			output << "     <x>" << coords << "</x>\r\n";
			coords = strtok(NULL, " ");
			output << "     <y>" << coords << "</y>\r\n";
			coords = strtok(NULL, " ");
			output << "     <z>" << coords << "</z>\r\n";
			output << "    </coordinates></vertex>\r\n";
		}
		output << "   </vertices>\r\n";
		output << "   <volume>\r\n";
		for (size_t i = 0; i < triangles.size(); i++) {
			triangle t = triangles[i];
			output << "    <triangle>\r\n";
			size_t index;
			index = std::distance(vertices.begin(), std::find(vertices.begin(), vertices.end(), t.vs1));
			output << "     <v1>" << index << "</v1>\r\n";
			index = std::distance(vertices.begin(), std::find(vertices.begin(), vertices.end(), t.vs2));
			output << "     <v2>" << index << "</v2>\r\n";
			index = std::distance(vertices.begin(), std::find(vertices.begin(), vertices.end(), t.vs3));
			output << "     <v3>" << index << "</v3>\r\n";
			output << "    </triangle>\r\n";
		}
		output << "   </volume>\r\n";
		output << "  </mesh>\r\n"
			<< " </object>\r\n"
			<< "</amf>\r\n";
	} catch (CGAL::Assertion_exception e) {
		PRINTB("ERROR: CGAL error in CGAL_Nef_polyhedron3::convert_to_Polyhedron(): %s", e.what());
	}
	CGAL::set_error_behaviour(old_behaviour);
	setlocale(LC_NUMERIC, ""); // Set default locale
}

#endif // ENABLE_CGAL

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

