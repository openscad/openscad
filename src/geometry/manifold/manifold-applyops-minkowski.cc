// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#ifdef ENABLE_MANIFOLD

#include "cgal.h"
#include "cgalutils.h"
#include <CGAL/convex_hull_3.h>

#include "PolySet.h"
#include "printutils.h"
#include "manifoldutils.h"
#include "ManifoldGeometry.h"
#include "parallel.h"

namespace ManifoldUtils {

/*!
   children cannot contain nullptr objects
 */
std::shared_ptr<const Geometry> applyMinkowskiManifold(const Geometry::Geometries& children)
{
  using Hull_Points = std::vector<glm::vec3>;
  using Nef_kernel = CGAL_Kernel3;
  using Polyhedron = CGAL_Polyhedron;
  using Nef = CGAL_Nef_polyhedron3;

  auto polyhedronFromGeometry = [](const std::shared_ptr<const Geometry>& geom, bool *pIsConvexOut) -> std::shared_ptr<Polyhedron> 
  {
    auto ps = dynamic_cast<const PolySet *>(geom.get());
    if (ps) {
      auto poly = std::make_shared<Polyhedron>();
      CGALUtils::createPolyhedronFromPolySet(*ps, *poly);
      if (pIsConvexOut) *pIsConvexOut = ps->isConvex();
      return poly;
    } else {
      if (auto mani = dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
        auto poly = mani->toPolyhedron<Polyhedron>();
        if (pIsConvexOut) *pIsConvexOut = CGALUtils::is_weakly_convex(*poly);
        return poly;
      } else throw 0;
    }
    throw 0;
  };
  
  CGAL::Timer t, t_tot;
  assert(children.size() >= 2);
  auto it = children.begin();
  t_tot.start();
  std::vector<std::shared_ptr<const Geometry>> operands = {it->second, std::shared_ptr<const Geometry>()};

  auto getHullPoints = [&](const Polyhedron &poly) {
    std::vector<glm::vec3> out;
    out.reserve(poly.size_of_vertices());
    for (auto pi = poly.vertices_begin(); pi != poly.vertices_end(); ++pi) {
      out.emplace_back(CGALUtils::vector_convert<glm::vec3>(pi->point()));
    }
    return out;
  };

  try {
    // Note: we could parallelize more, e.g. compute all decompositions ahead of time instead of doing them 2 by 2,
    // but this could use substantially more memory.
    while (++it != children.end()) {
      operands[1] = it->second;

      std::vector<std::list<Hull_Points>> part_points(2);

      parallelizable_transform(operands.begin(), operands.begin() + 2, part_points.begin(), [&](const auto &operand) {
        std::list<Hull_Points> part_points;

        bool is_convex;
        auto poly = polyhedronFromGeometry(operand, &is_convex);
        if (!poly) throw 0;
        if (poly->empty()) {
          throw 0;
        }

        if (is_convex) {
          part_points.emplace_back(getHullPoints(*poly));
        } else {
          Nef decomposed_nef(*poly);

          t.start();
          CGAL::convex_decomposition_3(decomposed_nef);

          // the first volume is the outer volume, which ignored in the decomposition
          Nef::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
          for (; ci != decomposed_nef.volumes_end(); ++ci) {
            if (ci->mark()) {
              Polyhedron poly;
              decomposed_nef.convert_inner_shell_to_polyhedron(ci->shells_begin(), poly);
              part_points.emplace_back(getHullPoints(poly));
            }
          }

          PRINTDB("Minkowski: decomposed into %d convex parts", part_points.size());
          t.stop();
          PRINTDB("Minkowski: decomposition took %f s", t.time());
        }
        return std::move(part_points);
      });
      
      auto combineParts = [&](const Hull_Points &points0, const Hull_Points &points1) -> std::shared_ptr<const ManifoldGeometry> {
        CGAL::Timer t;

        t.start();
        std::vector<glm::vec3> minkowski_points;

        auto np0 = points0.size();
        auto np1 = points1.size();
        minkowski_points.resize(np0 * np1);
        for (size_t i = 0; i < points0.size(); ++i) {
          auto &p0i = points0[i];
          auto offset = np1 * i;
          for (size_t j = 0; j < points1.size(); ++j) {
            minkowski_points[offset + j] = p0i + points1[j];
          }
        }

        if (minkowski_points.size() <= 3) {
          t.stop();
          return std::make_shared<const ManifoldGeometry>();
        }

        t.stop();
        PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms", points0.size() % points1.size() % minkowski_points.size() % (t.time() * 1000));
        t.reset();

        t.start();

        auto hull = manifold::Manifold::Hull(minkowski_points);
        t.stop();
        PRINTDB("Minkowski: Computing convex hull took %f s", t.time());
        t.reset();

        return std::make_shared<ManifoldGeometry>(std::make_shared<manifold::Manifold>(std::move(hull)));
      };

      std::vector<std::shared_ptr<const ManifoldGeometry>> result_parts(part_points[0].size() * part_points[1].size());
      parallelizable_cross_product_transform(
          part_points[0], part_points[1],
          result_parts.begin(),
          combineParts);

      if (it != std::next(children.begin())) operands[0].reset();

      t.start();
      PRINTDB("Minkowski: Computing union of %d parts", result_parts.size());
      Geometry::Geometries fake_children;
      for (const auto& part : result_parts) {
        fake_children.push_back(std::make_pair(std::shared_ptr<const AbstractNode>(),
                                                part));
      }
      auto N = ManifoldUtils::applyOperator3DManifold(fake_children, OpenSCADOperator::UNION);
        
      // FIXME: This should really never throw.
      // Assert once we figured out what went wrong with issue #1069?
      if (!N) throw 0;
      t.stop();
      PRINTDB("Minkowski: Union done: %f s", t.time());
      t.reset();

      operands[0] = N;
    }

    t_tot.stop();
    PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
    t_tot.reset();
    return operands[0];
  } catch (const std::exception& e) {
    LOG(message_group::Warning,
        "[manifold] Minkowski failed with error, falling back to Nef operation: %1$s\n", e.what());

    return ManifoldUtils::applyOperator3DManifold(children, OpenSCADOperator::MINKOWSKI);
  } catch (...) {
    LOG(message_group::Warning,
        "[manifold] Minkowski hard-crashed, falling back to Nef operation.");

    return ManifoldUtils::applyOperator3DManifold(children, OpenSCADOperator::MINKOWSKI);
  }
}

}  // namespace ManifoldUtils

#endif // ENABLE_MANIFOLD
