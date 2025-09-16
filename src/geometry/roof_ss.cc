// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#include "geometry/roof_ss.h"

#include <vector>
#include <clipper2/clipper.engine.h>
#include <iterator>
#include <functional>
#include <memory>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>
#include <CGAL/partition_2.h>
#include <CGAL/Partition_traits_2.h>
#if CGAL_VERSION_NR < CGAL_VERSION_NUMBER(6, 0, 0)
#include <boost/shared_ptr.hpp>
#endif

#include <algorithm>
#include <map>

#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
#include "geometry/GeometryUtils.h"
#include "geometry/ClipperUtils.h"
#include "core/RoofNode.h"
#include "geometry/PolySetBuilder.h"

#define RAISE_ROOF_EXCEPTION(message) \
  throw RoofNode::roof_exception(     \
    (boost::format("%s line %d: %s") % __FILE__ % __LINE__ % (message)).str());

namespace roof_ss {

using CGAL_KERNEL = CGAL::Exact_predicates_inexact_constructions_kernel;
using CGAL_Point_2 = CGAL_KERNEL::Point_2;
using CGAL_Polygon_2 = CGAL::Polygon_2<CGAL_KERNEL>;
using CGAL_Vector_2 = CGAL::Vector_2<CGAL_KERNEL>;
using CGAL_Line_2 = CGAL::Line_2<CGAL_KERNEL>;
using CGAL_Segment_2 = CGAL::Segment_2<CGAL_KERNEL>;
using CGAL_Polygon_with_holes_2 = CGAL::Polygon_with_holes_2<CGAL_KERNEL>;
using CGAL_Ss = CGAL::Straight_skeleton_2<CGAL_KERNEL>;

using CGAL_PT = CGAL::Partition_traits_2<CGAL_KERNEL>;

#if CGAL_VERSION_NR < CGAL_VERSION_NUMBER(6, 0, 0)
using CGAL_SsPtr = boost::shared_ptr<CGAL_Ss>;
#else
using CGAL_SsPtr = std::shared_ptr<CGAL_Ss>;
#endif

CGAL_Polygon_2 to_cgal_polygon_2(const Clipper2Lib::Path64& path, int scale_bits)
{
  CGAL_Polygon_2 poly;
  const double scale = std::ldexp(1.0, -scale_bits);
  for (auto v : path) {
    poly.push_back({v.x * scale, v.y * scale});
  }
  return poly;
}

// break a list of outlines into polygons with holes
std::vector<CGAL_Polygon_with_holes_2> polygons_with_holes(const Clipper2Lib::PolyTree64& polytree,
                                                           int scale_bits)
{
  std::vector<CGAL_Polygon_with_holes_2> ret;

  // lambda for recursive walk through polytree
  std::function<void(const Clipper2Lib::PolyPath64&)> walk = [&](const Clipper2Lib::PolyPath64& c) {
    // outer path
    CGAL_Polygon_with_holes_2 c_poly(to_cgal_polygon_2(c.Polygon(), scale_bits));
    // holes
    for (const auto& cc : c) {
      c_poly.add_hole(to_cgal_polygon_2(cc->Polygon(), scale_bits));
      for (const auto& ccc : *cc) walk(*ccc);
    }
    ret.push_back(c_poly);
    return;
  };

  for (const auto& root_node : polytree) walk(*root_node);

  return ret;
}

std::unique_ptr<PolySet> straight_skeleton_roof(const Polygon2d& poly)
{
  PolySetBuilder hatbuilder;

  const int scale_bits = ClipperUtils::scaleBitsFromPrecision();
  const Clipper2Lib::Paths64 paths = ClipperUtils::fromPolygon2d(poly, scale_bits);
  const std::unique_ptr<Clipper2Lib::PolyTree64> polytree = ClipperUtils::sanitize(paths);
  auto poly_sanitized = ClipperUtils::toPolygon2d(*polytree, scale_bits);

  try {
    // roof
    const std::vector<CGAL_Polygon_with_holes_2> shapes = polygons_with_holes(*polytree, scale_bits);
    for (const CGAL_Polygon_with_holes_2& shape : shapes) {
      const CGAL_SsPtr ss = CGAL::create_interior_straight_skeleton_2(shape);
      // store heights of vertices
      auto vector2d_comp = [](const Vector2d& a, const Vector2d& b) {
        return (a[0] < b[0]) || (a[0] == b[0] && a[1] < b[1]);
      };
      std::map<Vector2d, double, decltype(vector2d_comp)> heights(vector2d_comp);
      for (auto v = ss->vertices_begin(); v != ss->vertices_end(); v++) {
        const Vector2d p(v->point().x(), v->point().y());
        heights[p] = v->time();
      }

      for (auto ss_face = ss->faces_begin(); ss_face != ss->faces_end(); ss_face++) {
        // convert ss_face to cgal polygon
        CGAL_Polygon_2 face;
        for (auto h = ss_face->halfedge();;) {
          const CGAL_Point_2 pp = h->vertex()->point();
          face.push_back(pp);
          h = h->next();
          if (h == ss_face->halfedge()) {
            break;
          }
        }
        if (!face.is_simple()) {
          RAISE_ROOF_EXCEPTION(
            "A non-simple face in straight skeleton, likely cause is cgal issue #5177");
        }

        // do convex partition if necessary
        std::vector<CGAL_PT::Polygon_2> facets;
        CGAL::approx_convex_partition_2(face.vertices_begin(), face.vertices_end(),
                                        std::back_inserter(facets));

        for (const auto& facet : facets) {
          std::vector<int> roof;
          for (auto v = facet.vertices_begin(); v != facet.vertices_end(); v++) {
            const Vector2d vv(v->x(), v->y());
            roof.push_back(hatbuilder.vertexIndex(Vector3d(v->x(), v->y(), heights[vv])));
          }
          hatbuilder.appendPolygon(roof);
        }
      }
    }

    // floor
    {
      // poly has to go through clipper just as it does for the roof
      // because this may change coordinates
      auto tess = poly_sanitized->tessellate();
      for (const IndexedFace& triangle : tess->indices) {
        std::vector<int> floor;
        for (const int tv : triangle) {
          floor.push_back(hatbuilder.vertexIndex(tess->vertices[tv]));
        }
        // floor has wrong orientation
        std::reverse(floor.begin(), floor.end());
        hatbuilder.appendPolygon(floor);
      }
    }

    return hatbuilder.build();
  } catch (RoofNode::roof_exception& e) {
    throw;
  }
}

}  // namespace roof_ss
