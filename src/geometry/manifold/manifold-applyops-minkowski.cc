// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#ifdef ENABLE_MANIFOLD

#include <iterator>
#include <cassert>
#include <list>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

#include <CGAL/convex_hull_3.h>
#include <CGAL/Surface_mesh/Surface_mesh.h>

#include "geometry/cgal/cgal.h"
#include "geometry/Geometry.h"
#include "geometry/cgal/cgalutils.h"
#include "geometry/PolySet.h"
#include "utils/printutils.h"
#include "geometry/manifold/manifoldutils.h"
#include "geometry/manifold/ManifoldGeometry.h"
#include "utils/parallel.h"

namespace ManifoldUtils {

/*!
   children cannot contain nullptr objects
 */
std::shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children)
{
  assert(children.size() >= 2);

  using Hull_kernel = CGAL::Epick;
  using Hull_Mesh = CGAL::Surface_mesh<CGAL::Point_3<Hull_kernel>>;
  using Hull_Points = std::vector<CGAL::Point_3<Hull_kernel>>;

  auto surfaceMeshFromGeometry = [](const std::shared_ptr<const Geometry>& geom,
                                    bool *pIsConvexOut) -> std::shared_ptr<CGAL_Kernel3Mesh> {
    auto ps = std::dynamic_pointer_cast<const PolySet>(geom);
    if (ps) {
      auto mesh = CGALUtils::createSurfaceMeshFromPolySet<CGAL_Kernel3Mesh>(*ps);
      if (pIsConvexOut) *pIsConvexOut = ps->isConvex();
      return mesh;
    } else {
      if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
        auto mesh = ManifoldUtils::createSurfaceMeshFromManifold<CGAL_Kernel3Mesh>(mani->getManifold());
        if (pIsConvexOut) *pIsConvexOut = CGALUtils::is_weakly_convex(*mesh);
        return mesh;
      } else throw 0;
    }
    throw 0;
  };

  CGAL::Cartesian_converter<CGAL_Kernel3, Hull_kernel> conv;
  auto getHullPoints = [&](const CGAL_Polyhedron& poly) {
    std::vector<Hull_kernel::Point_3> out;
    out.reserve(poly.size_of_vertices());
    for (auto pi = poly.vertices_begin(); pi != poly.vertices_end(); ++pi) {
      out.push_back(conv(pi->point()));
    }
    return out;
  };
  auto getHullPointsFromMesh = [&](const CGAL_Kernel3Mesh& mesh) {
    std::vector<Hull_kernel::Point_3> out;
    out.reserve(mesh.number_of_vertices());
    for (auto idx : mesh.vertices()) {
      out.push_back(conv(mesh.point(idx)));
    }
    return out;
  };

  CGAL::Timer t_tot;
  t_tot.start();

  auto it = children.begin();
  std::vector<std::shared_ptr<const Geometry>> operands = {it->second,
                                                           std::shared_ptr<const Geometry>()};

  try {
    // Note: we could parallelize more, e.g. compute all decompositions ahead of time instead of doing
    // them 2 by 2, but this could use substantially more memory.
    while (++it != children.end()) {
      operands[1] = it->second;

      std::vector<std::list<Hull_Points>> part_points(2);

      parallelizable_transform(
        operands.begin(), operands.begin() + 2, part_points.begin(),
        [&](std::shared_ptr<const Geometry>& operand) {
          std::list<Hull_Points> part_points;

          bool is_convex;
          auto mesh = surfaceMeshFromGeometry(operand, &is_convex);
          if (!mesh) throw 0;
          if (mesh->is_empty()) {
            throw 0;
          }

          if (is_convex) {
            part_points.emplace_back(getHullPointsFromMesh(*mesh));
          } else {
            // The CGAL_Nef_polyhedron3 constructor can crash on bad polyhedron, so don't try
            if (!mesh->is_valid()) throw 0;
            CGAL::Timer convert_timer;
            convert_timer.start();
            CGAL_Nef_polyhedron3 decomposed_nef = CGALUtils::convertSurfaceMeshToNef(*mesh);
            if (!decomposed_nef.is_valid()) {
              LOG(message_group::Warning, "Minkowski: Nef polyhedron converted from mesh is invalid!");
              throw 0;
            }
            convert_timer.stop();
            PRINTDB("Minkowski: Nef conversion took %.2f s", convert_timer.time());

            CGAL::Timer t;
            t.start();
            CGAL::convex_decomposition_3(decomposed_nef);

            // the first volume is the outer volume, which ignored in the decomposition
            CGAL_Nef_polyhedron3::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
            for (; ci != decomposed_nef.volumes_end(); ++ci) {
              if (ci->mark()) {
                CGAL_Polyhedron poly;
                decomposed_nef.convert_inner_shell_to_polyhedron(ci->shells_begin(), poly);
                part_points.emplace_back(getHullPoints(poly));
              }
            }

            PRINTDB("Minkowski: decomposed into %d convex parts", part_points.size());
            t.stop();
            PRINTDB("Minkowski: decomposition took %f s", t.time());
          }
          return part_points;
        });

      std::vector<Hull_kernel::Point_3> minkowski_points;

      auto combineParts = [&](const Hull_Points& points0,
                              const Hull_Points& points1) -> std::shared_ptr<const ManifoldGeometry> {
        CGAL::Timer t;

        t.start();
        std::vector<Hull_kernel::Point_3> minkowski_points;

        minkowski_points.reserve(points0.size() * points1.size());
        for (const auto& p0 : points0) {
          for (const auto p1 : points1) {
            minkowski_points.push_back(p0 + (p1 - CGAL::ORIGIN));
          }
        }

        if (minkowski_points.size() <= 3) {
          t.stop();
          return std::make_shared<ManifoldGeometry>();
        }

        t.stop();
        PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms",
                points0.size() % points1.size() % minkowski_points.size() % (t.time() * 1000));
        t.reset();

        t.start();

        Hull_Mesh mesh;
        CGAL::convex_hull_3(minkowski_points.begin(), minkowski_points.end(), mesh);

        std::vector<Hull_kernel::Point_3> strict_points;
        strict_points.reserve(minkowski_points.size());

        for (auto v : mesh.vertices()) {
          auto& p = mesh.point(v);

          auto h = mesh.halfedge(v);
          auto e = h;
          bool collinear = false;
          bool coplanar = true;

          do {
            auto& q = mesh.point(mesh.target(mesh.opposite(h)));
            if (coplanar &&
                !CGAL::coplanar(p, q, mesh.point(mesh.target(mesh.next(h))),
                                mesh.point(mesh.target(mesh.next(mesh.opposite(mesh.next(h))))))) {
              coplanar = false;
            }

            for (auto j = mesh.opposite(mesh.next(h)); j != h && !collinear && !coplanar;
                 j = mesh.opposite(mesh.next(j))) {
              auto& r = mesh.point(mesh.target(mesh.opposite(j)));
              if (CGAL::collinear(p, q, r)) {
                collinear = true;
              }
            }

            h = mesh.opposite(mesh.next(h));
          } while (h != e && !collinear);

          if (!collinear && !coplanar) strict_points.push_back(p);
        }

        mesh.clear();
        CGAL::convex_hull_3(strict_points.begin(), strict_points.end(), mesh);

        t.stop();
        PRINTDB("Minkowski: Computing convex hull took %f s", t.time());
        t.reset();

        CGALUtils::triangulateFaces(mesh);
        return ManifoldUtils::createManifoldFromSurfaceMesh(mesh);
      };

      std::vector<std::shared_ptr<const ManifoldGeometry>> result_parts(part_points[0].size() *
                                                                        part_points[1].size());
      parallelizable_cross_product_transform(part_points[0], part_points[1], result_parts.begin(),
                                             combineParts);

      if (it != std::next(children.begin())) operands[0].reset();

      CGAL::Timer t;
      t.start();
      PRINTDB("Minkowski: Computing union of %d parts", result_parts.size());
      Geometry::Geometries fake_children;
      for (const auto& part : result_parts) {
        fake_children.push_back(std::make_pair(std::shared_ptr<const AbstractNode>(), part));
      }
      auto N = ManifoldUtils::applyOperator3DManifold(fake_children, OpenSCADOperator::UNION);

      // FIXME: This should really never throw.
      // Assert once we figured out what went wrong with issue #1069?
      if (!N) throw 0;
      t.stop();
      PRINTDB("Minkowski: Union done: %f s", t.time());
      t.reset();

      N->toOriginal();
      operands[0] = N;
    }

    t_tot.stop();
    PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
    t_tot.reset();
    return operands[0];
  } catch (const std::exception& e) {
    LOG(message_group::Warning,
        "[manifold] Minkowski failed with error, falling back to Nef operation: %1$s\n", e.what());
  } catch (...) {
    LOG(message_group::Warning, "[manifold] Minkowski hard-crashed, falling back to Nef operation.");
  }
  return ManifoldUtils::applyOperator3DManifold(children, OpenSCADOperator::MINKOWSKI);
}

}  // namespace ManifoldUtils

#endif  // ENABLE_MANIFOLD
