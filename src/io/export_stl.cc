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

#include <double-conversion/double-conversion.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <clocale>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "Feature.h"
#include "geometry/Geometry.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "geometry/linalg.h"
#include "io/export.h"
#include "utils/printutils.h"

#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALNefGeometry.h"
#include "geometry/cgal/cgalutils.h"
#endif

namespace {
/* Define values for double-conversion library. */
#define DC_BUFFER_SIZE (128)
#define DC_FLAGS (double_conversion::DoubleToStringConverter::UNIQUE_ZERO)
#define DC_INF NULL  // Only finite values in STL outputs!
#define DC_NAN NULL
#define DC_EXP 'e'
#define DC_DECIMAL_LOW_EXP (-6)
#define DC_DECIMAL_HIGH_EXP (21)
#define DC_MAX_LEADING_ZEROES (5)
#define DC_MAX_TRAILING_ZEROES (0)

std::string toString(const Vector3d& v)
{
  const double_conversion::DoubleToStringConverter dc(DC_FLAGS, DC_INF, DC_NAN, DC_EXP,
                                                      DC_DECIMAL_LOW_EXP, DC_DECIMAL_HIGH_EXP,
                                                      DC_MAX_LEADING_ZEROES, DC_MAX_TRAILING_ZEROES);

  char buffer[DC_BUFFER_SIZE];

  double_conversion::StringBuilder builder(buffer, DC_BUFFER_SIZE);
  dc.ToShortest(v[0], &builder);
  builder.AddCharacter(' ');
  dc.ToShortest(v[1], &builder);
  builder.AddCharacter(' ');
  dc.ToShortest(v[2], &builder);
  builder.Finalize();

  return buffer;
}

int32_t flipEndianness(int32_t x)
{
  return ((x << 24) & 0xff000000) | ((x >> 24) & 0xff) | ((x << 8) & 0xff0000) | ((x >> 8) & 0xff00);
}

template <size_t N>
void write_floats(std::ostream& output, const std::array<float, N>& data)
{
  static constexpr uint16_t test = 0x0001;
  static const bool isLittleEndian = *reinterpret_cast<const char *>(&test) == 1;

  if (isLittleEndian) {
    output.write(reinterpret_cast<char *>(const_cast<float *>(&data[0])), N * sizeof(float));
  } else {
    std::array<float, N> copy(data);

    auto *ints = reinterpret_cast<int32_t *>(&copy[0]);
    for (size_t i = 0; i < N; i++) {
      ints[i] = flipEndianness(ints[i]);
    }

    output.write(reinterpret_cast<char *>(&copy[0]), N * sizeof(float));
  }
}

/*!
   Writes the triangles of a prepared PolySet to output.
   The PolySet must be triangular (and, when experimental predictable output
   is enabled, sorted); see preparePolySet() which does that work up front so
   that the total triangle count is known before writing begins.
 */
uint64_t append_stl(const std::shared_ptr<const PolySet>& polyset, std::ostream& output, bool binary)
{
  static_assert(sizeof(float) == 4, "Need 32 bit float");

  const PolySet& ps = *polyset;

  uint64_t triangle_count = 0;

  // In ASCII mode only, convert each vertex to string.
  std::vector<std::string> vertexStrings;
  if (!binary) {
    vertexStrings.resize(ps.vertices.size());
    std::transform(ps.vertices.begin(), ps.vertices.end(), vertexStrings.begin(),
                   [](const auto& p) { return toString(p); });
  }

  // Used for binary mode only
  std::array<float, 4lu * 3> coords;

  for (const auto& t : ps.indices) {
    const auto& p0 = ps.vertices[t[0]];
    const auto& p1 = ps.vertices[t[1]];
    const auto& p2 = ps.vertices[t[2]];

    // Tessellation already eliminated these cases.
    assert(p0 != p1 && p0 != p2 && p1 != p2);

    auto normal = (p1 - p0).cross(p2 - p0);
    if (!normal.isZero(0)) {
      normal.normalize();
    }

    if (binary) {
      auto coords_offset = 0;
      auto addCoords = [&](const auto& v) {
        for (auto i : {0, 1, 2}) coords[coords_offset++] = v[i];
      };
      addCoords(normal);
      addCoords(p0);
      addCoords(p1);
      addCoords(p2);
      assert(coords_offset == 4 * 3);
      write_floats(output, coords);
      char attrib[2] = {0, 0};
      output.write(attrib, 2);
    } else {
      const auto& s0 = vertexStrings[t[0]];
      const auto& s1 = vertexStrings[t[1]];
      const auto& s2 = vertexStrings[t[2]];

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

/*!
   Converts a single (non-list) geometry to a PolySet suitable for STL
   writing: triangular (tessellated when needed) and, when experimental
   predictable output is enabled, deterministically sorted.
   Returns nullptr when the geometry cannot be converted.
 */
std::shared_ptr<const PolySet> preparePolySet(const std::shared_ptr<const Geometry>& geom)
{
  std::shared_ptr<const PolySet> ps;
  if (const auto p = std::dynamic_pointer_cast<const PolySet>(geom)) {
    ps = p;
  }
#ifdef ENABLE_CGAL
  else if (const auto N = std::dynamic_pointer_cast<const CGALNefGeometry>(geom)) {
    if (!N->p3->is_simple()) {
      LOG(message_group::Export_Warning,
          "Exported object may not be a valid 2-manifold and may need repair");
    }
    ps = CGALUtils::createPolySetFromNefPolyhedron3(*(N->p3));
    if (!ps) {
      LOG(message_group::Export_Error, "Nef->PolySet failed");
    }
  }
#endif
#ifdef ENABLE_MANIFOLD
  else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    if (!mani->isManifold()) {
      LOG(message_group::Export_Warning,
          "Exported object may not be a valid 2-manifold and may need repair");
    }
    ps = mani->toPolySet();
    if (!ps) {
      LOG(message_group::Export_Error, "Manifold->PolySet failed");
    }
  }
#endif
  else if (std::dynamic_pointer_cast<const Polygon2d>(geom)) {  // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else {  // NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }

  if (!ps) return nullptr;
  if (!ps->isTriangular()) {
    ps = PolySetUtils::tessellate_faces(*ps);
  }
  if (Feature::ExperimentalPredictibleOutput.is_enabled()) {
    ps = createSortedPolySet(*ps);
  }
  return ps;
}

/*!
   Collects all PolySets that make up the geometry (one per body for
   GeometryLists), each prepared for STL writing, so that the total triangle
   count is known before any bytes are written.
 */
std::vector<std::shared_ptr<const PolySet>> preparePolysets(const std::shared_ptr<const Geometry>& geom)
{
  std::vector<std::shared_ptr<const PolySet>> polysets;
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const Geometry::GeometryItem& item : geomlist->getChildren()) {
      if (!item.second) continue;
      auto children = preparePolysets(item.second);
      polysets.insert(polysets.end(), std::make_move_iterator(children.begin()),
                      std::make_move_iterator(children.end()));
    }
    return polysets;
  }
  if (const auto ps = preparePolySet(geom)) {
    polysets.push_back(std::move(ps));
  }
  return polysets;
}

}  // namespace

void export_stl(const std::shared_ptr<const Geometry>& geom, std::ostream& output, bool binary)
{
  // FIXME: In lazy union mode, should we export multiple solids?
  //
  // Prepare all bodies up front so the total triangle count is known before
  // writing begins. This lets binary STL be streamed directly to output
  // (header first, then triangles) instead of buffering the whole file in
  // memory, which used to hold ~2x the file size in RAM.
  const auto polysets = preparePolysets(geom);

  if (binary) {
    uint64_t triangle_count = 0;
    for (const auto& ps : polysets) {
      triangle_count += ps->indices.size();
    }

    if (triangle_count > 4294967295) {
      LOG(message_group::Export_Error,
          "Triangle count exceeded 4294967295, so the STL file is not valid");
    }

    // 80-byte title (zero padded) + 4-byte little-endian triangle count
    char header[84] = {};
    static const char title[] = "OpenSCAD Model\n";
    std::memcpy(header, title, sizeof(title) - 1);
    const auto count = static_cast<uint32_t>(triangle_count);
    header[80] = static_cast<char>(count & 0xff);
    header[81] = static_cast<char>((count >> 8) & 0xff);
    header[82] = static_cast<char>((count >> 16) & 0xff);
    header[83] = static_cast<char>((count >> 24) & 0xff);
    output.write(header, sizeof(header));

    // Stream triangles directly to the output
    for (const auto& ps : polysets) {
      append_stl(ps, output, true);
    }
  } else {
    // ASCII mode: Write directly to the output stream
    setlocale(LC_NUMERIC, "C");  // Ensure radix is . (not ,) in output
    output << "solid OpenSCAD_Model\n";
    for (const auto& ps : polysets) {
      append_stl(ps, output, false);
    }
    output << "endsolid OpenSCAD_Model\n";
    setlocale(LC_NUMERIC, "");  // Restore default locale
  }
}
