#include "GeometryEvaluator.h"
#include "Tree.h"
#include "GeometryCache.h"
#include "CGALCache.h"
#include "Polygon2d.h"
#include "ModuleInstantiation.h"
#include "State.h"
#include "OffsetNode.h"
#include "TransformNode.h"
#include "LinearExtrudeNode.h"
#include "RoofNode.h"
#include "roof_ss.h"
#include "roof_vd.h"
#include "RotateExtrudeNode.h"
#include "CgalAdvNode.h"
#include "ProjectionNode.h"
#include "CsgOpNode.h"
#include "TextNode.h"
#include "CGALHybridPolyhedron.h"
#include "cgalutils.h"
#include "RenderNode.h"
#include "ClipperUtils.h"
#include "PolySetUtils.h"
#include "PolySet.h"
#include "calc.h"
#include "printutils.h"
#include "calc.h"
#include "DxfData.h"
#include "degree_trig.h"
#include <ciso646> // C alternative tokens (xor)
#include <algorithm>
#include "boost-utils.h"
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#include "manifoldutils.h"
#endif

#include <CGAL/convex_hull_2.h>
#include <CGAL/Point_2.h>

class Geometry;
class Polygon2d;
class Tree;

GeometryEvaluator::GeometryEvaluator(const Tree& tree) : tree(tree) { }

/*!
   Set allownef to false to force the result to _not_ be a Nef polyhedron
 */
shared_ptr<const Geometry> GeometryEvaluator::evaluateGeometry(const AbstractNode& node,
                                                               bool allownef)
{
  const std::string& key = this->tree.getIdString(node);
  if (!GeometryCache::instance()->contains(key)) {
    shared_ptr<const Geometry> N;
    if (CGALCache::instance()->contains(key)) {
      N = CGALCache::instance()->get(key);
    }

    // If not found in any caches, we need to evaluate the geometry
    if (N) {
      this->root = N;
    } else {
      this->traverse(node);
    }

    if (dynamic_pointer_cast<const CGALHybridPolyhedron>(this->root)) {
      this->root = CGALUtils::getGeometryAsPolySet(this->root);
    }
#ifdef ENABLE_MANIFOLD
    if (dynamic_pointer_cast<const ManifoldGeometry>(this->root)) {
      this->root = CGALUtils::getGeometryAsPolySet(this->root);
    }
#endif

    if (!allownef) {
      // We cannot render concave polygons, so tessellate any 3D PolySets
      auto ps = CGALUtils::getGeometryAsPolySet(this->root);
      if (ps && !ps->isEmpty()) {
        // Since is_convex() doesn't handle non-planar faces, we need to tessellate
        // also in the indeterminate state so we cannot just use a boolean comparison. See #1061
        bool convex = bool(ps->convexValue()); // bool is true only if tribool is true, (not indeterminate and not false)
        if (!convex) {
          assert(ps->getDimension() == 3);
          auto ps_tri = new PolySet(3, ps->convexValue());
          ps_tri->setConvexity(ps->getConvexity());
          PolySetUtils::tessellate_faces(*ps, *ps_tri);
          this->root.reset(ps_tri);
        }
      }
    }
    smartCacheInsert(node, this->root);
    return this->root;
  }
  return GeometryCache::instance()->get(key);
}

bool GeometryEvaluator::isValidDim(const Geometry::GeometryItem& item, unsigned int& dim) const {
  if (!item.first->modinst->isBackground() && item.second) {
    if (!dim) dim = item.second->getDimension();
    else if (dim != item.second->getDimension() && !item.second->isEmpty()) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(), "Mixing 2D and 3D objects is not supported");
      return false;
    }
  }
  return true;
}

GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren(const AbstractNode& node, OpenSCADOperator op)
{
  unsigned int dim = 0;
  for (const auto& item : this->visitedchildren[node.index()]) {
    if (!isValidDim(item, dim)) break;
  }
  if (dim == 2) return {applyToChildren2D(node, op)};
  else if (dim == 3) return applyToChildren3D(node, op);
  return {};
}

/*!
   Applies the operator to all child nodes of the given node.

   May return nullptr or any 3D Geometry object (can be either PolySet or CGAL_Nef_polyhedron)
 */
GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren3D(const AbstractNode& node, OpenSCADOperator op)
{
  Geometry::Geometries children = collectChildren3D(node);
  if (children.size() == 0) return {};

  if (op == OpenSCADOperator::HULL) {
    auto *ps = new PolySet(3, /* convex */ true);

    if (CGALUtils::applyHull(children, *ps)) {
      return ps;
    }

    delete ps;
    return {};
  } else if (op == OpenSCADOperator::FILL) {
    for (const auto& item : children) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(), "fill() not yet implemented for 3D");
    }
  }

  // Only one child -> this is a noop
  if (children.size() == 1) return {children.front().second};

  switch (op) {
  case OpenSCADOperator::MINKOWSKI:
  {
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
    if (actualchildren.size() == 1) return {actualchildren.front().second};
    return {CGALUtils::applyMinkowski(actualchildren)};
    break;
  }
  case OpenSCADOperator::UNION:
  {
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
    if (actualchildren.size() == 1) return {actualchildren.front().second};
#ifdef ENABLE_MANIFOLD
    if (Feature::ExperimentalManifold.is_enabled()) {
      return {ManifoldUtils::applyOperator3DManifold(actualchildren, op)};
    }
#endif
    return {CGALUtils::applyUnion3D(actualchildren.begin(), actualchildren.end())};
    break;
  }
  default:
  {
#ifdef ENABLE_MANIFOLD
    if (Feature::ExperimentalManifold.is_enabled()) {
      return {ManifoldUtils::applyOperator3DManifold(children, op)};
    }
#endif
    return {CGALUtils::applyOperator3D(children, op)};
    break;
  }
  }
}



/*!
   Apply 2D hull.

   May return an empty geometry but will not return nullptr.
 */

Polygon2d *GeometryEvaluator::applyHull2D(const AbstractNode& node)
{
  std::vector<const Polygon2d *> children = collectChildren2D(node);
  auto *geometry = new Polygon2d();

  using CGALPoint2 = CGAL::Point_2<CGAL::Cartesian<double>>;
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
        outline.vertices.push_back(Vector2d(p[0], p[1]));
      }
      geometry->addOutline(outline);
    } catch (const CGAL::Failure_exception& e) {
      LOG(message_group::Warning, "GeometryEvaluator::applyHull2D() during CGAL::convex_hull_2(): %1$s", e.what());
    }
  }
  return geometry;
}

Polygon2d *GeometryEvaluator::applyFill2D(const AbstractNode& node)
{
  // Merge and sanitize input geometry
  std::vector<const Polygon2d *> children = collectChildren2D(node);
  Polygon2d *geometry_in = ClipperUtils::apply(children, ClipperLib::ctUnion);

  std::vector<const Polygon2d *> newchildren;
  // Keep only the 'positive' outlines, eg: the outside edges
  for (const auto& outline : geometry_in->outlines()) {
    if (outline.positive) {
      auto *poly = new Polygon2d();
      poly->addOutline(outline);
      newchildren.push_back(poly);
    }
  }

  // Re-merge geometry in case of nested outlines
  return ClipperUtils::apply(newchildren, ClipperLib::ctUnion);
}

Geometry *GeometryEvaluator::applyHull3D(const AbstractNode& node)
{
  Geometry::Geometries children = collectChildren3D(node);

  auto *P = new PolySet(3);
  if (CGALUtils::applyHull(children, *P)) {
    return P;
  }
  delete P;
  return nullptr;
}

Polygon2d *GeometryEvaluator::applyMinkowski2D(const AbstractNode& node)
{
  std::vector<const Polygon2d *> children = collectChildren2D(node);
  if (!children.empty()) {
    return ClipperUtils::applyMinkowski(children);
  }
  return nullptr;
}

/*!
   Returns a list of Polygon2d children of the given node.
   May return empty Polygon2d object, but not nullptr objects
 */
std::vector<const Polygon2d *> GeometryEvaluator::collectChildren2D(const AbstractNode& node)
{
  std::vector<const Polygon2d *> children;
  for (const auto& item : this->visitedchildren[node.index()]) {
    auto& chnode = item.first;
    const shared_ptr<const Geometry>& chgeom = item.second;
    if (chnode->modinst->isBackground()) continue;

    // NB! We insert into the cache here to ensure that all children of
    // a node is a valid object. If we inserted as we created them, the
    // cache could have been modified before we reach this point due to a large
    // sibling object.
    smartCacheInsert(*chnode, chgeom);

    if (chgeom) {
      if (chgeom->getDimension() == 3) {
        LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(), "Ignoring 3D child object for 2D operation");
        children.push_back(nullptr); // replace 3D geometry with empty geometry
      } else {
        if (chgeom->isEmpty()) {
          children.push_back(nullptr);
        } else {
          const auto *polygons = dynamic_cast<const Polygon2d *>(chgeom.get());
          assert(polygons);
          children.push_back(polygons);
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
                                         const shared_ptr<const Geometry>& geom)
{
  const std::string& key = this->tree.getIdString(node);

  if (CGALCache::acceptsGeometry(geom)) {
    if (!CGALCache::instance()->contains(key)) CGALCache::instance()->insert(key, geom);
  } else {
    if (!GeometryCache::instance()->contains(key)) {
      if (!GeometryCache::instance()->insert(key, geom)) {
        LOG(message_group::Warning, "GeometryEvaluator: Node didn't fit into cache.");
      }
    }
  }
}

bool GeometryEvaluator::isSmartCached(const AbstractNode& node)
{
  const std::string& key = this->tree.getIdString(node);
  return (GeometryCache::instance()->contains(key) ||
          CGALCache::instance()->contains(key));
}

shared_ptr<const Geometry> GeometryEvaluator::smartCacheGet(const AbstractNode& node, bool preferNef)
{
  const std::string& key = this->tree.getIdString(node);
  shared_ptr<const Geometry> geom;
  bool hasgeom = GeometryCache::instance()->contains(key);
  bool hascgal = CGALCache::instance()->contains(key);
  if (hascgal && (preferNef || !hasgeom)) geom = CGALCache::instance()->get(key);
  else if (hasgeom) geom = GeometryCache::instance()->get(key);
  return geom;
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
    const shared_ptr<const Geometry>& chgeom = item.second;
    if (chnode->modinst->isBackground()) continue;

    // NB! We insert into the cache here to ensure that all children of
    // a node is a valid object. If we inserted as we created them, the
    // cache could have been modified before we reach this point due to a large
    // sibling object.
    smartCacheInsert(*chnode, chgeom);

    if (chgeom && chgeom->getDimension() == 2) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(), "Ignoring 2D child object for 3D operation");
      children.push_back(std::make_pair(item.first, nullptr)); // replace 2D geometry with empty geometry
    } else {
      // Add children if geometry is 3D OR null/empty
      children.push_back(item);
    }
  }
  return children;
}
/*!

 */
Polygon2d *GeometryEvaluator::applyToChildren2D(const AbstractNode& node, OpenSCADOperator op)
{
  node.progress_report();
  if (op == OpenSCADOperator::MINKOWSKI) {
    return applyMinkowski2D(node);
  } else if (op == OpenSCADOperator::HULL) {
    return applyHull2D(node);
  } else if (op == OpenSCADOperator::FILL) {
    return applyFill2D(node);
  }

  std::vector<const Polygon2d *> children = collectChildren2D(node);

  if (children.empty()) {
    return nullptr;
  }

  if (children.size() == 1) {
    if (children[0]) {
      return new Polygon2d(*children[0]); // Copy
    } else {
      return nullptr;
    }
  }

  ClipperLib::ClipType clipType;
  switch (op) {
  case OpenSCADOperator::UNION:
    clipType = ClipperLib::ctUnion;
    break;
  case OpenSCADOperator::INTERSECTION:
    clipType = ClipperLib::ctIntersection;
    break;
  case OpenSCADOperator::DIFFERENCE:
    clipType = ClipperLib::ctDifference;
    break;
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
void GeometryEvaluator::addToParent(const State& state,
                                    const AbstractNode& node,
                                    const shared_ptr<const Geometry>& geom)
{
  this->visitedchildren.erase(node.index());
  if (state.parent()) {
    this->visitedchildren[state.parent()->index()].push_back(std::make_pair(node.shared_from_this(), geom));
  } else {
    // Root node
    this->root = geom;
    assert(this->visitedchildren.empty());
  }
}

/*!
   Custom nodes are handled here => implicit union
 */
Response GeometryEvaluator::visit(State& state, const AbstractNode& node)
{
  if (state.isPrefix()) {
    if (isSmartCached(node)) return Response::PruneTraversal;
    state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    shared_ptr<const Geometry> geom;
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
        const shared_ptr<const Geometry>& chgeom = item.second;
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

Response GeometryEvaluator::lazyEvaluateRootNode(State& state, const AbstractNode& node) {
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
    shared_ptr<const Geometry> geom;

    unsigned int dim = 0;
    GeometryList::Geometries geometries;
    for (const auto& item : this->visitedchildren[node.index()]) {
      if (!isValidDim(item, dim)) break;
      auto& chnode = item.first;
      const shared_ptr<const Geometry>& chgeom = item.second;
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
    else if (geometries.size() > 1) geom.reset(new GeometryList(geometries));

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
    shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      const Geometry *geometry = applyToChildren2D(node, OpenSCADOperator::UNION);
      if (geometry) {
        const auto *polygon = dynamic_cast<const Polygon2d *>(geometry);
        // ClipperLib documentation: The formula for the number of steps in a full
        // circular arc is ... Pi / acos(1 - arc_tolerance / abs(delta))
        double n = Calc::get_fragments_from_r(std::abs(node.delta), node.fn, node.fs, node.fa);
        double arc_tolerance = std::abs(node.delta) * (1 - cos_degrees(180 / n));
        const Polygon2d *result = ClipperUtils::applyOffset(*polygon, node.delta, node.join_type, node.miter_limit, arc_tolerance);
        assert(result);
        geom.reset(result);
        delete geometry;
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
    state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    shared_ptr<const Geometry> geom;
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
    shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      const Geometry *geometry = node.createGeometry();
      assert(geometry);
      if (const auto *polygon = dynamic_cast<const Polygon2d *>(geometry)) {
        if (!polygon->isSanitized()) {
          Polygon2d *p = ClipperUtils::sanitize(*polygon);
          delete geometry;
          geometry = p;
        }
      }
      geom.reset(geometry);
    } else geom = smartCacheGet(node, state.preferNef());
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::PruneTraversal;
}

Response GeometryEvaluator::visit(State& state, const TextNode& node)
{
  if (state.isPrefix()) {
    shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      std::vector<const Geometry *> geometrylist = node.createGeometryList();
      std::vector<const Polygon2d *> polygonlist;
      for (const auto& geometry : geometrylist) {
        const auto *polygon = dynamic_cast<const Polygon2d *>(geometry);
        assert(polygon);
        polygonlist.push_back(polygon);
      }
      geom.reset(ClipperUtils::apply(polygonlist, ClipperLib::ctUnion));
    } else geom = GeometryCache::instance()->get(this->tree.getIdString(node));
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
    state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    shared_ptr<const Geometry> geom;
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
    shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      if (matrix_contains_infinity(node.matrix) || matrix_contains_nan(node.matrix)) {
        // due to the way parse/eval works we can't currently distinguish between NaN and Inf
        LOG(message_group::Warning, node.modinst->location(), this->tree.getDocumentPath(), "Transformation matrix contains Not-a-Number and/or Infinity - removing object.");
      } else {
        // First union all children
        ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
        if ((geom = res.constptr())) {
          if (geom->getDimension() == 2) {
            shared_ptr<const Polygon2d> polygons = dynamic_pointer_cast<const Polygon2d>(geom);
            assert(polygons);

            // If we got a const object, make a copy
            shared_ptr<Polygon2d> newpoly;
            if (res.isConst()) newpoly.reset(new Polygon2d(*polygons));
            else newpoly = dynamic_pointer_cast<Polygon2d>(res.ptr());

            Transform2d mat2;
            mat2.matrix() <<
              node.matrix(0, 0), node.matrix(0, 1), node.matrix(0, 3),
              node.matrix(1, 0), node.matrix(1, 1), node.matrix(1, 3),
              node.matrix(3, 0), node.matrix(3, 1), node.matrix(3, 3);
            newpoly->transform(mat2);
            // A 2D transformation may flip the winding order of a polygon.
            // If that happens with a sanitized polygon, we need to reverse
            // the winding order for it to be correct.
            if (newpoly->isSanitized() && mat2.matrix().determinant() <= 0) {
              geom.reset(ClipperUtils::sanitize(*newpoly));
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

static void translate_PolySet(PolySet& ps, const Vector3d& translation)
{
  for (auto& p : ps.polygons) {
    for (auto& v : p) {
      v += translation;
    }
  }
}

/*
   Compare Euclidean length of vectors
   Return:
    -1 : if v1  < v2
     0 : if v1 ~= v2 (approximation to compoensate for floating point precision)
     1 : if v1  > v2
 */
int sgn_vdiff(const Vector2d& v1, const Vector2d& v2) {
  constexpr double ratio_threshold = 1e5; // 10ppm difference
  double l1 = v1.norm();
  double l2 = v2.norm();
  // Compare the average and difference, to be independent of geometry scale.
  // If the difference is within ratio_threshold of the avg, treat as equal.
  double scale = (l1 + l2);
  double diff = 2 * std::fabs(l1 - l2) * ratio_threshold;
  return diff > scale ? (l1 < l2 ? -1 : 1) : 0;
}

/*
   Enable/Disable experimental 4-way split quads for linear_extrude, with added midpoint.
   These look very nice when(and only when) diagonals are near equal.
   This typically happens when an edge is colinear with the origin.
 */
//#define LINEXT_4WAY

/*
   Attempt to triangulate quads in an ideal way.
   Each quad is composed of two adjacent outline vertices: (prev1, curr1)
   and their corresponding transformed points one step up: (prev2, curr2).
   Quads are triangulated across the shorter of the two diagonals, which works well in most cases.
   However, when diagonals are equal length, decision may flip depending on other factors.
 */
static void add_slice(PolySet *ps, const Polygon2d& poly,
                      double rot1, double rot2,
                      double h1, double h2,
                      const Vector2d& scale1,
                      const Vector2d& scale2)
{
  Eigen::Affine2d trans1(Eigen::Scaling(scale1) * Eigen::Affine2d(rotate_degrees(-rot1)));
  Eigen::Affine2d trans2(Eigen::Scaling(scale2) * Eigen::Affine2d(rotate_degrees(-rot2)));
#ifdef LINEXT_4WAY
  Eigen::Affine2d trans_mid(Eigen::Scaling((scale1 + scale2) / 2) * Eigen::Affine2d(rotate_degrees(-(rot1 + rot2) / 2)));
  bool is_straight = rot1 == rot2 && scale1[0] == scale1[1] && scale2[0] == scale2[1];
#endif
  bool any_zero = scale2[0] == 0 || scale2[1] == 0;
  bool any_non_zero = scale2[0] != 0 || scale2[1] != 0;
  // Not likely to matter, but when no twist (rot2 == rot1),
  // setting back_twist true helps keep diagonals same as previous builds.
  bool back_twist = rot2 <= rot1;

  for (const auto& o : poly.outlines()) {
    Vector2d prev1 = trans1 * o.vertices[0];
    Vector2d prev2 = trans2 * o.vertices[0];

    // For equal length diagonals, flip selected choice depending on direction of twist and
    // whether the outline is negative (eg circle hole inside a larger circle).
    // This was tested against circles with a single point touching the origin,
    // and extruded with twist.  Diagonal choice determined by whichever option
    // matched the direction of diagonal for neighboring edges (which did not exhibit "equal" diagonals).
    bool flip = ((!o.positive) xor (back_twist));

    for (size_t i = 1; i <= o.vertices.size(); ++i) {
      Vector2d curr1 = trans1 * o.vertices[i % o.vertices.size()];
      Vector2d curr2 = trans2 * o.vertices[i % o.vertices.size()];

      int diff_sign = sgn_vdiff(prev1 - curr2, curr1 - prev2);
      bool splitfirst = diff_sign == -1 || (diff_sign == 0 && !flip);

#ifdef LINEXT_4WAY
      // Diagonals should be equal whenever an edge is co-linear with the origin (edge itself need not touch it)
      if (!is_straight && diff_sign == 0) {
        // Split into 4 triangles, with an added midpoint.
        //Vector2d mid_prev = trans3 * (prev1 +curr1+curr2)/4;
        Vector2d mid = trans_mid * (o.vertices[(i - 1) % o.vertices.size()] + o.vertices[i % o.vertices.size()]) / 2;
        double h_mid = (h1 + h2) / 2;
        ps->append_poly();
        ps->insert_vertex(prev1[0], prev1[1], h1);
        ps->insert_vertex(mid[0],   mid[1], h_mid);
        ps->insert_vertex(curr1[0], curr1[1], h1);
        ps->append_poly();
        ps->insert_vertex(curr1[0], curr1[1], h1);
        ps->insert_vertex(mid[0],   mid[1], h_mid);
        ps->insert_vertex(curr2[0], curr2[1], h2);
        ps->append_poly();
        ps->insert_vertex(curr2[0], curr2[1], h2);
        ps->insert_vertex(mid[0],   mid[1], h_mid);
        ps->insert_vertex(prev2[0], prev2[1], h2);
        ps->append_poly();
        ps->insert_vertex(prev2[0], prev2[1], h2);
        ps->insert_vertex(mid[0],   mid[1], h_mid);
        ps->insert_vertex(prev1[0], prev1[1], h1);
      } else
#endif // ifdef LINEXT_4WAY
      // Split along shortest diagonal,
      // unless at top for a 0-scaled axis (which can create 0 thickness "ears")
      if (splitfirst xor any_zero) {
        ps->append_poly();
        ps->insert_vertex(prev1[0], prev1[1], h1);
        ps->insert_vertex(curr2[0], curr2[1], h2);
        ps->insert_vertex(curr1[0], curr1[1], h1);
        if (!any_zero || (any_non_zero && prev2 != curr2)) {
          ps->append_poly();
          ps->insert_vertex(curr2[0], curr2[1], h2);
          ps->insert_vertex(prev1[0], prev1[1], h1);
          ps->insert_vertex(prev2[0], prev2[1], h2);
        }
      } else {
        ps->append_poly();
        ps->insert_vertex(prev1[0], prev1[1], h1);
        ps->insert_vertex(prev2[0], prev2[1], h2);
        ps->insert_vertex(curr1[0], curr1[1], h1);
        if (!any_zero || (any_non_zero && prev2 != curr2)) {
          ps->append_poly();
          ps->insert_vertex(prev2[0], prev2[1], h2);
          ps->insert_vertex(curr2[0], curr2[1], h2);
          ps->insert_vertex(curr1[0], curr1[1], h1);
        }
      }
      prev1 = curr1;
      prev2 = curr2;
    }
  }
}

// Insert vertices for segments interpolated between v0 and v1.
// The last vertex (t==1) is not added here to avoid duplicate vertices,
// since it will be the first vertex of the *next* edge.
static void add_segmented_edge(Outline2d& o, const Vector2d& v0, const Vector2d& v1, unsigned int edge_segments) {
  for (unsigned int j = 0; j < edge_segments; ++j) {
    double t = static_cast<double>(j) / edge_segments;
    o.vertices.push_back((1 - t) * v0 + t * v1);
  }
}

// For each edge in original outline, find its max length over all slice transforms,
// and divide into segments no longer than fs.
static Outline2d splitOutlineByFs(
  const Outline2d& o,
  const double twist, const double scale_x, const double scale_y,
  const double fs, unsigned int slices)
{
  const auto sz = o.vertices.size();

  Vector2d v0 = o.vertices[0];
  Outline2d o2;
  o2.positive = o.positive;

  // non-uniform scaling requires iterating over each slice transform
  // to find maximum length of a given edge.
  if (scale_x != scale_y) {
    for (size_t i = 1; i <= sz; ++i) {
      Vector2d v1 = o.vertices[i % sz];
      double max_edgelen = 0.0; // max length for single edge over all transformed slices
      for (unsigned int j = 0; j <= slices; j++) {
        double t = static_cast<double>(j) / slices;
        Vector2d scale(Calc::lerp(1, scale_x, t), Calc::lerp(1, scale_y, t));
        double rot = twist * t;
        Eigen::Affine2d trans(Eigen::Scaling(scale) * Eigen::Affine2d(rotate_degrees(-rot)));
        double edgelen = (trans * v1 - trans * v0).norm();
        max_edgelen = std::max(max_edgelen, edgelen);
      }
      auto edge_segments = static_cast<unsigned int>(std::ceil(max_edgelen / fs));
      add_segmented_edge(o2, v0, v1, edge_segments);
      v0 = v1;
    }
  } else { // uniform scaling
    double max_scale = std::max(scale_x, 1.0);
    for (size_t i = 1; i <= sz; ++i) {
      Vector2d v1 = o.vertices[i % sz];
      unsigned int edge_segments = static_cast<unsigned int>(std::ceil((v1 - v0).norm() * max_scale / fs));
      add_segmented_edge(o2, v0, v1, edge_segments);
      v0 = v1;
    }
  }
  return o2;
}

// While total outline segments < fn, increment segment_count for edge with largest
// (max_edge_length / segment_count).
static Outline2d splitOutlineByFn(
  const Outline2d& o,
  const double twist, const double scale_x, const double scale_y,
  const double fn, unsigned int slices)
{

  struct segment_tracker {
    size_t edge_index;
    double max_edgelen;
    unsigned int segment_count{1u};
    segment_tracker(size_t i, double len) : edge_index(i), max_edgelen(len) { }
    // metric for comparison: average between (max segment length, and max segment length after split)
    [[nodiscard]] double metric() const { return max_edgelen / (segment_count + 0.5); }
    bool operator<(const segment_tracker& rhs) const { return this->metric() < rhs.metric();  }
    [[nodiscard]] bool close_match(const segment_tracker& other) const {
      // Edges are grouped when metrics match by at least 99.9%
      constexpr double APPROX_EQ_RATIO = 0.999;
      double l1 = this->metric(), l2 = other.metric();
      return std::min(l1, l2) / std::max(l1, l2) >= APPROX_EQ_RATIO;
    }
  };

  const auto sz = o.vertices.size();
  std::vector<unsigned int> segment_counts(sz, 1);
  std::priority_queue<segment_tracker, std::vector<segment_tracker>> q;

  Vector2d v0 = o.vertices[0];
  // non-uniform scaling requires iterating over each slice transform
  // to find maximum length of a given edge.
  if (scale_x != scale_y) {
    for (size_t i = 1; i <= sz; ++i) {
      Vector2d v1 = o.vertices[i % sz];
      double max_edgelen = 0.0; // max length for single edge over all transformed slices
      for (unsigned int j = 0; j <= slices; j++) {
        double t = static_cast<double>(j) / slices;
        Vector2d scale(Calc::lerp(1, scale_x, t), Calc::lerp(1, scale_y, t));
        double rot = twist * t;
        Eigen::Affine2d trans(Eigen::Scaling(scale) * Eigen::Affine2d(rotate_degrees(-rot)));
        double edgelen = (trans * v1 - trans * v0).norm();
        max_edgelen = std::max(max_edgelen, edgelen);
      }
      q.emplace(i - 1, max_edgelen);
      v0 = v1;
    }
  } else { // uniform scaling
    double max_scale = std::max(scale_x, 1.0);
    for (size_t i = 1; i <= sz; ++i) {
      Vector2d v1 = o.vertices[i % sz];
      double max_edgelen = (v1 - v0).norm() * max_scale;
      q.emplace(i - 1, max_edgelen);
      v0 = v1;
    }
  }

  std::vector<segment_tracker> tmp_q;
  // Process priority_queue until number of segments is reached.
  size_t seg_total = sz;
  while (seg_total < fn) {
    auto current = q.top();

    // Group similar length segmented edges to keep result roughly symmetrical.
    while (!q.empty() && (tmp_q.empty() || current.close_match(tmp_q.front()))) {
      q.pop();
      tmp_q.push_back(current);
      current = q.top();
    }

    if (seg_total + tmp_q.size() <= fn) {
      while (!tmp_q.empty()) {
        current = tmp_q.back();
        tmp_q.pop_back();
        ++current.segment_count;
        ++segment_counts[current.edge_index];
        ++seg_total;
        q.push(current);
      }
    } else {
      // fn too low to segment last group, push back onto queue without change.
      while (!tmp_q.empty()) {
        current = tmp_q.back();
        tmp_q.pop_back();
        q.push(current);
      }
      break;
    }
  }

  // Create final segmented edges.
  Outline2d o2;
  o2.positive = o.positive;
  v0 = o.vertices[0];
  for (size_t i = 1; i <= sz; ++i) {
    Vector2d v1 = o.vertices[i % sz];
    add_segmented_edge(o2, v0, v1, segment_counts[i - 1]);
    v0 = v1;
  }

  assert(o2.vertices.size() <= fn);
  return o2;
}


/*!
   Input to extrude should be sanitized. This means non-intersecting, correct winding order
   etc., the input coming from a library like Clipper.
 */
static Geometry *extrudePolygon(const LinearExtrudeNode& node, const Polygon2d& poly)
{
  bool non_linear = node.twist != 0 || node.scale_x != node.scale_y;
  boost::tribool isConvex{poly.is_convex()};
  // Twist or non-uniform scale makes convex polygons into unknown polyhedrons
  if (isConvex && non_linear) isConvex = unknown;
  auto *ps = new PolySet(3, isConvex);
  ps->setConvexity(node.convexity);
  if (node.height <= 0) return ps;

  size_t slices;
  if (node.has_slices) {
    slices = node.slices;
  } else if (node.has_twist) {
    double max_r1_sqr = 0; // r1 is before scaling
    Vector2d scale(node.scale_x, node.scale_y);
    for (const auto& o : poly.outlines())
      for (const auto& v : o.vertices)
        max_r1_sqr = fmax(max_r1_sqr, v.squaredNorm());
    // Calculate Helical curve length for Twist with no Scaling
    if (node.scale_x == 1.0 && node.scale_y == 1.0) {
      slices = (unsigned int)Calc::get_helix_slices(max_r1_sqr, node.height, node.twist, node.fn, node.fs, node.fa);
    } else if (node.scale_x != node.scale_y) {  // non uniform scaling with twist using max slices from twist and non uniform scale
      double max_delta_sqr = 0; // delta from before/after scaling
      Vector2d scale(node.scale_x, node.scale_y);
      for (const auto& o : poly.outlines()) {
        for (const auto& v : o.vertices) {
          max_delta_sqr = fmax(max_delta_sqr, (v - v.cwiseProduct(scale)).squaredNorm());
        }
      }
      size_t slicesNonUniScale;
      size_t slicesTwist;
      slicesNonUniScale = (unsigned int)Calc::get_diagonal_slices(max_delta_sqr, node.height, node.fn, node.fs);
      slicesTwist = (unsigned int)Calc::get_helix_slices(max_r1_sqr, node.height, node.twist, node.fn, node.fs, node.fa);
      slices = std::max(slicesNonUniScale, slicesTwist);
    } else { // uniform scaling with twist, use conical helix calculation
      slices = (unsigned int)Calc::get_conical_helix_slices(max_r1_sqr, node.height, node.twist, node.scale_x, node.fn, node.fs, node.fa);
    }
  } else if (node.scale_x != node.scale_y) {
    // Non uniform scaling, w/o twist
    double max_delta_sqr = 0; // delta from before/after scaling
    Vector2d scale(node.scale_x, node.scale_y);
    for (const auto& o : poly.outlines()) {
      for (const auto& v : o.vertices) {
        max_delta_sqr = fmax(max_delta_sqr, (v - v.cwiseProduct(scale)).squaredNorm());
      }
    }
    slices = Calc::get_diagonal_slices(max_delta_sqr, node.height, node.fn, node.fs);
  } else {
    // uniform or [1,1] scaling w/o twist needs only one slice
    slices = 1;
  }

  // Calculate outline segments if appropriate.
  Polygon2d seg_poly;
  bool is_segmented = false;
  if (node.has_segments) {
    // Set segments = 0 to disable
    if (node.segments > 0) {
      for (const auto& o : poly.outlines()) {
        if (o.vertices.size() >= node.segments) {
          seg_poly.addOutline(o);
        } else {
          seg_poly.addOutline(splitOutlineByFn(o, node.twist, node.scale_x, node.scale_y, node.segments, slices));
        }
      }
      is_segmented = true;
    }
  } else if (non_linear) {
    if (node.fn > 0.0) {
      for (const auto& o : poly.outlines()) {
        if (o.vertices.size() >= node.fn) {
          seg_poly.addOutline(o);
        } else {
          seg_poly.addOutline(splitOutlineByFn(o, node.twist, node.scale_x, node.scale_y, node.fn, slices));
        }
      }
    } else { // $fs and $fa based segmentation
      auto fa_segs = static_cast<unsigned int>(std::ceil(360.0 / node.fa));
      for (const auto& o : poly.outlines()) {
        if (o.vertices.size() >= fa_segs) {
          seg_poly.addOutline(o);
        } else {
          // try splitting by $fs, then check if $fa results in less segments
          auto fsOutline = splitOutlineByFs(o, node.twist, node.scale_x, node.scale_y, node.fs, slices);
          if (fsOutline.vertices.size() >= fa_segs) {
            seg_poly.addOutline(splitOutlineByFn(o, node.twist, node.scale_x, node.scale_y, fa_segs, slices));
          } else {
            seg_poly.addOutline(std::move(fsOutline));
          }
        }
      }
    }
    is_segmented = true;
  }

  const Polygon2d& polyref = is_segmented ? seg_poly : poly;

  double h1, h2;
  if (node.center) {
    h1 = -node.height / 2.0;
    h2 = +node.height / 2.0;
  } else {
    h1 = 0;
    h2 = node.height;
  }

  // Create bottom face.
  PolySet *ps_bottom = polyref.tessellate(); // bottom
  // Flip vertex ordering for bottom polygon
  for (auto& p : ps_bottom->polygons) {
    std::reverse(p.begin(), p.end());
  }
  translate_PolySet(*ps_bottom, Vector3d(0, 0, h1));
  ps->append(*ps_bottom);
  delete ps_bottom;

  // Create slice sides.
  for (unsigned int j = 0; j < slices; j++) {
    double rot1 = node.twist * j / slices;
    double rot2 = node.twist * (j + 1) / slices;
    double height1 = h1 + (h2 - h1) * j / slices;
    double height2 = h1 + (h2 - h1) * (j + 1) / slices;
    Vector2d scale1(1 - (1 - node.scale_x) * j / slices,
                    1 - (1 - node.scale_y) * j / slices);
    Vector2d scale2(1 - (1 - node.scale_x) * (j + 1) / slices,
                    1 - (1 - node.scale_y) * (j + 1) / slices);
    add_slice(ps, polyref, rot1, rot2, height1, height2, scale1, scale2);
  }

  // Create top face.
  // If either scale components are 0, then top will be zero-area, so skip it.
  if (node.scale_x != 0 && node.scale_y != 0) {
    Polygon2d top_poly(polyref);
    Eigen::Affine2d trans(Eigen::Scaling(node.scale_x, node.scale_y) * Eigen::Affine2d(rotate_degrees(-node.twist)));
    top_poly.transform(trans);
    PolySet *ps_top = top_poly.tessellate();
    translate_PolySet(*ps_top, Vector3d(0, 0, h2));
    ps->append(*ps_top);
    delete ps_top;
  }

  return ps;
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
    shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      const Geometry *geometry = nullptr;
      if (!node.filename.empty()) {
        DxfData dxf(node.fn, node.fs, node.fa, node.filename, node.layername, node.origin_x, node.origin_y, node.scale_x);

        Polygon2d *p2d = dxf.toPolygon2d();
        if (p2d) geometry = ClipperUtils::sanitize(*p2d);
        delete p2d;
      } else {
        geometry = applyToChildren2D(node, OpenSCADOperator::UNION);
      }
      if (geometry) {
        const auto *polygons = dynamic_cast<const Polygon2d *>(geometry);
        Geometry *extruded = extrudePolygon(node, *polygons);
        assert(extruded);
        geom.reset(extruded);
        delete geometry;
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

static void fill_ring(std::vector<Vector3d>& ring, const Outline2d& o, double a, bool flip)
{
  if (flip) {
    unsigned int l = o.vertices.size() - 1;
    for (unsigned int i = 0; i < o.vertices.size(); ++i) {
      ring[i][0] = o.vertices[l - i][0] * sin_degrees(a);
      ring[i][1] = o.vertices[l - i][0] * cos_degrees(a);
      ring[i][2] = o.vertices[l - i][1];
    }
  } else {
    for (unsigned int i = 0; i < o.vertices.size(); ++i) {
      ring[i][0] = o.vertices[i][0] * sin_degrees(a);
      ring[i][1] = o.vertices[i][0] * cos_degrees(a);
      ring[i][2] = o.vertices[i][1];
    }
  }
}

/*!
   Input to extrude should be clean. This means non-intersecting, correct winding order
   etc., the input coming from a library like Clipper.

   FIXME: We should handle some common corner cases better:
   o 2D polygon having an edge being on the Y axis:
    In this case, we don't need to generate geometry involving this edge as it
    will be an internal edge.
   o 2D polygon having a vertex touching the Y axis:
    This is more complex as the resulting geometry will (may?) be nonmanifold.
    In any case, the previous case is a specialization of this, so the following
    should be handled for both cases:
    Since the ring associated with this vertex will have a radius of zero, it will
    collapse to one vertex. Any quad using this ring will be collapsed to a triangle.

   Currently, we generate a lot of zero-area triangles

 */
static Geometry *rotatePolygon(const RotateExtrudeNode& node, const Polygon2d& poly)
{
  if (node.angle == 0) return nullptr;

  auto *ps = new PolySet(3);
  ps->setConvexity(node.convexity);

  double min_x = 0;
  double max_x = 0;
  unsigned int fragments = 0;
  for (const auto& o : poly.outlines()) {
    for (const auto& v : o.vertices) {
      min_x = fmin(min_x, v[0]);
      max_x = fmax(max_x, v[0]);
    }
  }

  if ((max_x - min_x) > max_x && (max_x - min_x) > fabs(min_x)) {
    LOG(message_group::Error, "all points for rotate_extrude() must have the same X coordinate sign (range is %1$.2f -> %2$.2f)", min_x, max_x);
    delete ps;
    return nullptr;
  }

  fragments = (unsigned int)std::ceil(fmax(Calc::get_fragments_from_r(max_x - min_x, node.fn, node.fs, node.fa) * std::abs(node.angle) / 360, 1));

  bool flip_faces = (min_x >= 0 && node.angle > 0 && node.angle != 360) || (min_x < 0 && (node.angle < 0 || node.angle == 360));

  if (node.angle != 360) {
    PolySet *ps_start = poly.tessellate(); // starting face
    Transform3d rot(angle_axis_degrees(90, Vector3d::UnitX()));
    ps_start->transform(rot);
    // Flip vertex ordering
    if (!flip_faces) {
      for (auto& p : ps_start->polygons) {
        std::reverse(p.begin(), p.end());
      }
    }
    ps->append(*ps_start);
    delete ps_start;

    PolySet *ps_end = poly.tessellate();
    Transform3d rot2(angle_axis_degrees(node.angle, Vector3d::UnitZ()) * angle_axis_degrees(90, Vector3d::UnitX()));
    ps_end->transform(rot2);
    if (flip_faces) {
      for (auto& p : ps_end->polygons) {
        std::reverse(p.begin(), p.end());
      }
    }
    ps->append(*ps_end);
    delete ps_end;
  }

  for (const auto& o : poly.outlines()) {
    std::vector<Vector3d> rings[2];
    rings[0].resize(o.vertices.size());
    rings[1].resize(o.vertices.size());

    fill_ring(rings[0], o, (node.angle == 360) ? -90 : 90, flip_faces); // first ring
    for (unsigned int j = 0; j < fragments; ++j) {
      double a;
      if (node.angle == 360) a = -90 + ((j + 1) % fragments) * 360.0 / fragments; // start on the -X axis, for legacy support
      else a = 90 - (j + 1) * node.angle / fragments; // start on the X axis
      fill_ring(rings[(j + 1) % 2], o, a, flip_faces);

      for (size_t i = 0; i < o.vertices.size(); ++i) {
        ps->append_poly();
        ps->insert_vertex(rings[j % 2][i]);
        ps->insert_vertex(rings[(j + 1) % 2][(i + 1) % o.vertices.size()]);
        ps->insert_vertex(rings[j % 2][(i + 1) % o.vertices.size()]);
        ps->append_poly();
        ps->insert_vertex(rings[j % 2][i]);
        ps->insert_vertex(rings[(j + 1) % 2][i]);
        ps->insert_vertex(rings[(j + 1) % 2][(i + 1) % o.vertices.size()]);
      }
    }
  }

  return ps;
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
    shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      const Geometry *geometry = nullptr;
      if (!node.filename.empty()) {
        DxfData dxf(node.fn, node.fs, node.fa, node.filename, node.layername, node.origin_x, node.origin_y, node.scale);
        Polygon2d *p2d = dxf.toPolygon2d();
        if (p2d) geometry = ClipperUtils::sanitize(*p2d);
        delete p2d;
      } else {
        geometry = applyToChildren2D(node, OpenSCADOperator::UNION);
      }
      if (geometry) {
        const auto *polygons = dynamic_cast<const Polygon2d *>(geometry);
        Geometry *rotated = rotatePolygon(node, *polygons);
        geom.reset(rotated);
        delete geometry;
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

shared_ptr<const Geometry> GeometryEvaluator::projectionCut(const ProjectionNode& node)
{
  shared_ptr<const Geometry> geom;
  shared_ptr<const Geometry> newgeom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (newgeom) {
    auto Nptr = CGALUtils::getNefPolyhedronFromGeometry(newgeom);
    if (Nptr && !Nptr->isEmpty()) {
      Polygon2d *poly = CGALUtils::project(*Nptr, node.cut_mode);
      if (poly) {
        poly->setConvexity(node.convexity);
        geom.reset(poly);
      }
    }
  }
  return geom;
}

shared_ptr<const Geometry> GeometryEvaluator::projectionNoCut(const ProjectionNode& node)
{
  shared_ptr<const Geometry> geom;
  std::vector<const Polygon2d *> tmp_geom;
  BoundingBox bounds;
  for (const auto& item : this->visitedchildren[node.index()]) {
    auto& chnode = item.first;
    const shared_ptr<const Geometry>& chgeom = item.second;
    if (chnode->modinst->isBackground()) continue;

    const Polygon2d *poly = nullptr;

    // Clipper version of Geometry projection
    // Clipper doesn't handle meshes very well.
    // It's better in V6 but not quite there. FIXME: stand-alone example.
    // project chgeom -> polygon2d
    auto chPS = CGALUtils::getGeometryAsPolySet(chgeom);
    if (chPS) poly = PolySetUtils::project(*chPS);

    if (poly) {
      bounds.extend(poly->getBoundingBox());
      tmp_geom.push_back(poly);
    }

  }
  int pow2 = ClipperUtils::getScalePow2(bounds);

  ClipperLib::Clipper sumclipper;
  for (auto poly : tmp_geom) {
    ClipperLib::Paths result = ClipperUtils::fromPolygon2d(*poly, pow2);
    // Using NonZero ensures that we don't create holes from polygons sharing
    // edges since we're unioning a mesh
    result = ClipperUtils::process(result, ClipperLib::ctUnion, ClipperLib::pftNonZero);
    // Add correctly winded polygons to the main clipper
    sumclipper.AddPaths(result, ClipperLib::ptSubject, true);
    delete poly;
  }

  ClipperLib::PolyTree sumresult;
  // This is key - without StrictlySimple, we tend to get self-intersecting results
  sumclipper.StrictlySimple(true);
  sumclipper.Execute(ClipperLib::ctUnion, sumresult, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  if (sumresult.Total() > 0) {
    geom.reset(ClipperUtils::toPolygon2d(sumresult, pow2));
  }

  return geom;
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
    shared_ptr<const Geometry> geom;
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
    shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      switch (node.type) {
      case CgalAdvType::MINKOWSKI: {
        ResultObject res = applyToChildren(node, OpenSCADOperator::MINKOWSKI);
        geom = res.constptr();
        // If we added convexity, we need to pass it on
        if (geom && geom->getConvexity() != node.convexity) {
          shared_ptr<Geometry> editablegeom;
          // If we got a const object, make a copy
          if (res.isConst()) editablegeom.reset(geom->copy());
          else editablegeom = res.ptr();
          geom = editablegeom;
          editablegeom->setConvexity(node.convexity);
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
      default:
        assert(false && "not implemented");
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
    state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    shared_ptr<const Geometry> geom;
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

static Geometry *roofOverPolygon(const RoofNode& node, const Polygon2d& poly)
{
  PolySet *roof = nullptr;
  if (node.method == "voronoi") {
    roof = roof_vd::voronoi_diagram_roof(poly, node.fa, node.fs);
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
    shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      const Geometry *geometry = applyToChildren2D(node, OpenSCADOperator::UNION);
      if (geometry) {
        auto *polygons = dynamic_cast<const Polygon2d *>(geometry);
        Geometry *roof;
        try {
          roof = roofOverPolygon(node, *polygons);
        } catch (RoofNode::roof_exception& e) {
          LOG(message_group::Error, node.modinst->location(), this->tree.getDocumentPath(),
              "Skeleton computation error. " + e.message());
          roof = new PolySet(3);
        }
        assert(roof);
        geom.reset(roof);
        delete geometry;
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
  }
  return Response::ContinueTraversal;
}
