#pragma once

#include <stddef.h>
#include <string>
#include <list>

#include "linalg.h"
#include "memory.h"

class GeometryVisitor;

class Geometry
{
public:
  typedef std::pair<const class AbstractNode *, shared_ptr<const Geometry>> GeometryItem;
  typedef std::list<GeometryItem> Geometries;

  Geometry() : convexity(1) {}
  virtual ~Geometry() {}

  virtual size_t memsize() const = 0;
  virtual BoundingBox getBoundingBox() const = 0;
  virtual std::string dump() const = 0;
  virtual unsigned int getDimension() const = 0;
  virtual bool isEmpty() const = 0;
  virtual Geometry *copy() const = 0;
  virtual size_t numFacets() const = 0;

  unsigned int getConvexity() const { return convexity; }
  void setConvexity(int c) { this->convexity = c; }

  virtual void transform(const Transform3d& mat) { assert(!"transform not implemented!"); }
  virtual void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) { assert(!"resize not implemented!"); }

  virtual void accept(class GeometryVisitor& visitor) const = 0;
protected:
  int convexity;
};

/**
 * A Base clss for simple visitors to process different Geometry subclasses uniformly
 */
class GeometryVisitor
{
public:
  virtual void visit(const class GeometryList& node) = 0;
  virtual void visit(const class PolySet& node) = 0;
  virtual void visit(const class Polygon2d& node) = 0;
#ifdef ENABLE_CGAL
  virtual void visit(const class CGAL_Nef_polyhedron& node) = 0;
  virtual void visit(const class CGALHybridPolyhedron& node) = 0;
#endif
  virtual ~GeometryVisitor(){}
};

#define VISITABLE_GEOMETRY() \
  void accept(class GeometryVisitor& visitor) const override { \
    visitor.visit(*this); \
  }

class GeometryList : public Geometry
{
public:
  VISITABLE_GEOMETRY();
  Geometries children;

  GeometryList();
  GeometryList(const Geometry::Geometries& geometries);
  virtual ~GeometryList();

  size_t memsize() const override;
  BoundingBox getBoundingBox() const override;
  std::string dump() const override;
  unsigned int getDimension() const override;
  bool isEmpty() const override;
  Geometry *copy() const override { return new GeometryList(*this); }
  size_t numFacets() const override { assert(false && "not implemented"); return 0; }

  const Geometries& getChildren() const {
    return this->children;
  }

  Geometries flatten() const;

};
