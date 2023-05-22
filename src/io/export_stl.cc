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
#include "PolySet.h"
#include "PolySetUtils.h"
#include "Reindexer.h"
#include "double-conversion/double-conversion.h"
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#endif

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "CGALHybridPolyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

namespace {

/* Define values for double-conversion library. */
#define DC_BUFFER_SIZE (128)
#define DC_FLAGS (double_conversion::DoubleToStringConverter::UNIQUE_ZERO | double_conversion::DoubleToStringConverter::EMIT_POSITIVE_EXPONENT_SIGN)
#define DC_INF NULL // Only finite values in STL outputs!
#define DC_NAN NULL
#define DC_EXP 'e'
#define DC_DECIMAL_LOW_EXP (-6)
#define DC_DECIMAL_HIGH_EXP (21)
#define DC_MAX_LEADING_ZEROES (5)
#define DC_MAX_TRAILING_ZEROES (0)

std::string toString(const Vector3f& v)
{
  double_conversion::DoubleToStringConverter dc(
    DC_FLAGS, DC_INF, DC_NAN, DC_EXP,
    DC_DECIMAL_LOW_EXP, DC_DECIMAL_HIGH_EXP, DC_MAX_LEADING_ZEROES, DC_MAX_TRAILING_ZEROES
  );

  char buffer[DC_BUFFER_SIZE];

  double_conversion::StringBuilder builder(buffer, DC_BUFFER_SIZE);
  dc.ToShortestSingle(v[0], &builder);
  builder.AddCharacter(' ');
  dc.ToShortestSingle(v[1], &builder);
  builder.AddCharacter(' ');
  dc.ToShortestSingle(v[2], &builder);
  builder.Finalize();

  return buffer;
}

void write_vector(std::ostream& output, const Vector3f& v) {
  for (int i = 0; i < 3; ++i) {
    static_assert(sizeof(float) == 4, "Need 32 bit float");
    float f = v[i];
    char *fbeg = reinterpret_cast<char *>(&f);
    char data[4];

    uint16_t test = 0x0001;
    if (*reinterpret_cast<char *>(&test) == 1) {
      std::copy(fbeg, fbeg + 4, data);
    } else {
      std::reverse_copy(fbeg, fbeg + 4, data);
    }
    output.write(data, 4);
  }
}

uint64_t append_stl(const IndexedTriangleMesh& mesh, std::ostream& output, bool binary)
{
  uint64_t triangle_count = 0;
  
  // In ASCII mode only, convert each vertex to string.
  std::vector<std::string> vertexStrings;
  if (!binary) {
    vertexStrings.resize(mesh.vertices.size());
    std::transform(mesh.vertices.begin(), mesh.vertices.end(), vertexStrings.begin(),
      [](auto &p) { return toString(p); });
  }

  for (const auto &t : mesh.triangles) {
    const auto &p0 = mesh.vertices[t[0]];
    const auto &p1 = mesh.vertices[t[1]];
    const auto &p2 = mesh.vertices[t[2]];

    // Tessellation already eliminated these cases.
    assert(p0 != p1 && p0 != p2 && p1 != p2);

    auto normal = (p1 - p0).cross(p2 - p0);
    if (!normal.isZero()) {
      normal.normalize();
    }

    if (binary) {
      write_vector(output, normal);
      write_vector(output, p0);
      write_vector(output, p1);
      write_vector(output, p2);
      char attrib[2] = {0, 0};
      output.write(attrib, 2);
    } else {
      const auto &s0 = vertexStrings[t[0]];
      const auto &s1 = vertexStrings[t[1]];
      const auto &s2 = vertexStrings[t[2]];

      // Since the points are different, the precision we use to 
      // format them to string should guarantee the strings are 
      // different too.
      assert(s0 != s1 && s0 != s2 && s1 != s2);
      
      output << "  facet normal ";
      output << toString(normal) << "\n";
      output << "    outer loop\n";
      output << "      vertex " << s0 << "\n";
      output << "      vertex " << s1 << "\n";
      output << "      vertex " << s2 << "\n";
      output << "    endloop\n";
      output << "  endfacet\n";
    }
    triangle_count++;
  }

  return triangle_count;
}

uint64_t append_stl(const PolySet& ps, std::ostream& output, bool binary)
{
  IndexedTriangleMesh triangulated;
  PolySetUtils::tessellate_faces(ps, triangulated);

  if (Feature::ExperimentalPredictibleOutput.is_enabled()) {
    IndexedTriangleMesh ordered;
    Export::ExportMesh exportMesh { triangulated };
    exportMesh.export_indexed(ordered);
    return append_stl(ordered, output, binary);
  } else {
    return append_stl(triangulated, output, binary);
  }
}

/*!
    Saves the current 3D CGAL Nef polyhedron as STL to the given file.
    The file must be open.
 */
uint64_t append_stl(const CGAL_Nef_polyhedron& root_N, std::ostream& output,
                    bool binary)
{
  uint64_t triangle_count = 0;
  if (!root_N.p3->is_simple()) {
    LOG(message_group::Export_Warning, "Exported object may not be a valid 2-manifold and may need repair");
  }

  PolySet ps(3);
  if (!CGALUtils::createPolySetFromNefPolyhedron3(*(root_N.p3), ps)) {
    triangle_count += append_stl(ps, output, binary);
  } else {
    LOG(message_group::Export_Error, "Nef->PolySet failed");
  }

  return triangle_count;
}

/*!
   Saves the current 3D CGAL Nef polyhedron as STL to the given file.
   The file must be open.
 */
uint64_t append_stl(const CGALHybridPolyhedron& hybrid, std::ostream& output,
                    bool binary)
{
  uint64_t triangle_count = 0;
  if (!hybrid.isManifold()) {
    LOG(message_group::Export_Warning, "Exported object may not be a valid 2-manifold and may need repair");
  }

  auto ps = hybrid.toPolySet();
  if (ps) {
    triangle_count += append_stl(*ps, output, binary);
  } else {
    LOG(message_group::Export_Error, "Nef->PolySet failed");
  }

  return triangle_count;
}

#ifdef ENABLE_MANIFOLD
/*!
   Saves the current 3D Manifold geometry as STL to the given file.
   The file must be open.
 */
uint64_t append_stl(const ManifoldGeometry& mani, std::ostream& output,
                    bool binary)
{
  uint64_t triangle_count = 0;
  if (!mani.isManifold()) {
    LOG(message_group::Export_Warning, "Exported object may not be a valid 2-manifold and may need repair");
  }

  auto ps = mani.toPolySet();
  if (ps) {
    triangle_count += append_stl(*ps, output, binary);
  } else {
    LOG(message_group::Export_Error, "Manifold->PolySet failed");
  }

  return triangle_count;
}
#endif


uint64_t append_stl(const shared_ptr<const Geometry>& geom, std::ostream& output,
                    bool binary)
{
  uint64_t triangle_count = 0;
  if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const Geometry::GeometryItem& item : geomlist->getChildren()) {
      triangle_count += append_stl(item.second, output, binary);
    }
  } else if (const auto N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    triangle_count += append_stl(*N, output, binary);
  } else if (const auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
    triangle_count += append_stl(*ps, output, binary);
  } else if (const auto hybrid = dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    triangle_count += append_stl(*hybrid, output, binary);
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    triangle_count += append_stl(*mani, output, binary);
#endif
  } else if (dynamic_pointer_cast<const Polygon2d>(geom)) { //NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else { //NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }

  return triangle_count;
}

} // namespace

void export_stl(const shared_ptr<const Geometry>& geom, std::ostream& output,
                bool binary)
{
  if (binary) {
    char header[80] = "OpenSCAD Model\n";
    output.write(header, sizeof(header));
    char tmp_triangle_count[4] = {0, 0, 0, 0}; // We must fill this in below.
    output.write(tmp_triangle_count, 4);
  } else {
    setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
    output << "solid OpenSCAD_Model\n";
  }

  uint64_t triangle_count = append_stl(geom, output, binary);

  if (binary) {
    // Fill in triangle count.
    output.seekp(80, std::ios_base::beg);
    output.put(triangle_count & 0xff);
    output.put((triangle_count >> 8) & 0xff);
    output.put((triangle_count >> 16) & 0xff);
    output.put((triangle_count >> 24) & 0xff);
    if (triangle_count > 4294967295) {
      LOG(message_group::Export_Error, "Triangle count exceeded 4294967295, so the stl file is not valid");
    }
  } else {
    output << "endsolid OpenSCAD_Model\n";
    setlocale(LC_NUMERIC, ""); // Set default locale
  }
}

#endif // ENABLE_CGAL
