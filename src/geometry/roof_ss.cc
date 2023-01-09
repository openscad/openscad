// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#include <boost/shared_ptr.hpp>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>
#include <CGAL/partition_2.h>
#include <CGAL/Partition_traits_2.h>

#include <algorithm>
#include <map>

#include "GeometryUtils.h"
#include "ClipperUtils.h"
#include "RoofNode.h"
#include "roof_ss.h"

#define RAISE_ROOF_EXCEPTION(message) \
        throw RoofNode::roof_exception((boost::format("%s line %d: %s") % __FILE__ % __LINE__ % (message)).str());

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

using CGAL_SsPtr = boost::shared_ptr<CGAL_Ss>;

using PolyTree = ClipperLib::PolyTree;
using PolyNode = ClipperLib::PolyNode;

CGAL_Polygon_2 to_cgal_polygon_2(const VectorOfVector2d& points)
{
  CGAL_Polygon_2 poly;
  for (auto v : points)
    poly.push_back({v[0], v[1]});
  return poly;
}

// break a list of outlines into polygons with holes
std::vector<CGAL_Polygon_with_holes_2> polygons_with_holes(const ClipperLib::PolyTree& polytree, int scale_pow2)
{
  std::vector<CGAL_Polygon_with_holes_2> ret;

  // lambda for recursive walk through polytree
  std::function<void (PolyNode *)> walk = [&](PolyNode *c) {
      // outer path
      CGAL_Polygon_with_holes_2 c_poly(to_cgal_polygon_2(ClipperUtils::fromPath(c->Contour, scale_pow2)));
      // holes
      for (auto cc : c->Childs) {
        c_poly.add_hole(to_cgal_polygon_2(ClipperUtils::fromPath(cc->Contour, scale_pow2)));
        for (auto ccc : cc->Childs)
          walk(ccc);
      }
      ret.push_back(c_poly);
      return;
    };

  for (auto root_node : polytree.Childs)
    walk(root_node);

  return ret;
}

PolySet *straight_skeleton_roof(const Polygon2d& poly)
{
  auto *hat = new PolySet(3);

  int scale_pow2 = ClipperUtils::getScalePow2(poly.getBoundingBox(), 32);
  ClipperLib::Paths paths = ClipperUtils::fromPolygon2d(poly, scale_pow2);
  ClipperLib::PolyTree polytree = ClipperUtils::sanitize(paths);
  Polygon2d *poly_sanitized = ClipperUtils::toPolygon2d(polytree, scale_pow2);

  try {
    // roof
    std::vector<CGAL_Polygon_with_holes_2> shapes = polygons_with_holes(polytree, scale_pow2);
    for (const CGAL_Polygon_with_holes_2& shape : shapes) {
      CGAL_SsPtr ss = CGAL::create_interior_straight_skeleton_2(shape);
      // store heights of vertices
      auto vector2d_comp = [](const Vector2d& a, const Vector2d& b) {
          return (a[0] < b[0]) || (a[0] == b[0] && a[1] < b[1]);
        };
      std::map<Vector2d, double, decltype(vector2d_comp)> heights(vector2d_comp);
      for (auto v = ss->vertices_begin(); v != ss->vertices_end(); v++) {
        Vector2d p(v->point().x(), v->point().y());
        heights[p] = v->time();
      }

      for (auto ss_face = ss->faces_begin(); ss_face != ss->faces_end(); ss_face++) {
        // convert ss_face to cgal polygon
        CGAL_Polygon_2 face;
        for (auto h = ss_face->halfedge(); ;) {
          CGAL_Point_2 pp = h->vertex()->point();
          face.push_back(pp);
          h = h->next();
          if (h == ss_face->halfedge()) {
            break;
          }
        }
        if (!face.is_simple()) {
          RAISE_ROOF_EXCEPTION("A non-simple face in straight skeleton, likely cause is cgal issue #5177");
        }

        // do convex partition if necessary
        std::vector<CGAL_PT::Polygon_2> facets;
        CGAL::approx_convex_partition_2(face.vertices_begin(), face.vertices_end(),
                                        std::back_inserter(facets));

        for (const auto& facet : facets) {
          Polygon roof;
          for (auto v = facet.vertices_begin(); v != facet.vertices_end(); v++) {
            Vector2d vv(v->x(), v->y());
            roof.push_back({v->x(), v->y(), heights[vv]});
          }
          hat->append_poly(roof);
        }
      }
    }

    // floor
    {
      // poly has to go through clipper just as it does for the roof
      // because this may change coordinates
      PolySet *tess = poly_sanitized->tessellate();
      for (const std::vector<Vector3d>& triangle : tess->polygons) {
        Polygon floor;
        for (const Vector3d& tv : triangle) {
          floor.push_back(tv);
        }
        // floor has wrong orientation
        std::reverse(floor.begin(), floor.end());
        hat->append_poly(floor);
      }
      delete tess;
    }

    delete poly_sanitized;

    return hat;
  } catch (RoofNode::roof_exception& e) {
    delete hat;
    throw;
  }
}

} // roof_ss
