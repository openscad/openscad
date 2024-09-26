#include "core/CSGTreeEvaluator.h"
#include "core/State.h"
#include "core/CsgOpNode.h"
#include "core/ModuleInstantiation.h"
#include "core/CSGNode.h"
#include "core/TransformNode.h"
#include "core/ColorNode.h"
#include "core/RenderNode.h"
#include "core/CgalAdvNode.h"
#include "utils/printutils.h"
#include "geometry/GeometryEvaluator.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"

#include <memory>
#include <string>
#include <map>
#include <list>
#include <cassert>
#include <cstddef>
#include <boost/range/adaptor/reversed.hpp>

/*!
   \class CSGTreeEvaluator

   A visitor responsible for creating a binary tree of CSGNode nodes used for rendering
   with OpenCSG.
 */

std::shared_ptr<CSGNode> CSGTreeEvaluator::buildCSGTree(const AbstractNode& node)
{
  this->traverse(node);

  std::shared_ptr<CSGNode> t(this->stored_term[node.index()]);
  if (t) {
    if (t->isHighlight()) this->highlightNodes.push_back(t);
    if (t->isBackground()) {
      this->backgroundNodes.push_back(t);
      t.reset();
    }
  }

  return this->rootNode = t;
}

void CSGTreeEvaluator::applyBackgroundAndHighlight(State& /*state*/, const AbstractNode& node)
{
  for (const auto& chnode : this->visitedchildren[node.index()]) {
    std::shared_ptr<CSGNode> t(this->stored_term[chnode->index()]);
    this->stored_term.erase(chnode->index());
    if (t) {
      if (t->isBackground()) this->backgroundNodes.push_back(t);
      if (t->isHighlight()) this->highlightNodes.push_back(t);
    }
  }
}

void CSGTreeEvaluator::applyToChildren(State& state, const AbstractNode& node, OpenSCADOperator op)
{
  const auto& vc = this->visitedchildren[node.index()];
  if (vc.empty()) {
    this->stored_term[node.index()] = CSGNode::createEmptySet();
    return;
  }

  std::shared_ptr<CSGNode> t1;
  for (const auto& chnode : vc) {
    std::shared_ptr<CSGNode> t2(this->stored_term[chnode->index()]);
    this->stored_term.erase(chnode->index());
    if (t2 && !t1) {
      t1 = t2;
    } else if (t2 && t1) {

      std::shared_ptr<CSGNode> t;
      // Handle background
      // Background objects are simply moved to backgroundNodes
      if (t2->isBackground()) {
        t = t1;
        this->backgroundNodes.push_back(t2);
      } else if (t1->isBackground()) {
        t = t2;
        this->backgroundNodes.push_back(t1);
      } else {
        t = CSGOperation::createCSGNode(op, t1, t2);
      }
      // Handle highlight
      switch (op) {
      case OpenSCADOperator::DIFFERENCE:
        if (t != t1 && t1->isHighlight()) {
          t->setHighlight(true);
        } else if (t != t2 && t2->isHighlight()) {
          this->highlightNodes.push_back(t2);
        }
        break;
      case OpenSCADOperator::INTERSECTION:
        if (t && !t->isEmptySet() && t != t1 && t != t2 &&
            t1->isHighlight() && t2->isHighlight()) {
          t->setHighlight(true);
        } else {
          if (t != t1 && t1->isHighlight()) {
            this->highlightNodes.push_back(t1);
          }
          if (t != t2 && t2->isHighlight()) {
            this->highlightNodes.push_back(t2);
          }
        }
        break;
      case OpenSCADOperator::UNION:
        if (t != t1 && t != t2 &&
            t1->isHighlight() && t2->isHighlight()) {
          t->setHighlight(true);
        } else if (t != t1 && t1->isHighlight()) {
          this->highlightNodes.push_back(t1);
          t = t2;
        } else if (t != t2 && t2->isHighlight()) {
          this->highlightNodes.push_back(t2);
          t = t1;
        }
        break;
      case OpenSCADOperator::MINKOWSKI:
      case OpenSCADOperator::HULL:
      case OpenSCADOperator::FILL:
      case OpenSCADOperator::RESIZE:
        break;
      }
      t1 = t;
    }
  }
  if (t1) {
    if (node.modinst->isBackground() || state.isBackground()) t1->setBackground(true);
    if (node.modinst->isHighlight() || state.isHighlight()) t1->setHighlight(true);
  }
  this->stored_term[node.index()] = t1;
}

Response CSGTreeEvaluator::visit(State& state, const AbstractNode& node)
{
  if (state.isPostfix()) {
    applyToChildren(state, node, OpenSCADOperator::UNION);
    addToParent(state, node);
  }
  return Response::ContinueTraversal;
}

Response CSGTreeEvaluator::visit(State& state, const AbstractIntersectionNode& node)
{
  if (state.isPostfix()) {
    applyToChildren(state, node, OpenSCADOperator::INTERSECTION);
    addToParent(state, node);
  }
  return Response::ContinueTraversal;
}

Response CSGTreeEvaluator::visit(State& state, const class ListNode& node)
{
  if (state.parent()) {
    if (state.isPrefix()) {
      if (node.modinst->isHighlight()) state.setHighlight(true);
      if (node.modinst->isBackground()) state.setBackground(true);
    }
    if (state.isPostfix()) {
      for (auto &chnode : this->visitedchildren[node.index()]) {
        addToParent(state, *chnode);
      }
    }
    return Response::ContinueTraversal;
  } else {
    // Handle root modifier on ListNode just like a group
    return visit(state, (const AbstractNode&)node);
  }

}

// Creates a 1-unit-thick PolySet with dim==2 from a Polygon2d.
std::shared_ptr<const PolySet> polygon2dToPolySet(const Polygon2d &p2d) {
  const auto ps = p2d.tessellate();
  constexpr int dim = 2;
  // Estimating num vertices and polygons: top + bottom + sides
  PolySetBuilder builder(ps->vertices.size() * 2, 
                         ps->indices.size() * 2 + ps->vertices.size(),
                         dim, p2d.is_convex());
  builder.setConvexity(p2d.getConvexity());

  // Create bottom face.
  for (const auto& poly : ps->indices) {
    builder.beginPolygon(poly.size());
    // Flip vertex ordering for bottom polygon
    for (const auto& ind: boost::adaptors::reverse(poly)) {
      builder.addVertex(ps->vertices[ind] - Vector3d(0, 0, 0.5));
    }
  }

  // Create top face.
  for (const auto& poly : ps->indices) {
    builder.beginPolygon(poly.size());
    for (const auto& ind: poly) {
      builder.addVertex(ps->vertices[ind] + Vector3d(0, 0, 0.5));
    }
  }

  // Create sides
  for (const auto& o : p2d.outlines()) {
    for (size_t i = 0; i < o.vertices.size(); ++i) {
      const Vector2d &prev = o.vertices[i];
      const Vector2d &curr = o.vertices[(i+1)%o.vertices.size()];
      builder.appendPolygon({
        Vector3d(prev[0], prev[1], -0.5),
        Vector3d(curr[0], curr[1], -0.5),
        Vector3d(curr[0], curr[1], 0.5),
        Vector3d(prev[0], prev[1], 0.5),
      });
    }
  }

  return builder.build();
}


std::shared_ptr<CSGNode> CSGTreeEvaluator::evaluateCSGNodeFromGeometry(
  State& state, const std::shared_ptr<const Geometry>& geom,
  const ModuleInstantiation *modinst, const AbstractNode& node)
{
  assert(geom);
  // We cannot render Polygon2d directly, so we convert it to a PolySet here
  std::shared_ptr<const PolySet> ps;
  if (!geom->isEmpty()) {
    if (auto p2d = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
      ps = polygon2dToPolySet(*p2d);
    }
    // 3D PolySets are tessellated before inserting into Geometry cache, inside GeometryEvaluator::evaluateGeometry
    else {
      ps = std::dynamic_pointer_cast<const PolySet>(geom);
    }
  }

  std::shared_ptr<CSGNode> t(new CSGLeaf(ps, state.matrix(), state.color(), STR(node.name(), node.index()), node.index()));
  if (modinst->isHighlight() || state.isHighlight()) t->setHighlight(true);
  if (modinst->isBackground() || state.isBackground()) t->setBackground(true);
  return t;
}

Response CSGTreeEvaluator::visit(State& state, const AbstractPolyNode& node)
{
  if (state.isPostfix()) {
    std::shared_ptr<CSGNode> t1;
    if (this->geomevaluator) {
      auto geom = this->geomevaluator->evaluateGeometry(node, false);
      if (geom) {
        t1 = evaluateCSGNodeFromGeometry(state, geom, node.modinst, node);
      } else {
        t1 = CSGNode::createEmptySet();
      }
      node.progress_report();
    }
    this->stored_term[node.index()] = t1;
    addToParent(state, node);
  }
  return Response::ContinueTraversal;
}

Response CSGTreeEvaluator::visit(State& state, const CsgOpNode& node)
{
  if (state.isPostfix()) {
    applyToChildren(state, node, node.type);
    addToParent(state, node);
  }
  return Response::ContinueTraversal;
}

Response CSGTreeEvaluator::visit(State& state, const TransformNode& node)
{
  if (state.isPrefix()) {
    if (matrix_contains_infinity(node.matrix) || matrix_contains_nan(node.matrix)) {
      LOG(message_group::Warning, "Transformation matrix contains Not-a-Number and/or Infinity - removing object.");
      return Response::PruneTraversal;
    }
    state.setMatrix(state.matrix() * node.matrix);
  }
  if (state.isPostfix()) {
    applyToChildren(state, node, OpenSCADOperator::UNION);
    addToParent(state, node);
  }
  return Response::ContinueTraversal;
}

Response CSGTreeEvaluator::visit(State& state, const ColorNode& node)
{
  if (state.isPrefix()) {
    if (!state.color().isValid()) state.setColor(node.color);
  }
  if (state.isPostfix()) {
    applyToChildren(state, node, OpenSCADOperator::UNION);
    addToParent(state, node);
  }
  return Response::ContinueTraversal;
}

// FIXME: If we've got CGAL support, render this node as a CGAL union into a PolySet
Response CSGTreeEvaluator::visit(State& state, const RenderNode& node)
{
  if (state.isPostfix()) {
    std::shared_ptr<CSGNode> t1;
    std::shared_ptr<const Geometry> geom;
    if (this->geomevaluator) {
      geom = this->geomevaluator->evaluateGeometry(node, false);
      if (geom) {
        t1 = evaluateCSGNodeFromGeometry(state, geom, node.modinst, node);
      } else {
        t1 = CSGNode::createEmptySet();
      }
      node.progress_report();
    }
    this->stored_term[node.index()] = t1;
    addToParent(state, node);
  }
  return Response::ContinueTraversal;
}

Response CSGTreeEvaluator::visit(State& state, const CgalAdvNode& node)
{
  if (state.isPostfix()) {
    std::shared_ptr<CSGNode> t1;
    // FIXME: Calling evaluator directly since we're not a PolyNode. Generalize this.
    std::shared_ptr<const Geometry> geom;
    if (this->geomevaluator) {
      geom = this->geomevaluator->evaluateGeometry(node, false);
      if (geom) {
        t1 = evaluateCSGNodeFromGeometry(state, geom, node.modinst, node);
      } else {
        t1 = CSGNode::createEmptySet();
      }
      node.progress_report();
    }
    this->stored_term[node.index()] = t1;
    applyBackgroundAndHighlight(state, node);
    addToParent(state, node);
  }
  return Response::ContinueTraversal;
}

/*!
   Adds ourself to out parent's list of traversed children.
   Call this for _every_ node which affects output during traversal.
    Usually, this should be called from the postfix stage, but for some nodes, we defer traversal letting other components (e.g. CGAL) render the subgraph, and we'll then call this from prefix and prune further traversal.
 */
void CSGTreeEvaluator::addToParent(const State& state, const AbstractNode& node)
{
  this->visitedchildren.erase(node.index());
  if (state.parent()) {
    this->visitedchildren[state.parent()->index()].push_back(node.shared_from_this());
  }
}
