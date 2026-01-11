#include "geometry/GeometryEvaluator.h"

#include "Feature.h"
#include "geometry/boolean_utils.h"
#include "geometry/cgal/cgal.h"
#include "geometry/ClipperUtils.h"
#include "geometry/linalg.h"
#include "geometry/linear_extrude.h"
#include "geometry/Geometry.h"
#include "geometry/GeometryCache.h"
#include "geometry/Polygon2d.h"
#include "geometry/PolySetUtils.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/roof_ss.h"
#include "geometry/roof_vd.h"
#include "geometry/rotate_extrude.h"

#include "glview/RenderSettings.h"

#include "core/CgalAdvNode.h"
#include "core/ColorNode.h"
#include "core/CsgOpNode.h"
#include "core/ModuleInstantiation.h"
#include "core/LinearExtrudeNode.h"
#include "core/OffsetNode.h"
#include "core/ProjectionNode.h"
#include "core/RenderNode.h"
#include "core/RoofNode.h"
#include "core/RotateExtrudeNode.h"
#include "core/State.h"
#include "core/TextNode.h"
#include "core/TransformNode.h"
#include "core/CurveDiscretizer.h"
#include "core/Tree.h"
#include "utils/calc.h"
#include "utils/degree_trig.h"
#include "utils/printutils.h"

#include <iterator>
#include <cassert>
#include <list>
#include <utility>
#include <memory>
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALCache.h"
#include "geometry/cgal/cgalutils.h"
#include <CGAL/convex_hull_2.h>
#include <CGAL/Point_2.h>
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/manifoldutils.h"
#endif

#include <vector>

class Geometry;
class Polygon2d;
class Tree;

GeometryEvaluator::GeometryEvaluator(const Tree& tree) : tree(tree) {}

/*!
   Set allownef to false to force the result to _not_ be a Nef polyhedron

   There are some guarantees on the returned geometry:
   * 2D and 3D geometry cannot be mixed; we will return either _only_ 2D or _only_ 3D geometries
   * PolySet geometries are always 3D. 2D Polysets are only created for special-purpose rendering
   operations downstream from here.
   * Needs validation: Implementation-specific geometries shouldn't be mixed (Nef polyhedron, Manifold)
 */
std::shared_ptr<const Geometry> GeometryEvaluator::evaluateGeometry(const AbstractNode& node,
                                                                    bool allownef)
{
  auto result = smartCacheGet(node, allownef);
  if (!result) {
    // If not found in any caches, we need to evaluate the geometry
    // traverse() will set this->root to a geometry, which can be any geometry
    // (including GeometryList if the lazyunions feature is enabled)
    this->traverse(node);
    result = this->root;

    // Insert the raw result into the cache.
    smartCacheInsert(node, result);
  }

  // Convert engine-specific 3D geometry to PolySet if needed
  // Note: we don't store the converted into the cache as it would conflict with subsequent calls where
  // allownef is true.
  if (!allownef) {
    if (auto ps = PolySetUtils::getGeometryAsPolySet(result)) {
      assert(ps->getDimension() == 3);
      // We cannot render concave polygons, so tessellate any PolySets
      if (!ps->isEmpty() && !ps->isTriangular()) {
        // Since is_convex() doesn't handle non-planar faces, we need to tessellate
        // also in the indeterminate state so we cannot just use a boolean comparison. See #1061
        bool convex = bool(
          ps->convexValue());  // bool is true only if tribool is true, (not indeterminate and not false)
        if (!convex) {
          ps = PolySetUtils::tessellate_faces(*ps);
        }
      }
      return ps;
    }
  }
  return result;
}

bool GeometryEvaluator::isValidDim(const Geometry::GeometryItem& item, unsigned int& dim) const
{
  if (!item.first->modinst->isBackground() && item.second) {
    if (!dim) dim = item.second->getDimension();
    else if (dim != item.second->getDimension() && !item.second->isEmpty()) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(),
          "Mixing 2D and 3D objects is not supported");
      return false;
    }
  }
  return true;
}

GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren(const AbstractNode& node,
                                                                   OpenSCADOperator op)
{
  unsigned int dim = 0;
  for (const auto& item : this->visitedchildren[node.index()]) {
    if (!isValidDim(item, dim)) break;
  }
  if (dim == 2)
    return ResultObject::mutableResult(std::shared_ptr<Geometry>(applyToChildren2D(node, op)));
  else if (dim == 3) return applyToChildren3D(node, op);
  return {};
}

/*!
   Applies the operator to all child nodes of the given node.

   May return nullptr or any 3D Geometry object
 */
GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren3D(const AbstractNode& node,
                                                                     OpenSCADOperator op)
{
  Geometry::Geometries children = collectChildren3D(node);
  if (children.empty()) return {};

  if (op == OpenSCADOperator::HULL) {
    return ResultObject::mutableResult(std::shared_ptr<Geometry>(applyHull(children)));
  } else if (op == OpenSCADOperator::FILL) {
    for (const auto& item : children) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(),
          "fill() not yet implemented for 3D");
    }
  }

  // Only one child -> this is a noop
  if (children.size() == 1) return ResultObject::constResult(children.front().second);

  switch (op) {
  case OpenSCADOperator::MINKOWSKI: {
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
    if (actualchildren.size() == 1) return ResultObject::constResult(actualchildren.front().second);
    return ResultObject::constResult(applyMinkowski(actualchildren));
    break;
  }
  case OpenSCADOperator::UNION: {
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
    if (actualchildren.size() == 1) return ResultObject::constResult(actualchildren.front().second);
#ifdef ENABLE_MANIFOLD
    if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
      return ResultObject::mutableResult(ManifoldUtils::applyOperator3DManifold(actualchildren, op));
    }
#endif
#ifdef ENABLE_CGAL
    return ResultObject::constResult(std::shared_ptr<const Geometry>(
      CGALUtils::applyUnion3D(actualchildren.begin(), actualchildren.end())));
#else
    assert(false && "No boolean backend available");
#endif
    break;
  }
  default: {
#ifdef ENABLE_MANIFOLD
    if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
      return ResultObject::mutableResult(ManifoldUtils::applyOperator3DManifold(children, op));
    }
#endif
#ifdef ENABLE_CGAL
    return ResultObject::constResult(CGALUtils::applyOperator3D(children, op));
#else
    assert(false && "No boolean backend available");
#endif
    break;
  }
  }
}

/*!
   Apply 2D hull.

   May return an empty geometry but will not return nullptr.
 */

std::unique_ptr<Polygon2d> GeometryEvaluator::applyHull2D(const AbstractNode& node)
{
  auto children = collectChildren2D(node);
  auto geometry = std::make_unique<Polygon2d>();

#ifdef ENABLE_CGAL
  using CGALPoint2 = CGAL::Point_2<CGAL_DoubleKernel>;
  // Collect point cloud
  std::list<CGALPoint2> points;
  for (const auto& p : children) {
    if (p) {
      for (const auto& o : p->outlines()) {
        for (const auto& v : o.vertices) {
          points.emplace_back(v[0], v[1]);
        }
      }
    }
  }
  if (points.size() > 0) {
    // Apply hull
    std::list<CGALPoint2> result;
    try {
      CGAL::convex_hull_2(points.begin(), points.end(), std::back_inserter(result));
      // Construct Polygon2d
      Outline2d outline;
      for (const auto& p : result) {
        outline.vertices.emplace_back(p[0], p[1]);
      }
      geometry->addOutline(outline);
      geometry->setSanitized(true);
    } catch (const CGAL::Failure_exception& e) {
      LOG(message_group::Warning, "GeometryEvaluator::applyHull2D() during CGAL::convex_hull_2(): %1$s",
          e.what());
    }
  }
#endif
  return geometry;
}

std::unique_ptr<Polygon2d> GeometryEvaluator::applyFill2D(const AbstractNode& node)
{
  // Merge and sanitize input geometry
  auto geometry_in = ClipperUtils::apply(collectChildren2D(node), Clipper2Lib::ClipType::Union);
  assert(geometry_in->isSanitized());

  std::vector<std::shared_ptr<const Polygon2d>> newchildren;
  // Keep only the 'positive' outlines, eg: the outside edges
  for (const auto& outline : geometry_in->outlines()) {
    if (outline.positive) {
      newchildren.push_back(std::make_shared<Polygon2d>(outline));
    }
  }

  // Re-merge geometry in case of nested outlines
  return ClipperUtils::apply(newchildren, Clipper2Lib::ClipType::Union);
}

std::unique_ptr<Geometry> GeometryEvaluator::applyHull3D(const AbstractNode& node)
{
  Geometry::Geometries children = collectChildren3D(node);

  auto P = PolySet::createEmpty();
  return applyHull(children);
}

std::unique_ptr<Polygon2d> GeometryEvaluator::applyMinkowski2D(const AbstractNode& node)
{
  auto children = collectChildren2D(node);
  if (!children.empty()) {
    return ClipperUtils::applyMinkowski(children);
  }
  return nullptr;
}

/*!
   Returns a list of Polygon2d children of the given node.
   May return empty Polygon2d object, but not nullptr objects
 */
std::vector<std::shared_ptr<const Polygon2d>> GeometryEvaluator::collectChildren2D(
  const AbstractNode& node)
{
  std::vector<std::shared_ptr<const Polygon2d>> children;
  for (const auto& item : this->visitedchildren[node.index()]) {
    auto& chnode = item.first;
    auto& chgeom = item.second;
    if (chnode->modinst->isBackground()) continue;

    // NB! We insert into the cache here to ensure that all children of
    // a node is a valid object. If we inserted as we created them, the
    // cache could have been modified before we reach this point due to a large
    // sibling object.
    smartCacheInsert(*chnode, chgeom);

    if (chgeom) {
      if (chgeom->getDimension() == 3) {
        LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(),
            "Ignoring 3D child object for 2D operation");
        children.push_back(nullptr);  // replace 3D geometry with empty geometry
      } else {
        if (chgeom->isEmpty()) {
          children.push_back(nullptr);
        } else {
          const auto polygon2d = std::dynamic_pointer_cast<const Polygon2d>(chgeom);
          assert(polygon2d);
          children.push_back(polygon2d);
        }
      }
    } else {
      children.push_back(nullptr);
    }
  }
  return children;
}

/*!
   Since we can generate both Nef and non-Nef geometry, we need to insert it into
   the appropriate cache.
   This method inserts the geometry into the appropriate cache if it's not already cached.
 */
void GeometryEvaluator::smartCacheInsert(const AbstractNode& node,
                                         const std::shared_ptr<const Geometry>& geom)
{
#ifdef ENABLE_MANIFOLD
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
    bool skipCache = false;
    if (const auto opNode = dynamic_cast<const CsgOpNode *>(&node)) {
      // Skip caching UNION to enable BatchUnion optimizations.
      // We still cache DIFFERENCE and INTERSECTION as they are destructive/expensive
      // and valid candidates for reuse boundaries.
      if (opNode->type == OpenSCADOperator::UNION) {
        skipCache = true;
      }
    } else if (dynamic_cast<const TransformNode *>(&node) || dynamic_cast<const ColorNode *>(&node)) {
      skipCache = true;
    } else if (dynamic_cast<const GroupNode *>(&node)) {
      // Don't skip the RootNode, as we want to cache the final result
      if (!dynamic_cast<const RootNode *>(&node)) {
        skipCache = true;
      }
    }

    if (skipCache) return;
  }
#endif

  const std::string& key = this->tree.getIdString(node);

  if (CGALCache::acceptsGeometry(geom)) {
    if (!CGALCache::instance()->contains(key)) {
      CGALCache::instance()->insert(key, geom);
    }
  } else if (!GeometryCache::instance()->contains(key)) {
    // FIXME: Sanity-check Polygon2d as well?
    // if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    //   assert(!ps->hasDegeneratePolygons());
    // }

    // Perhaps add acceptsGeometry() to GeometryCache as well?
    if (!GeometryCache::instance()->insert(key, geom)) {
      LOG(message_group::Warning, "GeometryEvaluator: Node didn't fit into cache.");
    }
  }
}

bool GeometryEvaluator::isSmartCached(const AbstractNode& node)
{
  const std::string& key = this->tree.getIdString(node);
  return GeometryCache::instance()->contains(key) || CGALCache::instance()->contains(key);
}

std::shared_ptr<const Geometry> GeometryEvaluator::smartCacheGet(const AbstractNode& node,
                                                                 bool preferNef)
{
  const std::string& key = this->tree.getIdString(node);
  const bool hasgeom = GeometryCache::instance()->contains(key);
  const bool hascgal = CGALCache::instance()->contains(key);
  if (hascgal && (preferNef || !hasgeom)) return CGALCache::instance()->get(key);
  if (hasgeom) return GeometryCache::instance()->get(key);
  return {};
}

/*!
   Returns a list of 3D Geometry children of the given node.
   May return empty geometries, but not nullptr objects
 */
Geometry::Geometries GeometryEvaluator::collectChildren3D(const AbstractNode& node)
{
  Geometry::Geometries children;
  for (const auto& item : this->visitedchildren[node.index()]) {
    auto& chnode = item.first;
    const std::shared_ptr<const Geometry>& chgeom = item.second;
    if (chnode->modinst->isBackground()) continue;

    // NB! We insert into the cache here to ensure that all children of
    // a node is a valid object. If we inserted as we created them, the
    // cache could have been modified before we reach this point due to a large
    // sibling object.
    smartCacheInsert(*chnode, chgeom);

    if (chgeom && chgeom->getDimension() == 2) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(),
          "Ignoring 2D child object for 3D operation");
      children.push_back(
        std::make_pair(item.first, nullptr));  // replace 2D geometry with empty geometry
    } else {
      // Add children if geometry is 3D OR null/empty
      children.push_back(item);
    }
  }
  return children;
}

/*!

 */
std::unique_ptr<Polygon2d> GeometryEvaluator::applyToChildren2D(const AbstractNode& node,
                                                                OpenSCADOperator op)
{
  node.progress_report();
  if (op == OpenSCADOperator::MINKOWSKI) {
    return applyMinkowski2D(node);
  } else if (op == OpenSCADOperator::HULL) {
    return applyHull2D(node);
  } else if (op == OpenSCADOperator::FILL) {
    return applyFill2D(node);
  }

  auto children = collectChildren2D(node);

  if (children.empty()) {
    return nullptr;
  }

  if (children.size() == 1) {
    if (children[0]) {
      return std::make_unique<Polygon2d>(*children[0]);  // Copy
    } else {
      return nullptr;
    }
  }

  Clipper2Lib::ClipType clipType;
  switch (op) {
  case OpenSCADOperator::UNION:        clipType = Clipper2Lib::ClipType::Union; break;
  case OpenSCADOperator::INTERSECTION: clipType = Clipper2Lib::ClipType::Intersection; break;
  case OpenSCADOperator::DIFFERENCE:   clipType = Clipper2Lib::ClipType::Difference; break;
  default:
    LOG(message_group::Error, "Unknown boolean operation %1$d", int(op));
    return nullptr;
    break;
  }

  return ClipperUtils::apply(children, clipType);
}

/*!
   Adds ourself to our parent's list of traversed children.
   Call this for _every_ node which affects output during traversal.
   Usually, this should be called from the postfix stage, but for some nodes,
   we defer traversal letting other components (e.g. CGAL) render the subgraph,
   and we'll then call this from prefix and prune further traversal.

   The added geometry can be nullptr if it wasn't possible to evaluate it.
 */
void GeometryEvaluator::addToParent(const State& state, const AbstractNode& node,
                                    const std::shared_ptr<const Geometry>& geom)
{
  this->visitedchildren.erase(node.index());
  if (state.parent()) {
    this->visitedchildren[state.parent()->index()].push_back(
      std::make_pair(node.shared_from_this(), geom));
  } else {
    // Root node
    this->root = geom;
    assert(this->visitedchildren.empty());
  }
}

Response GeometryEvaluator::visit(State& state, const ColorNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      // First union all children
      ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
      if ((geom = res.constptr())) {
        auto mutableGeom = res.asMutableGeometry();
        if (mutableGeom) mutableGeom->setColor(node.color);
        geom = mutableGeom;
      }
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   Custom nodes are handled here => implicit union
 */
Response GeometryEvaluator::visit(State& state, const AbstractNode& node)
{
  if (state.isPrefix()) {
    if (isSmartCached(node)) return Response::PruneTraversal;
    state.setPreferNef(true);  // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      geom = applyToChildren(node, OpenSCADOperator::UNION).constptr();
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   Pass children to parent without touching them. Used by e.g. for loops
 */
Response GeometryEvaluator::visit(State& state, const ListNode& node)
{
  if (state.parent()) {
    if (state.isPrefix() && node.modinst->isBackground()) {
      if (node.modinst->isBackground()) state.setBackground(true);
      return Response::PruneTraversal;
    }
    if (state.isPostfix()) {
      unsigned int dim = 0;
      for (const auto& item : this->visitedchildren[node.index()]) {
        if (!isValidDim(item, dim)) break;
        auto& chnode = item.first;
        const std::shared_ptr<const Geometry>& chgeom = item.second;
        addToParent(state, *chnode, chgeom);
      }
      this->visitedchildren.erase(node.index());
    }
    return Response::ContinueTraversal;
  } else {
    // Handle when a ListNode is given root modifier
    return lazyEvaluateRootNode(state, node);
  }
}

/*!
 */
Response GeometryEvaluator::visit(State& state, const GroupNode& node)
{
  return visit(state, (const AbstractNode&)node);
}

Response GeometryEvaluator::lazyEvaluateRootNode(State& state, const AbstractNode& node)
{
  if (state.isPrefix()) {
    if (node.modinst->isBackground()) {
      state.setBackground(true);
      return Response::PruneTraversal;
    }
    if (isSmartCached(node)) {
      return Response::PruneTraversal;
    }
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;

    unsigned int dim = 0;
    GeometryList::Geometries geometries;
    for (const auto& item : this->visitedchildren[node.index()]) {
      if (!isValidDim(item, dim)) break;
      auto& chnode = item.first;
      const std::shared_ptr<const Geometry>& chgeom = item.second;
      if (chnode->modinst->isBackground()) continue;
      // NB! We insert into the cache here to ensure that all children of
      // a node is a valid object. If we inserted as we created them, the
      // cache could have been modified before we reach this point due to a large
      // sibling object.
      smartCacheInsert(*chnode, chgeom);
      // Only use valid geometries
      if (chgeom && !chgeom->isEmpty()) geometries.push_back(item);
    }
    if (geometries.size() == 1) geom = geometries.front().second;
    else if (geometries.size() > 1) geom = std::make_shared<GeometryList>(geometries);

    this->root = geom;
  }
  return Response::ContinueTraversal;
}

/*!
   Root nodes are handled specially; they will flatten any child group
   nodes to avoid doing an implicit top-level union.

   NB! This is likely a temporary measure until a better implementation of
   group nodes is in place.
 */
Response GeometryEvaluator::visit(State& state, const RootNode& node)
{
  // If we didn't enable lazy unions, just union the top-level objects
  if (!Feature::ExperimentalLazyUnion.is_enabled()) {
    return visit(state, (const GroupNode&)node);
  }
  return lazyEvaluateRootNode(state, node);
}

Response GeometryEvaluator::visit(State& state, const OffsetNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      if (const auto polygon = applyToChildren2D(node, OpenSCADOperator::UNION)) {
        // ClipperLib documentation: The formula for the number of steps in a full
        // circular arc is ... Pi / acos(1 - arc_tolerance / abs(delta))
        double n = node.discretizer.getCircularSegmentCount(std::abs(node.delta)).value_or(3);
        double arc_tolerance = std::abs(node.delta) * (1 - cos_degrees(180 / n));
        geom = ClipperUtils::applyOffset(*polygon, node.delta, node.join_type, node.miter_limit,
                                         arc_tolerance);
        assert(geom);
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   RenderNodes just pass on convexity
 */
Response GeometryEvaluator::visit(State& state, const RenderNode& node)
{
  if (state.isPrefix()) {
    if (isSmartCached(node)) return Response::PruneTraversal;
    state.setPreferNef(true);  // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
      auto mutableGeom = res.asMutableGeometry();
      if (mutableGeom) mutableGeom->setConvexity(node.convexity);
      geom = mutableGeom;
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    node.progress_report();
    addToParent(state, node, geom);
  }
  return Response::ContinueTraversal;
}

/*!
   Leaf nodes can create their own geometry, so let them do that

   input: None
   output: PolySet or Polygon2d
 */
Response GeometryEvaluator::visit(State& state, const LeafNode& node)
{
  if (state.isPrefix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      geom = node.createGeometry();
      assert(geom);
      if (const auto polygon = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
        if (!polygon->isSanitized()) {
          geom = ClipperUtils::sanitize(*polygon);
        }
      } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
        //        assert(!ps->hasDegeneratePolygons());
      }
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::PruneTraversal;
}

Response GeometryEvaluator::visit(State& state, const TextNode& node)
{
  if (state.isPrefix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      auto polygonlist = node.createPolygonList();
      geom = ClipperUtils::apply(polygonlist, Clipper2Lib::ClipType::Union);
    } else {
      geom = GeometryCache::instance()->get(this->tree.getIdString(node));
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::PruneTraversal;
}

/*!
   input: List of 2D or 3D objects (not mixed)
   output: Polygon2d or 3D PolySet
   operation:
    o Perform csg op on children
 */
Response GeometryEvaluator::visit(State& state, const CsgOpNode& node)
{
  if (state.isPrefix()) {
    if (isSmartCached(node)) return Response::PruneTraversal;
    state.setPreferNef(true);  // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      geom = applyToChildren(node, node.type).constptr();
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   input: List of 2D or 3D objects (not mixed)
   output: Polygon2d or 3D PolySet
   operation:
    o Union all children
    o Perform transform
 */
Response GeometryEvaluator::visit(State& state, const TransformNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      if (matrix_contains_infinity(node.matrix) || matrix_contains_nan(node.matrix)) {
        // due to the way parse/eval works we can't currently distinguish between NaN and Inf
        LOG(message_group::Warning, node.modinst->location(), this->tree.getDocumentPath(),
            "Transformation matrix contains Not-a-Number and/or Infinity - removing object.");
      } else {
        // First union all children
        ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
        if ((geom = res.constptr())) {
          if (geom->getDimension() == 2) {
            auto polygons = std::dynamic_pointer_cast<Polygon2d>(res.asMutableGeometry());
            assert(polygons);

            Transform2d mat2;
            mat2.matrix() << node.matrix(0, 0), node.matrix(0, 1), node.matrix(0, 3), node.matrix(1, 0),
              node.matrix(1, 1), node.matrix(1, 3), node.matrix(3, 0), node.matrix(3, 1),
              node.matrix(3, 3);
            polygons->transform(mat2);
            // FIXME: We lose the transform if we copied a const geometry above. Probably similar issue
            // in multiple places A 2D transformation may flip the winding order of a polygon. If that
            // happens with a sanitized polygon, we need to reverse the winding order for it to be
            // correct.
            if (polygons->isSanitized() && mat2.matrix().determinant() <= 0) {
              geom = ClipperUtils::sanitize(*polygons);
            }
          } else if (geom->getDimension() == 3) {
            auto mutableGeom = res.asMutableGeometry();
            if (mutableGeom) mutableGeom->transform(node.matrix);
            geom = mutableGeom;
          }
        }
      }
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   input: List of 2D objects
   output: 3D PolySet
   operation:
    o Union all children
    o Perform extrude
 */
Response GeometryEvaluator::visit(State& state, const LinearExtrudeNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      const std::shared_ptr<const Geometry> geometry = applyToChildren2D(node, OpenSCADOperator::UNION);
      if (geometry) {
        const auto polygons = std::dynamic_pointer_cast<const Polygon2d>(geometry);
        geom = extrudePolygon(node, *polygons);
        assert(geom);
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   input: List of 2D objects
   output: 3D PolySet
   operation:
    o Union all children
    o Perform extrude
 */
Response GeometryEvaluator::visit(State& state, const RotateExtrudeNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      const std::shared_ptr<const Polygon2d> geometry = applyToChildren2D(node, OpenSCADOperator::UNION);
      if (geometry) {
        geom = rotatePolygon(node, *geometry);
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   FIXME: Not in use
 */
Response GeometryEvaluator::visit(State& /*state*/, const AbstractPolyNode& /*node*/)
{
  assert(false);
  return Response::AbortTraversal;
}

std::shared_ptr<const Geometry> GeometryEvaluator::projectionCut(const ProjectionNode& node)
{
  std::shared_ptr<const Geometry> geom;
  std::shared_ptr<const Geometry> newgeom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (newgeom) {
#ifdef ENABLE_MANIFOLD
    if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
      auto manifold = ManifoldUtils::createManifoldFromGeometry(newgeom);
      if (manifold != nullptr) {
        auto poly2d = manifold->slice();
        return std::shared_ptr<const Polygon2d>(ClipperUtils::sanitize(poly2d));
      }
    }
#endif
#ifdef ENABLE_CGAL
    auto Nptr = CGALUtils::getNefPolyhedronFromGeometry(newgeom);
    if (Nptr && !Nptr->isEmpty()) {
      auto poly = CGALUtils::project(*Nptr, node.cut_mode);
      if (poly) {
        poly->setConvexity(node.convexity);
        geom = std::move(poly);
      }
    }
#endif
  }
  return geom;
}

std::shared_ptr<const Geometry> GeometryEvaluator::projectionNoCut(const ProjectionNode& node)
{
#ifdef ENABLE_MANIFOLD
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
    const std::shared_ptr<const Geometry> newgeom =
      applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
    if (newgeom) {
      auto manifold = ManifoldUtils::createManifoldFromGeometry(newgeom);
      if (manifold != nullptr) {
        auto poly2d = manifold->project();
        return std::shared_ptr<const Polygon2d>(ClipperUtils::sanitize(poly2d));
      }
    } else {
      return std::make_shared<Polygon2d>();
    }
  }
#endif

  std::vector<std::shared_ptr<const Polygon2d>> tmp_geom;
  for (const auto& [chnode, chgeom] : this->visitedchildren[node.index()]) {
    if (chnode->modinst->isBackground()) continue;

    // Clipper version of Geometry projection
    // Clipper doesn't handle meshes very well.
    // It's better in V6 but not quite there. FIXME: stand-alone example.
    // project chgeom -> polygon2d
    if (auto chPS = PolySetUtils::getGeometryAsPolySet(chgeom)) {
      if (auto poly = PolySetUtils::project(*chPS)) {
        tmp_geom.push_back(std::shared_ptr(std::move(poly)));
      }
    }
  }
  auto projected = ClipperUtils::applyProjection(tmp_geom);
  return std::shared_ptr(std::move(projected));
}

/*!
   input: List of 3D objects
   output: Polygon2d
   operation:
    o Union all children
    o Perform projection
 */
Response GeometryEvaluator::visit(State& state, const ProjectionNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (isSmartCached(node)) {
      geom = smartCacheGet(node, false);
    } else {
      if (node.cut_mode) {
        geom = projectionCut(node);
      } else {
        geom = projectionNoCut(node);
      }
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   input: List of 2D or 3D objects (not mixed)
   output: any Geometry
   operation:
    o Perform cgal operation
 */
Response GeometryEvaluator::visit(State& state, const CgalAdvNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      switch (node.type) {
      case CgalAdvType::MINKOWSKI: {
        ResultObject res = applyToChildren(node, OpenSCADOperator::MINKOWSKI);
        geom = res.constptr();
        // If we added convexity, we need to pass it on
        if (geom && geom->getConvexity() != node.convexity) {
          auto editablegeom = res.asMutableGeometry();
          editablegeom->setConvexity(node.convexity);
          geom = editablegeom;
        }
        break;
      }
      case CgalAdvType::HULL: {
        geom = applyToChildren(node, OpenSCADOperator::HULL).constptr();
        break;
      }
      case CgalAdvType::FILL: {
        geom = applyToChildren(node, OpenSCADOperator::FILL).constptr();
        break;
      }
      case CgalAdvType::RESIZE: {
        ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
        auto editablegeom = res.asMutableGeometry();
        geom = editablegeom;
        if (editablegeom) {
          editablegeom->setConvexity(node.convexity);
          editablegeom->resize(node.newsize, node.autosize);
        }
        break;
      }
      default: assert(false && "not implemented");
      }
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

Response GeometryEvaluator::visit(State& state, const AbstractIntersectionNode& node)
{
  if (state.isPrefix()) {
    if (isSmartCached(node)) return Response::PruneTraversal;
    state.setPreferNef(true);  // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      geom = applyToChildren(node, OpenSCADOperator::INTERSECTION).constptr();
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
// FIXME: What is the convex/manifold situation of the resulting PolySet?
static std::unique_ptr<Geometry> roofOverPolygon(const RoofNode& node, const Polygon2d& poly)
{
  std::unique_ptr<PolySet> roof;
  if (node.method == "voronoi") {
    roof = roof_vd::voronoi_diagram_roof(poly, node.discretizer);
    roof->setConvexity(node.convexity);
  } else if (node.method == "straight") {
    roof = roof_ss::straight_skeleton_roof(poly);
    roof->setConvexity(node.convexity);
  } else {
    assert(false && "Invalid roof method");
  }

  return roof;
}

Response GeometryEvaluator::visit(State& state, const RoofNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      const auto polygon2d = applyToChildren2D(node, OpenSCADOperator::UNION);
      if (polygon2d) {
        std::unique_ptr<Geometry> roof;
        try {
          roof = roofOverPolygon(node, *polygon2d);
        } catch (RoofNode::roof_exception& e) {
          LOG(message_group::Error, node.modinst->location(), this->tree.getDocumentPath(),
              "Skeleton computation error. " + e.message());
          roof = PolySet::createEmpty();
        }
        assert(roof);
        geom = std::move(roof);
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
  }
  return Response::ContinueTraversal;
}
#endif
