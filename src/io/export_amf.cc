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

#ifdef ENABLE_CGAL
#include "cgal.h"
#include "cgalutils.h"
#include "CGAL_Nef_polyhedron.h"

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

struct vertex_str {
  std::string x, y, z;
  bool operator==(const vertex_str& rhs) {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }
};

using Vertex = CGAL_Polyhedron::Vertex;
using Point = Vertex::Point;
using VCI = CGAL_Polyhedron::Vertex_const_iterator;
using FCI = CGAL_Polyhedron::Facet_const_iterator;
using HFCC = CGAL_Polyhedron::Halfedge_around_facet_const_circulator;
using vertex_vec = std::vector<vertex_str>;

struct triangle {
  size_t vi1, vi2, vi3;
};

static int objectid;

static size_t add_vertex(std::vector<vertex_str>& vertices, const Point& p) {
  double x = CGAL::to_double(p.x());
  double y = CGAL::to_double(p.y());
  double z = CGAL::to_double(p.z());
  vertex_str vs{STR(x), STR(y), STR(z)};
  auto vi = std::find(vertices.begin(), vertices.end(), vs);
  if (vi == vertices.end()) {
    vertices.push_back(vs);
    return vertices.size() - 1;
  } else {
    return std::distance(vertices.begin(), vi);
  }
}

/*!
    Saves the current 3D CGAL Nef polyhedron as AMF to the given file.
    The file must be open.
 */
static void append_amf(const CGAL_Nef_polyhedron& root_N, std::ostream& output)
{
  if (!root_N.p3->is_simple()) {
    LOG(message_group::Export_Warning, "Export failed, the object isn't a valid 2-manifold.");
    return;
  }
  try {
    CGAL_Polyhedron P;
    CGALUtils::convertNefToPolyhedron(*root_N.p3, P);

    vertex_vec vertices;
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
        auto vi1 = add_vertex(vertices, v1.point());
        auto vi2 = add_vertex(vertices, v2.point());
        auto vi3 = add_vertex(vertices, v3.point());
        if (vi1 != vi2 && vi1 != vi3 && vi2 != vi3) {
          // The above condition ensures that there are 3 distinct vertices, but
          // they may be collinear. If they are, the unit normal is meaningless
          // so the default value of "1 0 0" can be used. If the vertices are not
          // collinear then the unit normal must be calculated from the
          // components.
          triangles.push_back({vi1, vi2, vi3});
        }
      } while (hc != hc_end);
    }

    output << " <object id=\"" << objectid++ << "\">\r\n"
           << "  <mesh>\r\n";
    output << "   <vertices>\r\n";
    for (const auto& s : vertices) {
      output << "    <vertex><coordinates>\r\n";
      output << "     <x>" << s.x << "</x>\r\n";
      output << "     <y>" << s.y << "</y>\r\n";
      output << "     <z>" << s.z << "</z>\r\n";
      output << "    </coordinates></vertex>\r\n";
    }
    output << "   </vertices>\r\n";
    output << "   <volume>\r\n";
    for (auto t : triangles) {
      output << "    <triangle>\r\n";
      output << "     <v1>" << t.vi1 << "</v1>\r\n";
      output << "     <v2>" << t.vi2 << "</v2>\r\n";
      output << "     <v3>" << t.vi3 << "</v3>\r\n";
      output << "    </triangle>\r\n";
    }
    output << "   </volume>\r\n";
    output << "  </mesh>\r\n"
           << " </object>\r\n";
  } catch (CGAL::Assertion_exception& e) {
    LOG(message_group::Export_Error, "CGAL error in CGAL_Nef_polyhedron3::convert_to_polyhedron(): %1$s", e.what());
  }
}

static void append_amf(const shared_ptr<const Geometry>& geom, std::ostream& output)
{
  if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      append_amf(item.second, output);
    }
  } else if (auto N = CGALUtils::getNefPolyhedronFromGeometry(geom)) {
    // FIXME: Implement this without creating a Nef polyhedron
    if (!N->isEmpty()) append_amf(*N, output);
  } else if (geom->getDimension() != 3) { // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else { // NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }
}

void export_amf(const shared_ptr<const Geometry>& geom, std::ostream& output)
{
  setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output

  output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         << "<amf unit=\"millimeter\">\r\n"
         << " <metadata type=\"producer\">OpenSCAD " << QUOTED(OPENSCAD_VERSION)
#ifdef OPENSCAD_COMMIT
    << " (git " << QUOTED(OPENSCAD_COMMIT) << ")"
#endif
    << "</metadata>\r\n";

  objectid = 0;
  append_amf(geom, output);

  output << "</amf>\r\n";
  setlocale(LC_NUMERIC, ""); // Set default locale
}

#endif // ENABLE_CGAL
