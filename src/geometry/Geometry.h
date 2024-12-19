#pragma once

#include <cassert>
#include <utility>
#include <cstddef>
#include <string>
#include <list>
#include <memory>

#include "geometry/linalg.h"

class AbstractNode;
class CGAL_Nef_polyhedron;
class CGALHybridPolyhedron;
class GeometryList;
class GeometryVisitor;
class Polygon2d;
class PolySet;
#ifdef ENABLE_MANIFOLD
class ManifoldGeometry;
#endif

class Geometry
{
public:
  using GeometryItem = std::pair<std::shared_ptr<const AbstractNode>, std::shared_ptr<const Geometry>>;
  using Geometries = std::list<GeometryItem>;

  Geometry() = default;
  Geometry(const Geometry&) = default;
  Geometry& operator=(const Geometry&) = default;
  Geometry(Geometry&&) = default;
  Geometry& operator=(Geometry&&) = default;
  virtual ~Geometry() = default;

  [[nodiscard]] virtual size_t memsize() const = 0;
  [[nodiscard]] virtual BoundingBox getBoundingBox() const = 0;
  [[nodiscard]] virtual std::string dump() const = 0;
  [[nodiscard]] virtual unsigned int getDimension() const = 0;
  [[nodiscard]] virtual bool isEmpty() const = 0;
  [[nodiscard]] virtual std::unique_ptr<Geometry> copy() const = 0;
  [[nodiscard]] virtual size_t numFacets() const = 0;
  [[nodiscard]] unsigned int getConvexity() const { return convexity; }
  void setConvexity(int c) { this->convexity = c; }
  virtual void setColor(const Color4f& c) {}

  virtual void transform(const Transform3d& /*mat*/) { assert(!"transform not implemented!"); }
  virtual void resize(const Vector3d& /*newsize*/, const Eigen::Matrix<bool, 3, 1>& /*autosize*/) {
    assert(!"resize not implemented!");
  }

  virtual void accept(GeometryVisitor& visitor) const = 0;
protected:
  int convexity{1};
};

/**
 * A Base class for simple visitors to process different Geometry subclasses uniformly
 */
class GeometryVisitor
{
public:
  virtual void visit(const GeometryList& node) = 0;
  virtual void visit(const PolySet& node) = 0;
  virtual void visit(const Polygon2d& node) = 0;
#ifdef ENABLE_CGAL
  virtual void visit(const CGAL_Nef_polyhedron& node) = 0;
  virtual void visit(const CGALHybridPolyhedron& node) = 0;
#endif
#ifdef ENABLE_MANIFOLD
  virtual void visit(const ManifoldGeometry& node) = 0;
#endif
  virtual ~GeometryVisitor() = default;
};

#define VISITABLE_GEOMETRY() \
        void accept(GeometryVisitor &visitor) const override { \
          visitor.visit(*this); \
        }

class GeometryList : public Geometry
{
public:
  VISITABLE_GEOMETRY();
  Geometries children;

  GeometryList();
  GeometryList(Geometry::Geometries geometries);

  [[nodiscard]] size_t memsize() const override;
  [[nodiscard]] BoundingBox getBoundingBox() const override;
  [[nodiscard]] std::string dump() const override;
  [[nodiscard]] unsigned int getDimension() const override;
  [[nodiscard]] bool isEmpty() const override;
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override;
  [[nodiscard]] size_t numFacets() const override { assert(false && "not implemented"); return 0; }

  [[nodiscard]] const Geometries& getChildren() const {
    return this->children;
  }

  [[nodiscard]] Geometries flatten() const;

};
