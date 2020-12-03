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
#include "dxfdata.h"

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

namespace {

std::string toString(const Vector3d &v)
{
	return STR(v[0] << " " << v[1] << " " << v[2]);
}

Vector3d fromString(const std::string &vertexString)
{
	Vector3d v;
	std::istringstream stream{vertexString};
	stream >> v[0] >> v[1] >> v[2];
	return v;
}

void write_vector(std::ostream &output, const Vector3f& v) {
    for (int i=0; i<3; ++i) {
        static_assert(sizeof(float)==4, "Need 32 bit float");
        float f = v[i];
        char *fbeg = reinterpret_cast<char *>(&f);
        char data[4];

        uint16_t test = 0x0001;
        if (*reinterpret_cast<char *>(&test) == 1) {
            std::copy(fbeg, fbeg+4, data);
        }
        else {
            std::reverse_copy(fbeg, fbeg+4, data);
        }
        output.write(data, 4);
    }
}

	
size_t append_stl(const PolySet &ps, std::ostream &output, bool binary)
{
    size_t triangle_count = 0;
    PolySet triangulated(3);
    PolysetUtils::tessellate_faces(ps, triangulated);

	for(const auto &p : triangulated.polygons) {
		assert(p.size() == 3); // STL only allows triangles
        triangle_count++;

        if (binary) {
            Vector3f p0 = p[0].cast<float>();
            Vector3f p1 = p[1].cast<float>();
            Vector3f p2 = p[2].cast<float>();

            // Ensure 3 distinct vertices.
            if ((p0 != p1) && (p0 != p2) && (p1 != p2)) {
                Vector3f normal = (p1 - p0).cross(p2 - p0);
                normal.normalize();
                if (!is_finite(normal) || is_nan(normal)) {
                    // Collinear vertices.
                    normal << 0, 0, 0;
                }
                write_vector(output, normal);
            }
            write_vector(output, p0);
            write_vector(output, p1);
            write_vector(output, p2);
            char attrib[2] = {0,0};
            output.write(attrib, 2);
        }
        else { // ascii
            std::array<std::string, 3> vertexStrings;
            std::transform(p.cbegin(), p.cend(), vertexStrings.begin(),
                toString);

            if (vertexStrings[0] != vertexStrings[1] &&
                vertexStrings[0] != vertexStrings[2] &&
                vertexStrings[1] != vertexStrings[2]) {

                // The above condition ensures that there are 3 distinct
                // vertices, but they may be collinear. If they are, the unit
                // normal is meaningless so the default value of "0 0 0" can
                // be used. If the vertices are not collinear then the unit
                // normal must be calculated from the components.
                output << "  facet normal ";

                Vector3d p0 = fromString(vertexStrings[0]);
                Vector3d p1 = fromString(vertexStrings[1]);
                Vector3d p2 = fromString(vertexStrings[2]);

                Vector3d normal = (p1 - p0).cross(p2 - p0);
                normal.normalize();
                if (is_finite(normal) && !is_nan(normal)) {
                    output << normal[0] << " " << normal[1] << " " << normal[2]
                      << "\n";
                }
                else {
                    output << "0 0 0\n";
                }
                output << "    outer loop\n";

                for (const auto &vertexString : vertexStrings) {
                    output << "      vertex " << vertexString << "\n";
                }
                output << "    endloop\n";
                output << "  endfacet\n";
            }
        }
    }

    return triangle_count;
}

/*!
	Saves the current 3D CGAL Nef polyhedron as STL to the given file.
	The file must be open.
 */
size_t append_stl(const CGAL_Nef_polyhedron &root_N, std::ostream &output,
    bool binary)
{
  size_t triangle_count = 0;
	if (!root_N.p3->is_simple()) {
		LOG(message_group::Export_Warning,Location::NONE,"","Exported object may not be a valid 2-manifold and may need repair");
	}

	PolySet ps(3);
	if (!CGALUtils::createPolySetFromNefPolyhedron3(*(root_N.p3), ps)) {
		triangle_count += append_stl(ps, output, binary);
	}
	else {
		LOG(message_group::Export_Error,Location::NONE,"","Nef->PolySet failed");
	}

  return triangle_count;
}

size_t append_stl(const shared_ptr<const Geometry> &geom, std::ostream &output,
    bool binary)
{
    size_t triangle_count = 0;
	if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
		for(const Geometry::GeometryItem &item : geomlist->getChildren()) {
			triangle_count += append_stl(item.second, output, binary);
		}
	}
	else if (const auto N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		triangle_count += append_stl(*N, output, binary);
	}
	else if (const auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
		triangle_count += append_stl(*ps, output, binary);
	}
	else if (dynamic_pointer_cast<const Polygon2d>(geom)) {
		assert(false && "Unsupported file format");
	} else {
		assert(false && "Not implemented");
	}

    return triangle_count;
}

} // namespace

void export_stl(const shared_ptr<const Geometry> &geom, std::ostream &output,
    bool binary)
{
    if (binary) {
      char header[80] = "OpenSCAD Model\n";
      output.write(header, sizeof(header));
      char tmp_triangle_count[4] = {0,0,0,0};  // We must fill this in below.
      output.write(tmp_triangle_count, 4);
    }
    else {
        setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
        output << "solid OpenSCAD_Model\n";
    }

	size_t triangle_count = append_stl(geom, output, binary);

    if (binary) {
        // Fill in triangle count.
        output.seekp(80, std::ios_base::beg);
        output.put(triangle_count & 0xff);
        output.put((triangle_count >> 8) & 0xff);
        output.put((triangle_count >> 16) & 0xff);
        output.put((triangle_count >> 24) & 0xff);
        if (triangle_count > 4294967295) {
            LOG(message_group::Export_Error,Location::NONE,"","Triangle count exceeded 4294967295, so the stl file is not valid");
        }
    }
    else {
        output << "endsolid OpenSCAD_Model\n";
        setlocale(LC_NUMERIC, "");      // Set default locale
    }
}

#endif // ENABLE_CGAL
