#pragma once

#include <cassert>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "core/BaseVisitable.h"
#include "core/NodeVisitor.h"
#include "core/enums.h"
#include "core/node.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"

class CGALNefGeometry;
class Polygon2d;
class Tree;

class EdgeKey
{
public:
  EdgeKey()
  {
    this->ind1 = -1;
    this->ind2 = -1;
  }
  EdgeKey(int i1, int i2);
  int ind1, ind2;
  int operator==(const EdgeKey ref)
  {
    if (this->ind1 == ref.ind1 && this->ind2 == ref.ind2) return 1;
    return 0;
  }
};

unsigned int hash_value(const EdgeKey& r);
int operator==(const EdgeKey& t1, const EdgeKey& t2);

struct EdgeVal {
  int sel;
  int facea, posa;  // face a with edge ind1 -> ind2, posa = index of ind1 within facea
  int faceb, posb;  // face b with edge ind2 -> ind1, posb = index of ind2 within faceb
  IndexedFace bez1;
  IndexedFace bez2;
  double angle;
};

class PolySetBuilder;
std::vector<std::vector<IndexedColorTriangle>> wrapSlice(PolySetBuilder& builder,
                                                         const std::vector<Vector3d> vertices,
                                                         const std::vector<IndexedColorFace>& faces,
                                                         const std::vector<Vector4d>& normals,
                                                         std::vector<double> xsteps);

// 3D Map stuff
//
#define BUCKET 8

class Map3DTree
{
public:
  Map3DTree(void);
  int ind[8];  // 8 octants, intially -1
  Vector3d pts[BUCKET];
  int ptsind[BUCKET];
  int ptlen;
};

class Map3D
{
public:
  Map3D(Vector3d min, Vector3d max);
  void add(Vector3d pt, int ind);
  void del(Vector3d pt);
  int find(Vector3d pt, double r, std::vector<Vector3d>& result, std::vector<int>& resultind,
           int maxresult);
  void dump_hier(int ind, int hier, float minx, float miny, float minz, float maxx, float maxy,
                 float maxz);
  void dump();

private:
  void add_sub(int ind, Vector3d min, Vector3d max, Vector3d pt, int ptind, int disable_local_num);
  void find_sub(int ind, double minx, double miny, double minz, double maxx, double maxy, double maxz,
                Vector3d pt, double r, std::vector<Vector3d>& result, std::vector<int>& resultind,
                int maxresult);
  Vector3d min, max;
  std::vector<Map3DTree> items;
};

int cut_face_face_face(Vector3d p1, Vector3d n1, Vector3d p2, Vector3d n2, Vector3d p3, Vector3d n3,
                       Vector3d& res, double *detptr = NULL);
int cut_face_line(Vector3d fp, Vector3d fn, Vector3d lp, Vector3d ld, Vector3d& res,
                  double *detptr = NULL);
bool pointInPolygon(const std::vector<Vector3d>& vert, const IndexedFace& bnd, int ptind);
Vector4d calcTriangleNormal(const std::vector<Vector3d>& vertices, const IndexedFace& pol);
std::vector<Vector4d> calcTriangleNormals(const std::vector<Vector3d>& vertices,
                                          const std::vector<IndexedFace>& indices);
std::vector<IndexedFace> mergeTriangles(const std::vector<IndexedFace> polygons,
                                        const std::vector<Vector4d> normals,
                                        std::vector<Vector4d>& newNormals, std::vector<int>& faceParents,
                                        const std::vector<Vector3d>& vert);
std::vector<IndexedColorFace> mergeTriangles(const std::vector<IndexedColorFace> polygons,
                                             const std::vector<Vector4d> normals,
                                             std::vector<Vector4d>& newNormals,
                                             std::vector<int>& faceParents,
                                             const std::vector<Vector3d>& vert);
std::unordered_map<EdgeKey, EdgeVal, boost::hash<EdgeKey>> createEdgeDb(
  const std::vector<IndexedFace>& indices);

VectorOfVector2d alterprofile(VectorOfVector2d vertices, double scalex, double scaley, double origin_x,
                              double origin_y, double offset_x, double offset_y, double rot);
// This evaluates a node tree into concrete geometry usign an underlying geometry engine
// FIXME: Ideally, each engine should implement its own subtype. Instead we currently have
// multiple embedded engines with varoius methods of selecting the right one.
std::unique_ptr<Geometry> union_geoms(std::vector<std::shared_ptr<PolySet>> parts);
std::unique_ptr<Geometry> difference_geoms(std::vector<std::shared_ptr<PolySet>> parts);

class GeometryEvaluator : public NodeVisitor
{
public:
  GeometryEvaluator(const Tree& tree);

  std::shared_ptr<const Geometry> evaluateGeometry(const AbstractNode& node, bool allownef);

  Response visit(State& state, const AbstractNode& node) override;
  Response visit(State& state, const ColorNode& node) override;
  Response visit(State& state, const AbstractIntersectionNode& node) override;
  Response visit(State& state, const AbstractPolyNode& node) override;
  Response visit(State& state, const SkinNode& node) override;
  Response visit(State& state, const ConcatNode& node) override;
  Response visit(State& state, const LinearExtrudeNode& node) override;
  Response visit(State& state, const PathExtrudeNode& node) override;
  Response visit(State& state, const RotateExtrudeNode& node) override;
  Response visit(State& state, const PullNode& node) override;
  Response visit(State& state, const DebugNode& node) override;
  Response visit(State& state, const RepairNode& node) override;
  Response visit(State& state, const WrapNode& node) override;
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
    template <class T>
    static ResultObject constResult(std::shared_ptr<const T> geom)
    {
      return {geom};
    }
    template <class T>
    static ResultObject mutableResult(std::shared_ptr<T> geom)
    {
      return {geom};
    }

    // Default constructor with nullptr can be used to represent empty geometry,
    // for example union() with no children, etc.
    ResultObject() : is_const(true) {}
    std::shared_ptr<Geometry> ptr()
    {
      assert(!is_const);
      return pointer;
    }
    [[nodiscard]] std::shared_ptr<const Geometry> constptr() const
    {
      return is_const ? const_pointer : std::static_pointer_cast<const Geometry>(pointer);
    }
    std::shared_ptr<Geometry> asMutableGeometry()
    {
      if (is_const) return {constptr() ? constptr()->copy() : nullptr};
      else return ptr();
    }

  private:
    template <class T>
    ResultObject(std::shared_ptr<const T> g) : is_const(true), const_pointer(std::move(g))
    {
    }
    template <class T>
    ResultObject(std::shared_ptr<T> g) : is_const(false), pointer(std::move(g))
    {
    }

    bool is_const;
    std::shared_ptr<Geometry> pointer;
    std::shared_ptr<const Geometry> const_pointer;
  };

  void smartCacheInsert(const AbstractNode& node, const std::shared_ptr<const Geometry>& geom);
  std::shared_ptr<const Geometry> smartCacheGet(const AbstractNode& node, bool preferNef);
  bool isSmartCached(const AbstractNode& node);
  bool isValidDim(const Geometry::GeometryItem& item, unsigned int& dim) const;
  std::vector<std::shared_ptr<const Barcode1d>> collectChildren1D(const AbstractNode& node);
  std::vector<std::shared_ptr<const Polygon2d>> collectChildren2D(const AbstractNode& node);
  Geometry::Geometries collectChildren3D(const AbstractNode& node);
  std::unique_ptr<Polygon2d> applyMinkowski2D(const AbstractNode& node);
  std::unique_ptr<Polygon2d> applyHull2D(const AbstractNode& node);
  std::unique_ptr<Polygon2d> applyFill2D(const AbstractNode& node);
  std::unique_ptr<Geometry> applyHull3D(const AbstractNode& node);
  void applyResize3D(CGALNefGeometry& N, const Vector3d& newsize,
                     const Eigen::Matrix<bool, 3, 1>& autosize);
  std::unique_ptr<Barcode1d> applyToChildren1D(const AbstractNode& node, OpenSCADOperator op);
  std::unique_ptr<Polygon2d> applyToChildren2D(const AbstractNode& node, OpenSCADOperator op);
  ResultObject applyToChildren3D(const AbstractNode& node, OpenSCADOperator op);
  ResultObject applyToChildren(const AbstractNode& node, OpenSCADOperator op);
  std::shared_ptr<const Geometry> projectionCut(const ProjectionNode& node);
  std::shared_ptr<const Geometry> projectionNoCut(const ProjectionNode& node);

  void addToParent(const State& state, const AbstractNode& node,
                   const std::shared_ptr<const Geometry>& geom);
  Response lazyEvaluateRootNode(State& state, const AbstractNode& node);

  std::map<int, Geometry::Geometries> visitedchildren;
  const Tree& tree;
  std::shared_ptr<const Geometry> root;

public:
};
