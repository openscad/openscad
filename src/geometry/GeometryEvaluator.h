#pragma once

#include "core/NodeVisitor.h"
#include "core/enums.h"
#include "geometry/Geometry.h"

#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include <map>

class CGAL_Nef_polyhedron;
class Polygon2d;
class Tree;

// This evaluates a node tree into concrete geometry usign an underlying geometry engine
// FIXME: Ideally, each engine should implement its own subtype. Instead we currently have
// multiple embedded engines with varoius methods of selecting the right one.
class GeometryEvaluator : public NodeVisitor
{
public:
  GeometryEvaluator(const Tree& tree);

  std::shared_ptr<const Geometry> evaluateGeometry(const AbstractNode& node, bool allownef);

  Response visit(State& state, const AbstractNode& node) override;
  Response visit(State& state, const ColorNode& node) override;
  Response visit(State& state, const AbstractIntersectionNode& node) override;
  Response visit(State& state, const AbstractPolyNode& node) override;
  Response visit(State& state, const LinearExtrudeNode& node) override;
  Response visit(State& state, const RotateExtrudeNode& node) override;
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
  Response visit(State& state, const RoofNode& node) override;
#endif
  Response visit(State& state, const ListNode& node) override;
  Response visit(State& state, const GroupNode& node) override;
  Response visit(State& state, const RootNode& node) override;
  Response visit(State& state, const LeafNode& node) override;
  Response visit(State& state, const TransformNode& node) override;
  Response visit(State& state, const CsgOpNode& node) override;
  Response visit(State& state, const CgalAdvNode& node) override;
  Response visit(State& state, const ProjectionNode& node) override;
  Response visit(State& state, const RenderNode& node) override;
  Response visit(State& state, const TextNode& node) override;
  Response visit(State& state, const OffsetNode& node) override;

  [[nodiscard]] const Tree& getTree() const { return this->tree; }

private:
  class ResultObject
  {
public:
    // This makes it explicit if we want a const vs. non-const result.
    // This is important to avoid inadvertently tagging a geometry as const when
    // the underlying geometry is actually mutable. 
    // The template trick, combined with private constructors, makes it possible
    // to create a ResultObject containing a const, _only_ from const objects
    // (i.e. no implicit conversion from non-const to const).
    template<class T> static ResultObject constResult(std::shared_ptr<const T> geom) {return {geom};}
    template<class T> static ResultObject mutableResult(std::shared_ptr<T> geom) {return {geom};}

    // Default constructor with nullptr can be used to represent empty geometry,
    // for example union() with no children, etc.
    ResultObject() : is_const(true) {}
    std::shared_ptr<Geometry> ptr() { assert(!is_const); return pointer; }
    [[nodiscard]] std::shared_ptr<const Geometry> constptr() const {
      return is_const ? const_pointer : std::static_pointer_cast<const Geometry>(pointer);
    }
    std::shared_ptr<Geometry> asMutableGeometry() {
      if (is_const) return {constptr() ? constptr()->copy() : nullptr};
      else return ptr();
    }
private:
    template<class T> ResultObject(std::shared_ptr<const T> g) : is_const(true), const_pointer(std::move(g)) {}
    template<class T> ResultObject(std::shared_ptr<T> g) : is_const(false), pointer(std::move(g)) {}

    bool is_const;
    std::shared_ptr<Geometry> pointer;
    std::shared_ptr<const Geometry> const_pointer;
  };

  void smartCacheInsert(const AbstractNode& node, const std::shared_ptr<const Geometry>& geom);
  std::shared_ptr<const Geometry> smartCacheGet(const AbstractNode& node, bool preferNef);
  bool isSmartCached(const AbstractNode& node);
  bool isValidDim(const Geometry::GeometryItem& item, unsigned int& dim) const;
  std::vector<std::shared_ptr<const Polygon2d>> collectChildren2D(const AbstractNode& node);
  Geometry::Geometries collectChildren3D(const AbstractNode& node);
  std::unique_ptr<Polygon2d> applyMinkowski2D(const AbstractNode& node);
  std::unique_ptr<Polygon2d> applyHull2D(const AbstractNode& node);
  std::unique_ptr<Polygon2d> applyFill2D(const AbstractNode& node);
  std::unique_ptr<Geometry> applyHull3D(const AbstractNode& node);
  void applyResize3D(CGAL_Nef_polyhedron& N, const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize);
  std::unique_ptr<Polygon2d> applyToChildren2D(const AbstractNode& node, OpenSCADOperator op);
  ResultObject applyToChildren3D(const AbstractNode& node, OpenSCADOperator op);
  ResultObject applyToChildren(const AbstractNode& node, OpenSCADOperator op);
  std::shared_ptr<const Geometry> projectionCut(const ProjectionNode& node);
  std::shared_ptr<const Geometry> projectionNoCut(const ProjectionNode& node);

  void addToParent(const State& state, const AbstractNode& node, const std::shared_ptr<const Geometry>& geom);
  Response lazyEvaluateRootNode(State& state, const AbstractNode& node);

  std::map<int, Geometry::Geometries> visitedchildren;
  const Tree& tree;
  std::shared_ptr<const Geometry> root;

public:
};
