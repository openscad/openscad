#include "geometry/GeometryEvaluator.h"

#include <cassert>
#include <cmath>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "Feature.h"
#include "core/BaseVisitable.h"
#include "core/CgalAdvNode.h"
#include "core/ColorNode.h"
#include "core/CsgOpNode.h"
#include "core/CurveDiscretizer.h"
#include "core/LinearExtrudeNode.h"
#include "core/ModuleInstantiation.h"
#include "core/OffsetNode.h"
#include "core/ProjectionNode.h"
#include "core/RenderNode.h"
#include "core/RoofNode.h"
#include "core/RotateExtrudeNode.h"
#include "core/SkinNode.h"
#include "core/ConcatNode.h"
#include "core/PathExtrudeNode.h"
#include "core/PullNode.h"
#include "core/DebugNode.h"
#include "core/RepairNode.h"
#include "core/WrapNode.h"
#include "glview/ColorMap.h"
#include "geometry/Barcode1d.h"
#include "core/State.h"
#include "core/TextNode.h"
#include "core/TransformNode.h"
#include "core/Tree.h"
#include "core/enums.h"
#include "core/node.h"
#include "geometry/ClipperUtils.h"
#include "geometry/Geometry.h"
#include "geometry/GeometryCache.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "geometry/Polygon2d.h"
#include "geometry/boolean_utils.h"
#include "geometry/cgal/cgal.h"
#include "geometry/linalg.h"
#include "geometry/linear_extrude.h"
#include "geometry/roof_ss.h"
#include "geometry/roof_vd.h"
#include "geometry/rotate_extrude.h"
#include "glview/RenderSettings.h"
#include "utils/calc.h"
#include "utils/degree_trig.h"
#include "utils/printutils.h"
#ifdef ENABLE_CGAL
#include <CGAL/Point_2.h>
#include <CGAL/convex_hull_2.h>

#include "geometry/cgal/CGALCache.h"
#include "geometry/cgal/cgalutils.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/manifoldutils.h"
#endif
#include "geometry/skin.h"

#ifdef ENABLE_PYTHON
#include <src/python/python_public.h>
#endif
#include <vector>

class Geometry;
class Polygon2d;
class Tree;

GeometryEvaluator::GeometryEvaluator(const Tree& tree) : tree(tree)
{
}

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
void vectorDump(const char *msg, const Vector3d& vec)
{
  printf("%s ", msg);
  printf("(%g/%g/%g) ", vec[0], vec[1], vec[2]);
}

void triangleDump(const char *msg, const IndexedFace& face, const std::vector<Vector3d>& vert)
{
  printf("%s ", msg);
  for (size_t i = 0; i < face.size(); i++) {
    const Vector3d& pt = vert[face[i]];
    vectorDump(" ", pt);
  }
}

Vector4d calcTriangleNormal(const std::vector<Vector3d>& vertices, const IndexedFace& pol)
{
  int n = pol.size();
  assert(pol.size() >= 3);
  Vector3d norm(0, 0, 0);
  for (int j = 0; j < n - 2; j++) {
    // need to calculate all normals, as 1st 2 could be in a concave corner
    Vector3d diff1 = (vertices[pol[0]] - vertices[pol[j + 1]]);
    Vector3d diff2 = (vertices[pol[j + 1]] - vertices[pol[j + 2]]);
    norm += diff1.cross(diff2);
  }
  norm.normalize();
  Vector3d pt = vertices[pol[0]];
  double off = norm.dot(pt);
  return Vector4d(norm[0], norm[1], norm[2], off);
}

std::vector<Vector4d> calcTriangleNormals(const std::vector<Vector3d>& vertices,
                                          const std::vector<IndexedFace>& indices)
{
  std::vector<Vector4d> faceNormal;
  for (unsigned int i = 0; i < indices.size(); i++) {
    IndexedFace pol = indices[i];
    assert(pol.size() >= 3);
    faceNormal.push_back(calcTriangleNormal(vertices, pol));
  }
  return faceNormal;
}

bool pointInPolygon(const std::vector<Vector3d>& vert, const IndexedFace& bnd, int ptind)
{
  int i, n;
  n = bnd.size();
  int cuts = 0;
  Vector3d p1, p2;
  Vector3d pt = vert[ptind];
  Vector3d res;
  if (n < 3) return false;
  Vector3d raydir = vert[bnd[1]] - vert[bnd[0]];
  Vector3d raydir2 = vert[bnd[1]] - vert[bnd[2]];
  Vector3d fn = raydir.cross(raydir2).normalized();
  // check, how many times the ray crosses the 3D fence, classical algorithm in 3D
  raydir = (raydir * 0.371 + raydir2 * 0.712).normalized();
  for (i = 0; i < n; i++) {
    // build fence side
    const Vector3d& p1 = vert[bnd[i]];
    const Vector3d& p2 = vert[bnd[(i + 1) % n]];

    if (linsystem(p2 - p1, raydir, fn, pt - p1, res, nullptr)) continue;

    if (res[1] > 0) continue;  // not behind
    if (res[0] < 0) continue;  // within segment
    if (res[0] > 1) continue;
    cuts++;
  }
  return (cuts & 1) ? true : false;
}

unsigned int hash_value(const EdgeKey& r)
{
  unsigned int i;
  i = r.ind1 | (r.ind2 << 16);
  return i;
}
EdgeKey::EdgeKey(int i1, int i2)
{
  this->ind1 = i1 < i2 ? i1 : i2;
  this->ind2 = i2 > i1 ? i2 : i1;
}

int operator==(const EdgeKey& t1, const EdgeKey& t2)
{
  if (t1.ind1 == t2.ind1 && t1.ind2 == t2.ind2) return 1;
  return 0;
}

std::unordered_map<EdgeKey, EdgeVal, boost::hash<EdgeKey>> createEdgeDb(
  const std::vector<IndexedFace>& indices)
{
  std::unordered_map<EdgeKey, EdgeVal, boost::hash<EdgeKey>> edge_db;
  EdgeKey edge;
  //
  // Create EDGE DB
  EdgeVal val;
  val.sel = 0;
  val.facea = -1;
  val.faceb = -1;
  val.posa = -1;
  val.posb = -1;
  int ind1, ind2;
  for (size_t i = 0; i < indices.size(); i++) {
    int n = indices[i].size();
    for (int j = 0; j < n; j++) {
      ind1 = indices[i][j];
      ind2 = indices[i][(j + 1) % n];
      if (ind2 > ind1) {
        edge.ind1 = ind1;
        edge.ind2 = ind2;
        if (edge_db.count(edge) == 0) edge_db[edge] = val;
        edge_db[edge].facea = i;
        edge_db[edge].posa = j;
      } else {
        edge.ind1 = ind2;
        edge.ind2 = ind1;
        if (edge_db.count(edge) == 0) edge_db[edge] = val;
        edge_db[edge].faceb = i;
        edge_db[edge].posb = j;
      }
    }
  }
  int error = 0;
  for (auto& e : edge_db) {
    if (e.second.facea == -1 || e.second.faceb == -1) {
      printf("Mismatched EdgeDB ind1=%d idn2=%d facea=%d faceb=%d\n", e.first.ind1, e.first.ind2,
             e.second.facea, e.second.faceb);
      error = 1;
    }
  }
  if (error) {
    for (unsigned int i = 0; i < indices.size(); i++) {
      auto& face = indices[i];
      printf("%d :", i);
      for (unsigned int j = 0; j < face.size(); j++) printf("%d ", face[j]);
      printf("\n");
    }  // tri 5-9-11 missing
    assert(0);
  }
  return edge_db;
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

typedef std::vector<IndexedFace> indexedFaceList;

bool mergeTrianglesOpposite(const IndexedFace& poly1, const IndexedFace& poly2)
{
  if (poly1.size() != poly2.size()) return false;
  int n = poly1.size();
  int off = -1;
  for (int i = 0; off == -1 && i < n; i++)
    if (poly1[0] == poly2[i]) off = i;
  if (off == -1) return false;

  for (int i = 1; i < n; i++)
    if (poly1[i] != poly2[(off + n - i) % n]) return false;

  return true;
}

typedef std::vector<size_t> intList;
typedef std::vector<intList> intListList;

class TriCombineStub
{
public:
  int ind1, ind2, ind3;
  int operator==(const TriCombineStub ref)
  {
    if (this->ind1 == ref.ind1 && this->ind2 == ref.ind2) return 1;
    return 0;
  }
};

unsigned int hash_value(const TriCombineStub& r)
{
  unsigned int i;
  i = r.ind1 | (r.ind2 << 16);
  return i;
}

int operator==(const TriCombineStub& t1, const TriCombineStub& t2)
{
  if (t1.ind1 == t2.ind1 && t1.ind2 == t2.ind2) return 1;
  return 0;
}

static indexedFaceList mergeTrianglesSub(const std::vector<IndexedFace>& triangles,
                                         const std::vector<Vector3d>& vert)
{
  unsigned int i, j, n;
  int ind1, ind2;
  std::unordered_set<TriCombineStub, boost::hash<TriCombineStub>> stubs_pos;
  std::unordered_set<TriCombineStub, boost::hash<TriCombineStub>> stubs_neg;

  TriCombineStub e;
  for (i = 0; i < triangles.size(); i++) {
    n = triangles[i].size();
    for (j = 0; j < n; j++) {
      ind1 = triangles[i][j];
      ind2 = triangles[i][(j + 1) % n];
      if (ind2 > ind1)  // positive edge
      {
        e.ind1 = ind1;
        e.ind2 = ind2;
        if (stubs_neg.find(e) != stubs_neg.end()) stubs_neg.erase(e);
        else if (stubs_pos.find(e) != stubs_pos.end()) printf("Duplicate Edge %d->%d \n", ind1, ind2);
        else stubs_pos.insert(e);
      }
      if (ind2 < ind1)  // negative edge
      {
        e.ind1 = ind2;
        e.ind2 = ind1;
        if (stubs_pos.find(e) != stubs_pos.end()) stubs_pos.erase(e);
        else if (stubs_neg.find(e) != stubs_neg.end()) printf("Duplicate Edge %d->%d \n", ind2, ind1);
        else stubs_neg.insert(e);
      }
    }
  }

  // now chain everything
  std::unordered_map<int, int> stubs_chain;
  std::vector<TriCombineStub> stubs;
  std::vector<TriCombineStub> stubs_bak;
  for (const auto& stubs : stubs_pos) {
    if (stubs_chain.count(stubs.ind1) > 0) {
      stubs_bak.push_back(stubs);
    } else stubs_chain[stubs.ind1] = stubs.ind2;
  }

  for (const auto& stubs : stubs_neg) {
    if (stubs_chain.count(stubs.ind2) > 0) {
      TriCombineStub ts;
      ts.ind1 = stubs.ind2;
      ts.ind2 = stubs.ind1;
      stubs_bak.push_back(ts);
    } else stubs_chain[stubs.ind2] = stubs.ind1;
  }
  std::vector<IndexedFace> result;

  while (stubs_chain.size() > 0) {
    int ind, ind_new;
    auto [ind_start, dummy] = *(stubs_chain.begin());
    ind = ind_start;
    IndexedFace poly;
    while (1) {
      if (stubs_chain.count(ind) > 0) {
        ind_new = stubs_chain[ind];
        stubs_chain.erase(ind);
      } else {
        ind_new = -1;
        for (i = 0; ind_new == -1 && i < stubs_bak.size(); i++) {
          if (stubs_bak[i].ind1 == ind) {
            ind_new = stubs_bak[i].ind2;
            std::vector<TriCombineStub>::iterator it = stubs_bak.begin();
            std::advance(it, i);
            stubs_bak.erase(it);
            break;
          }
        }
        if (ind_new == -1) break;
      }
      poly.push_back(ind_new);
      if (ind_new == ind_start) break;  // chain closed
      ind = ind_new;
    }

    // spitz-an-spitz loesen, in einzelketten trennen
    unsigned int beg, end, begbest;
    int dist, distbest, repeat;
    do {
      repeat = 0;
      std::unordered_map<int, int> value_pos;
      distbest = -1;
      n = poly.size();
      for (i = 0; i < n; i++) {
        ind = poly[i];
        if (value_pos.count(ind) > 0) {
          beg = value_pos[ind];
          end = i;
          dist = end - beg;
          std::unordered_map<int, int> value_pos2;
          int doubles = 0;
          for (j = beg; j < end; j++) {
            if (value_pos2.count(poly[j]) > 0) doubles = 1;
          }

          if (dist > distbest &&
              doubles == 0)  // es duerfen sich keine doppelten zahlen drinnen befinden
          {
            distbest = dist;
            begbest = beg;
          }
          if (end - beg == 2) {
            for (int i = 0; i < 2; i++) {  // TODO make more efficient
              auto it = poly.begin();
              std::advance(it, beg);
              poly.erase(it);
            }
            repeat = 1;
            distbest = -1;
            break;
          }
          if (beg + n - end == 2) {
            IndexedFace polynew;
            for (j = beg; j < end; j++) polynew.push_back(poly[j]);
            poly = polynew;
            repeat = 1;
            distbest = -1;
            break;
          }
        }
        value_pos[ind] = i;
      }
      if (distbest != -1) {
        IndexedFace polynew;
        for (i = begbest; i < begbest + distbest; i++) {
          polynew.push_back(poly[i]);
        }
        if (polynew.size() >= 3) result.push_back(polynew);
        for (int i = 0; i < distbest; i++) {  // TODO make more efficient
          auto it = poly.begin();
          std::advance(it, begbest);
          poly.erase(it);
        }
        repeat = 1;
      }
    } while (repeat);
    if (vert.size() != 0) {
      // Reduce colinear points
      int n = poly.size();
      IndexedFace poly_new;
      int cur = poly[0], next;
      for (int i = 0; i < n; i++) {
        next = poly[(i + 1) % n];
        if (1) {  // (p2-p1).cross(p1-p0).norm() > 0.00001) {
                  // TODO enable again, need partner also to remove
          poly_new.push_back(cur);
          cur = next;
        } else {
          cur = next;
        }
      }

      if (poly_new.size() > 2) result.push_back(poly_new);
    } else result.push_back(poly);
  }
  return result;
}

std::vector<IndexedFace> mergeTriangles(const std::vector<IndexedFace> polygons,
                                        const std::vector<Vector4d> normals,
                                        std::vector<Vector4d>& newNormals, std::vector<int>& faceParents,
                                        const std::vector<Vector3d>& vert)
{
  indexedFaceList emptyList;
  std::vector<Vector4d> norm_list;
  std::vector<indexedFaceList> polygons_sorted;
  // sort polygons into buckets of same orientation
  for (unsigned int i = 0; i < polygons.size(); i++) {
    Vector4d norm = normals[i];
    const IndexedFace& triangle = polygons[i];

    int norm_ind = -1;
    for (unsigned int j = 0; norm_ind == -1 && j < norm_list.size(); j++) {
      const auto& cur = norm_list[j];
      if (cur.head<3>().dot(norm.head<3>()) > 0.99999 && fabs(cur[3] - norm[3]) < 0.001) {
        norm_ind = j;
      }
      if (cur.norm() < 1e-6 && norm.norm() < 1e-6) norm_ind = j;  // zero vector matches zero vector
    }
    if (norm_ind == -1) {
      norm_ind = norm_list.size();
      norm_list.push_back(norm);
      polygons_sorted.push_back(emptyList);
    }
    polygons_sorted[norm_ind].push_back(triangle);
  }

  //  now put back hole of polygons into the correct bucket
  for (unsigned int i = 0; i < polygons_sorted.size(); i++) {
    // check if bucket has an opposite oriented bucket
    // Vector4d n = norm_list[i];
    int i_ = -1;
    Vector3d nref = norm_list[i].head<3>();
    for (unsigned int j = 0; j < polygons_sorted.size(); j++) {
      if (j == i) continue;
      if (norm_list[j].head<3>().dot(nref) < -0.999 && fabs(norm_list[j][3] + norm_list[i][3]) < 0.005) {
        i_ = j;
        break;
      }
    }
    if (i_ == -1) continue;
    // assuming that i_ contains the holes, find, it there is a match

    for (unsigned int k = 0; k < polygons_sorted[i].size(); k++) {
      IndexedFace poly = polygons_sorted[i][k];
      for (unsigned int l = 0; l < polygons_sorted[i_].size(); l++) {
        IndexedFace hole = polygons_sorted[i_][l];
        //				// holes dont intersect with the polygon, so its sufficent to
        // check, if one point of the hole is inside the polygon
        if (mergeTrianglesOpposite(poly, hole)) {
          polygons_sorted[i].erase(polygons_sorted[i].begin() + k);
          polygons_sorted[i_].erase(polygons_sorted[i_].begin() + l);
          k--;
          l--;
        } else if (pointInPolygon(vert, poly, hole[0])) {
          polygons_sorted[i].push_back(hole);
          polygons_sorted[i_].erase(polygons_sorted[i_].begin() + l);
          l--;  // could have more holes
        }
      }
    }
  }

  // now merge the polygons in all buckets independly
  std::vector<IndexedFace> indices;
  newNormals.clear();
  faceParents.clear();
  for (unsigned int i = 0; i < polygons_sorted.size(); i++) {
    indexedFaceList indices_sub = mergeTrianglesSub(polygons_sorted[i], vert);
    int off = indices.size();
    for (unsigned int j = 0; j < indices_sub.size(); j++) {
      indices.push_back(indices_sub[j]);
      newNormals.push_back(norm_list[i]);
      Vector4d loc_norm = calcTriangleNormal(vert, indices_sub[j]);
      if (norm_list[i].head<3>().dot(loc_norm.head<3>()) > 0) faceParents.push_back(-1);
      else {
        int par = -1;
        for (unsigned int k = 0; k < indices_sub.size(); k++) {
          if (k == j) continue;
          if (pointInPolygon(vert, indices_sub[k], indices_sub[j][0])) {
            par = k;
          }
        }
        if (par != -1) faceParents.push_back(par + off);
        else {
          faceParents.push_back(-1);
          printf("par not found here 1\n");
        }
      }
    }
  }

  return indices;
}

void export_debug_polyset(const PolySet& ps);

std::vector<IndexedColorFace> mergeTriangles(const std::vector<IndexedColorFace> polygons,
                                             const std::vector<Vector4d> normals,
                                             std::vector<Vector4d>& newNormals,
                                             std::vector<int>& faceParents,
                                             const std::vector<Vector3d>& vert)
{
  indexedFaceList emptyList;
  std::vector<Vector4d> norm_list;
  std::vector<int> color_list;
  std::vector<indexedFaceList> polygons_sorted;
  // sort polygons into buckets of same orientation and color
  for (unsigned int i = 0; i < polygons.size(); i++) {
    Vector4d norm = normals[i];
    const IndexedColorFace& triangle = polygons[i];

    int bucket_ind = -1;  // TODO fix name
    for (unsigned int j = 0; bucket_ind == -1 && j < norm_list.size(); j++) {
      if (polygons[i].color == color_list[j]) {
        const auto& cur_norm = norm_list[j];
        if (cur_norm.head<3>().dot(norm.head<3>()) > 0.99999 && fabs(cur_norm[3] - norm[3]) < 0.001) {
          bucket_ind = j;
        }
        if (cur_norm.norm() < 1e-6 && norm.norm() < 1e-6)
          bucket_ind = j;  // zero vector matches zero vector
      }
    }
    if (bucket_ind == -1) {
      bucket_ind = norm_list.size();
      norm_list.push_back(norm);
      color_list.push_back(polygons[i].color);
      polygons_sorted.push_back(emptyList);
    }
    polygons_sorted[bucket_ind].push_back(triangle.face);
  }

  //  now put back hole of polygons into the correct bucket
  for (unsigned int i = 0; i < polygons_sorted.size(); i++) {
    // check if bucket has an opposite oriented bucket
    // Vector4d n = norm_list[i];
    int i_ = -1;
    Vector3d nref = norm_list[i].head<3>();
    for (unsigned int j = 0; j < polygons_sorted.size(); j++) {
      if (j == i) continue;
      if (norm_list[j].head<3>().dot(nref) < -0.999 && fabs(norm_list[j][3] + norm_list[i][3]) < 0.005) {
        i_ = j;
        break;
      }
    }
    if (i_ == -1) continue;
    // assuming that i_ contains the holes, find, it there is a match

    for (unsigned int k = 0; k < polygons_sorted[i].size(); k++) {
      IndexedFace poly = polygons_sorted[i][k];
      for (unsigned int l = 0; l < polygons_sorted[i_].size(); l++) {
        IndexedFace hole = polygons_sorted[i_][l];
        //				// holes dont intersect with the polygon, so its sufficent to
        // check, if one point of the hole is inside the polygon
        if (mergeTrianglesOpposite(poly, hole)) {
          polygons_sorted[i].erase(polygons_sorted[i].begin() + k);
          polygons_sorted[i_].erase(polygons_sorted[i_].begin() + l);
          k--;
          l--;
        } else if (pointInPolygon(vert, poly, hole[0])) {
          polygons_sorted[i].push_back(hole);
          polygons_sorted[i_].erase(polygons_sorted[i_].begin() + l);
          l--;  // could have more holes
        }
      }
    }
  }

  // now merge the polygons in all buckets independly
  std::vector<IndexedColorFace> indices;
  newNormals.clear();
  faceParents.clear();
  for (unsigned int i = 0; i < polygons_sorted.size(); i++) {
    indexedFaceList indices_sub = mergeTrianglesSub(polygons_sorted[i], vert);
    int off = indices.size();
    for (unsigned int j = 0; j < indices_sub.size(); j++) {
      indices.push_back(IndexedColorFace({.face = indices_sub[j], .color = color_list[i]}));
      newNormals.push_back(norm_list[i]);
      Vector4d loc_norm = calcTriangleNormal(vert, indices_sub[j]);
      if (norm_list[i].head<3>().dot(loc_norm.head<3>()) > 0) faceParents.push_back(-1);
      else {
        int par = -1;
        for (unsigned int k = 0; k < indices_sub.size(); k++) {
          if (k == j) continue;
          if (pointInPolygon(vert, indices_sub[k], indices_sub[j][0])) {
            par = k;
          }
        }
        if (par == -1) printf("par not found here\n");
        // assert(par != -1); TODO fix
        faceParents.push_back(par + off);
      }
    }
  }

  return indices;
}

Map3DTree::Map3DTree(void)
{
  for (int i = 0; i < 8; ind[i++] = -1);
  ptlen = 0;
}

Map3D::Map3D(Vector3d min, Vector3d max)
{
  this->min = min;
  this->max = max;
}
void Map3D::add_sub(int ind, Vector3d min, Vector3d max, Vector3d pt, int ptind, int disable_local_num)
{
  int indnew;
  int corner;
  Vector3d mid;
  do {
    if (items[ind].ptlen >= 0 && disable_local_num != ind) {
      if (items[ind].ptlen < BUCKET) {
        for (int i = 0; i < items[ind].ptlen; i++)
          if (items[ind].pts[i] == pt) return;
        items[ind].pts[items[ind].ptlen] = pt;
        items[ind].ptsind[items[ind].ptlen] = ptind;
        items[ind].ptlen++;
        return;
      } else {
        for (int i = 0; i < items[ind].ptlen; i++) {
          add_sub(ind, min, max, items[ind].pts[i], items[ind].ptsind[i], ind);
        }
        items[ind].ptlen = -1;
        // run through
      }
    }
    mid[0] = (min[0] + max[0]) / 2.0;
    mid[1] = (min[1] + max[1]) / 2.0;
    mid[2] = (min[2] + max[2]) / 2.0;
    corner = (pt[0] >= mid[0] ? 1 : 0) + (pt[1] >= mid[1] ? 2 : 0) + (pt[2] >= mid[2] ? 4 : 0);
    indnew = items[ind].ind[corner];
    if (indnew == -1) {
      indnew = items.size();
      items.push_back(Map3DTree());
      items[ind].ind[corner] = indnew;
    }
    if (corner & 1) min[0] = mid[0];
    else max[0] = mid[0];
    if (corner & 2) min[1] = mid[1];
    else max[1] = mid[1];
    if (corner & 4) min[2] = mid[2];
    else max[2] = mid[2];
    ind = indnew;
  } while (1);
}
void Map3D::add(Vector3d pt, int ind)
{
  if (items.size() == 0) {
    items.push_back(Map3DTree());
    items[0].pts[0] = pt;
    items[0].ptsind[0] = ind;
    items[0].ptlen++;
    return;
  }
  add_sub(0, this->min, this->max, pt, ind, -1);
}

void Map3D::del(Vector3d pt)
{
  int ind = 0;
  int corner;
  Vector3d min = this->min;
  Vector3d max = this->max;
  Vector3d mid;
  printf("Deleting %g/%g/%g\n", pt[0], pt[1], pt[2]);
  while (ind != -1) {
    for (int i = 0; i < items[ind].ptlen; i++) {
      if (items[ind].pts[i] == pt) {
        for (int j = i + 1; j < items[ind].ptlen; j++) items[ind].pts[j - 1] = items[ind].pts[j];
        items[ind].ptlen--;
        return;
      }
      // was wenn leer wird dnn sind ind immer noch -1
    }
    mid[0] = (min[0] + max[0]) / 2.0;
    mid[1] = (min[1] + max[1]) / 2.0;
    mid[2] = (min[2] + max[2]) / 2.0;
    corner = (pt[0] > mid[0] ? 1 : 0) + (pt[1] > mid[1] ? 2 : 0) + (pt[2] > mid[2] ? 4 : 0);
    printf("corner=%d\n", corner);
    ind = items[ind].ind[corner];
    if (corner & 1) min[0] = mid[0];
    else max[0] = mid[0];
    if (corner & 2) min[1] = mid[1];
    else max[1] = mid[1];
    if (corner & 4) min[2] = mid[2];
    else max[2] = mid[2];
  }
}

void Map3D::find_sub(int ind, double minx, double miny, double minz, double maxx, double maxy,
                     double maxz, Vector3d pt, double r, std::vector<Vector3d>& result,
                     std::vector<int>& resultind, int maxresult)
{
  if (ind == -1) return;
  if (this->items[ind].ptlen > 0) {
    for (int i = 0; i < this->items[ind].ptlen; i++) {
      if ((this->items[ind].pts[i] - pt).norm() < r) {
        result.push_back(this->items[ind].pts[i]);
        resultind.push_back(this->items[ind].ptsind[i]);
      }
      if (result.size() >= (size_t)maxresult) return;
    }
    return;
  }
  double midx, midy, midz;
  //	printf("find_sub ind=%d %g/%g/%g - %g/%g/%g\n",ind, minx, miny,  minz, maxx, maxy, maxz );
  midx = (minx + maxx) / 2.0;
  midy = (miny + maxy) / 2.0;
  midz = (minz + maxz) / 2.0;
  if (result.size() >= (size_t)maxresult) return;
  if (pt[2] + r >= minz && pt[2] - r < midz) {
    if (pt[1] + r >= miny && pt[1] - r < midy) {
      if (pt[0] + r >= minx && pt[0] - r < midx)
        find_sub(this->items[ind].ind[0], minx, miny, minz, midx, midy, midz, pt, r, result, resultind,
                 maxresult);
      if (pt[0] + r >= midx && pt[0] - r < maxx)
        find_sub(this->items[ind].ind[1], midx, miny, minz, maxx, midy, midz, pt, r, result, resultind,
                 maxresult);
    }
    if (pt[1] + r >= midy && pt[1] - r < maxy) {
      if (pt[0] + r >= minx && pt[0] - r < midx)
        find_sub(this->items[ind].ind[2], minx, midy, minz, midx, maxy, midz, pt, r, result, resultind,
                 maxresult);
      if (pt[0] + r >= midx && pt[0] - r < maxx)
        find_sub(this->items[ind].ind[3], midx, midy, minz, maxx, maxy, midz, pt, r, result, resultind,
                 maxresult);
    }
  }
  if (pt[2] + r >= midz && pt[2] - r < maxz) {
    if (pt[1] + r >= miny && pt[1] - r < midy) {
      if (pt[0] + r >= minx && pt[0] - r < midx)
        find_sub(this->items[ind].ind[4], minx, miny, midz, midx, midy, maxz, pt, r, result, resultind,
                 maxresult);
      if (pt[0] + r >= midx && pt[0] - r < maxx)
        find_sub(this->items[ind].ind[5], midx, miny, midz, maxx, midy, maxz, pt, r, result, resultind,
                 maxresult);
    }
    if (pt[1] + r >= midy && pt[1] - r < maxy) {
      if (pt[0] + r >= minx && pt[0] - r < midx)
        find_sub(this->items[ind].ind[6], minx, midy, midz, midx, maxy, maxz, pt, r, result, resultind,
                 maxresult);
      if (pt[0] + r >= midx && pt[0] - r < maxx)
        find_sub(this->items[ind].ind[7], midx, midy, midz, maxx, maxy, maxz, pt, r, result, resultind,
                 maxresult);
    }
  }
}
int Map3D::find(Vector3d pt, double r, std::vector<Vector3d>& result, std::vector<int>& resultind,
                int maxresult)
{
  int results = 0;
  if (items.size() == 0) return results;
  result.clear();
  resultind.clear();
  find_sub(0, this->min[0], this->min[1], this->min[2], this->max[0], this->max[1], this->max[2], pt, r,
           result, resultind, maxresult);
  return result.size();
}

void Map3D::dump_hier(int i, int hier, float minx, float miny, float minz, float maxx, float maxy,
                      float maxz)
{
  for (int i = 0; i < hier; i++) printf("  ");
  printf("%d inds ", i);
  for (int j = 0; j < 8; j++) printf("%d ", items[i].ind[j]);
  printf("pts ");
  for (int j = 0; j < items[i].ptlen; j++)
    printf("%g/%g/%g ", items[i].pts[j][0], items[i].pts[j][1], items[i].pts[j][2]);

  float midx, midy, midz;
  midx = (minx + maxx) / 2.0;
  midy = (miny + maxy) / 2.0;
  midz = (minz + maxz) / 2.0;
  printf(" (%g/%g/%g - %g/%g/%g)\n", minx, miny, minz, maxx, maxy, maxz);
  if (items[i].ind[0] != -1) dump_hier(items[i].ind[0], hier + 1, minx, miny, minz, midx, midy, midz);
  if (items[i].ind[1] != -1) dump_hier(items[i].ind[1], hier + 1, midx, miny, minz, maxx, midy, midz);
  if (items[i].ind[2] != -1) dump_hier(items[i].ind[2], hier + 1, minx, midy, minz, midx, maxy, midz);
  if (items[i].ind[3] != -1) dump_hier(items[i].ind[3], hier + 1, midx, midy, minz, maxx, maxy, midz);
  if (items[i].ind[4] != -1) dump_hier(items[i].ind[4], hier + 1, minx, miny, midz, midx, midy, maxz);
  if (items[i].ind[5] != -1) dump_hier(items[i].ind[5], hier + 1, midx, miny, midz, maxx, midy, maxz);
  if (items[i].ind[6] != -1) dump_hier(items[i].ind[6], hier + 1, minx, midy, midz, midx, maxy, maxz);
  if (items[i].ind[7] != -1) dump_hier(items[i].ind[7], hier + 1, midx, midy, midz, maxx, maxy, maxz);
}
void Map3D::dump(void)
{
  dump_hier(0, 0, min[0], min[1], min[2], max[0], max[1], max[2]);
}

GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren(const AbstractNode& node,
                                                                   OpenSCADOperator op)
{
  unsigned int dim = 0;
  for (const auto& item : this->visitedchildren[node.index()]) {
    if (!isValidDim(item, dim)) break;
  }
  switch (dim) {
  case 1:  return ResultObject::mutableResult(std::shared_ptr<Geometry>(applyToChildren1D(node, op)));
  case 2:  return ResultObject::mutableResult(std::shared_ptr<Geometry>(applyToChildren2D(node, op)));
  case 3:  return applyToChildren3D(node, op);
  default: return {};
  }
}

int cut_face_face_face(Vector3d p1, Vector3d n1, Vector3d p2, Vector3d n2, Vector3d p3, Vector3d n3,
                       Vector3d& res, double *detptr)
{
  //   vec1     vec2     vec3
  // x*dirx + y*diry + z*dirz =( posx*dirx + posy*diry + posz*dirz )
  // x*dirx + y*diry + z*dirz =( posx*dirx + posy*diry + posz*dirz )
  // x*dirx + y*diry + z*dirz =( posx*dirx + posy*diry + posz*dirz )
  Vector3d vec1, vec2, vec3, sum;
  vec1[0] = n1[0];
  vec1[1] = n2[0];
  vec1[2] = n3[0];
  vec2[0] = n1[1];
  vec2[1] = n2[1];
  vec2[2] = n3[1];
  vec3[0] = n1[2];
  vec3[1] = n2[2];
  vec3[2] = n3[2];
  sum[0] = p1.dot(n1);
  sum[1] = p2.dot(n2);
  sum[2] = p3.dot(n3);
  return linsystem(vec1, vec2, vec3, sum, res, detptr);
}

int cut_face_line(Vector3d fp, Vector3d fn, Vector3d lp, Vector3d ld, Vector3d& res, double *detptr)
{
  Vector3d c1 = fn.cross(ld);
  Vector3d c2 = fn.cross(c1);
  Vector3d diff = fp - lp;
  if (linsystem(ld, c1, c2, diff, res, detptr)) return 1;
  res = lp + ld * res[0];
  return 0;
}

std::unique_ptr<Geometry> union_geoms(std::vector<std::shared_ptr<PolySet>> parts)  // TODO use widely
{
#ifdef ENABLE_MANIFOLD
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
    std::unique_ptr<ManifoldGeometry> result = nullptr;
    for (auto part : parts) {
      std::shared_ptr<const ManifoldGeometry> part_mani =
        ManifoldUtils::createManifoldFromGeometry(part);
      if (result == nullptr) result = std::make_unique<ManifoldGeometry>(*part_mani);
      else *result = *result + *part_mani;
    }
    return result;
  } else
#endif
  {
    assert(parts.size() > 0);
    std::shared_ptr<CGALNefGeometry> result = nullptr;
    for (const std::shared_ptr<PolySet>& part : parts) {
      std::shared_ptr<const CGALNefGeometry> op = CGALUtils::getNefPolyhedronFromGeometry(part);
      if (result == nullptr) result = std::make_shared<CGALNefGeometry>(*op);
      else *result += *op;
      auto p2 = CGALUtils::getNefPolyhedronFromGeometry(parts[0]);
    }
    return std::make_unique<CGALNefGeometry>(result->p3);
  }
}

std::unique_ptr<Geometry> difference_geoms(
  std::vector<std::shared_ptr<PolySet>> parts)  // TODO use widely
{
#ifdef ENABLE_MANIFOLD
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
    std::unique_ptr<ManifoldGeometry> result = nullptr;
    for (auto part : parts) {
      std::shared_ptr<const ManifoldGeometry> part_mani =
        ManifoldUtils::createManifoldFromGeometry(part);
      if (result == nullptr) result = std::make_unique<ManifoldGeometry>(*part_mani);
      else *result = *result - *part_mani;
    }
    return result;
  } else
#endif
  {
    assert(parts.size() > 0);
    std::shared_ptr<CGALNefGeometry> result = nullptr;
    for (const std::shared_ptr<PolySet>& part : parts) {
      std::shared_ptr<const CGALNefGeometry> op = CGALUtils::getNefPolyhedronFromGeometry(part);
      if (result == nullptr) result = std::make_shared<CGALNefGeometry>(*op);
      else *result -= *op;
      auto p2 = CGALUtils::getNefPolyhedronFromGeometry(parts[0]);
    }
    return std::make_unique<CGALNefGeometry>(result->p3);
  }
}

class Offset3D_CornerContext
{
public:
  PolySetBuilder builder;
  std::vector<IndexedFace> triangles;
  Vector3d basept;
  double r;
  std::vector<Vector2d> flatloop;
  double minx, miny, maxx, maxy;
  Matrix4d invmat;
  std::unordered_map<Vector3d, bool, boost::hash<Vector3d>> inside_map;
};

bool offset3D_inside(Offset3D_CornerContext& cxt, const Vector3d& pt, double delta)
{
  if (cxt.inside_map.count(pt) != 0) {
    return cxt.inside_map.at(pt);
  }

  Vector4d tmp = cxt.invmat * Vector4d(pt[0], pt[1], pt[2], 1);
  if (tmp[2] < 0) {
    cxt.inside_map[pt] = false;
    return false;  // not behind
  }
  if (tmp[0] < cxt.minx + delta || tmp[0] > cxt.maxx - delta) {
    cxt.inside_map[pt] = false;
    return false;
  }
  if (tmp[1] < cxt.miny + delta || tmp[1] > cxt.maxy - delta) {
    cxt.inside_map[pt] = false;
    return false;
  }
  Vector2d ptflat = tmp.head<2>();
  int n = cxt.flatloop.size();
  double mindist = 1e9;
  for (int i = 0; i < n; i++) {
    Vector2d p1 = cxt.flatloop[i];
    Vector2d p2 = cxt.flatloop[(i + 1) % n];
    Vector2d dir = (p2 - p1).normalized();
    Vector2d dir_(-dir[1], dir[0]);

    double dist = (ptflat - p1).dot(dir_);
    if (dist < mindist) {
      mindist = dist;
    }
    if (mindist < delta) {
      cxt.inside_map[pt] = false;
      return false;
    }
  }
  bool result = (mindist > delta);
  cxt.inside_map[pt] = result;
  return result;
}

std::shared_ptr<const Geometry> offset3D(const std::shared_ptr<const PolySet>& ps, double off,
                                         const CurveDiscretizer& discretizer)
{
  std::vector<std::shared_ptr<PolySet>> subgeoms;
  std::shared_ptr<PolySet> inner = std::make_shared<PolySet>(*ps);
  subgeoms.push_back(inner);  // 1st add orignal polyhedron

  std::vector<Vector4d> newNormals;
  std::vector<int> faceParents;
  std::vector<Vector4d> faceNormal = calcTriangleNormals(ps->vertices, ps->indices);
  auto indicesNew = mergeTriangles(ps->indices, faceNormal, newNormals, faceParents, ps->vertices);

  intList empty;
  std::vector<std::vector<size_t>> corner_rounds;
  for (size_t i = 0; i < ps->vertices.size(); i++) corner_rounds.push_back(empty);
  std::vector<Vector3f> verticesFloat;
  for (const auto& v : ps->vertices) verticesFloat.push_back(v.cast<float>());

  std::vector<Vector4d> faceNormals;
  // process all faces and bulid a prisma on top of it
  for (size_t a = 0; a < indicesNew.size(); a++) {
    Vector4d faceNormal = calcTriangleNormal(ps->vertices, indicesNew[a]);
    faceNormals.push_back(faceNormal);
    if (faceParents[a] != -1) continue;  // its a hole

    std::vector<IndexedFace> face_set;
    face_set.push_back(indicesNew[a]);
    for (size_t b = 0; b < faceParents.size(); b++)
      if ((size_t)faceParents[b] == a) face_set.push_back(indicesNew[b]);

    PolySetBuilder builder;

    Vector3d botshift, topshift;
    if (off > 0) {
      botshift = -faceNormal.head<3>() * 0.0001;
      topshift = faceNormal.head<3>() * off;
    } else {
      botshift = faceNormal.head<3>() * off;
      topshift = faceNormal.head<3>() * 0.0001;
    }

    std::vector<IndexedTriangle> triangles;
    Vector3f faceNormalf(faceNormal[0], faceNormal[1], faceNormal[2]);
    GeometryUtils::tessellatePolygonWithHoles(verticesFloat, face_set, triangles, &faceNormalf);

    for (const auto& base : triangles) {
      int bot[3], top[3];
      for (int i = 0; i < 3; i++) {
        bot[i] = builder.vertexIndex(ps->vertices[base[i]] + botshift);
        top[i] = builder.vertexIndex(ps->vertices[base[i]] + topshift);
      }
      builder.appendPolygon({bot[2], bot[1], bot[0]});
      builder.appendPolygon({top[0], top[1], top[2]});
    }

    for (auto face : face_set) {
      std::vector<int> base_pts;
      std::vector<int> top_pts;
      for (auto ind : face) {  // for sides
        Vector3d pt = ps->vertices[ind];
        base_pts.push_back(builder.vertexIndex(pt + botshift));
        top_pts.push_back(builder.vertexIndex(pt + topshift));
      }
      // side faces
      int n = base_pts.size();
      for (int i = 0; i < n; i++) {
        builder.appendPolygon({base_pts[i], base_pts[(i + 1) % n], top_pts[(i + 1) % n], top_pts[i]});
      }
    }

    auto result_u = builder.build();
    std::shared_ptr<PolySet> result_s = std::move(result_u);
    subgeoms.push_back(result_s);  // prisms
  }

  // create edge_db
  std::unordered_map<EdgeKey, std::vector<Vector3d>, boost::hash<EdgeKey>> edge_startarc;
  std::unordered_map<EdgeKey, std::vector<Vector3d>, boost::hash<EdgeKey>> edge_endarc;
  auto edge_db = createEdgeDb(indicesNew);
  int abs_eff_fn = 0;
  for (auto& e : edge_db) {
    Vector3d p1 = ps->vertices[e.first.ind1];
    Vector3d p2 = ps->vertices[e.first.ind2];
    // schauen ob es eine konkave kante ist

    Vector3d fan = faceNormals[e.second.facea].head<3>();
    if (faceParents[e.second.facea] != -1) fan = -fan;
    Vector3d fbn = faceNormals[e.second.faceb].head<3>();
    if (faceParents[e.second.faceb] != -1) fbn = -fbn;
    Vector3d axis = fan.cross(fbn);
    double conv = fan.cross(fbn).dot(p2 - p1);
    if (conv * off < 0) continue;

    corner_rounds[e.first.ind1].push_back(e.first.ind2);
    corner_rounds[e.first.ind2].push_back(e.first.ind1);
    double totang = acos(fan.dot(fbn));
    std::vector<Vector3d> startarc, endarc;
    // create arcs for begin and end
    startarc.push_back(p1 + off * fan);
    endarc.push_back(p2 + off * fan);

    int eff_fn = discretizer.getCircularSegmentCountAlt(fabs(off), 180.0*totang/M_PI).value_or(3);

    for (int i = 1; i < eff_fn - 1; i++) {
      Transform3d matrix = Transform3d::Identity();
      auto M = angle_axis_degrees(180 / 3.14 * totang * i / (double)(eff_fn - 1), axis);
      matrix.rotate(M);
      Vector3d rotv = matrix * fan;
      startarc.push_back(p1 + off * rotv);
      endarc.push_back(p2 + off * rotv);
    }
    startarc.push_back(p1 + off * fbn);
    endarc.push_back(p2 + off * fbn);

    edge_startarc[e.first] = startarc;
    edge_endarc[e.first] = endarc;
  }

  for (auto& e : edge_db) {
    if (!edge_startarc.count(e.first)) continue;
    std::vector<Vector3d> startarc;
    std::vector<Vector3d> endarc;
    int startpt, endpt;
    PolySetBuilder builder;
    if (off > 0) {
      startarc = edge_startarc[e.first];
      endarc = edge_endarc[e.first];
      startpt = builder.vertexIndex(ps->vertices[e.first.ind1]);
      endpt = builder.vertexIndex(ps->vertices[e.first.ind2]);
    } else {
      startarc = edge_endarc[e.first];
      endarc = edge_startarc[e.first];
      endpt = builder.vertexIndex(ps->vertices[e.first.ind1]);
      startpt = builder.vertexIndex(ps->vertices[e.first.ind2]);
    }
    std::vector<int> start_inds, end_inds;

    int n = startarc.size();
    for (int i = 0; i < n; i++) {
      start_inds.push_back(builder.vertexIndex(startarc[i]));
      end_inds.push_back(builder.vertexIndex(endarc[i]));
    }
    // end wall
    builder.beginPolygon(n + 1);
    for (size_t i = 0; i < end_inds.size(); i++) builder.addVertex(end_inds[i]);
    builder.addVertex(endpt);

    // top rounding
    for (size_t i = 0; i < (size_t)n - 1; i++) {
      builder.appendPolygon({start_inds[i], start_inds[i + 1], end_inds[i + 1], end_inds[i]});
    }

    // start wall
    builder.beginPolygon(n + 1);
    for (int i = n - 1; i >= 0; i--) builder.addVertex(start_inds[i]);
    builder.addVertex(startpt);

    // front wall
    builder.appendPolygon({startpt, start_inds[0], end_inds[0], endpt});

    // back wall
    builder.appendPolygon({startpt, endpt, end_inds[n - 1], start_inds[n - 1]});

    auto result_u = builder.build();
    std::shared_ptr<PolySet> result_s = std::move(result_u);
    subgeoms.push_back(result_s);  // edges
  }

  for (size_t i = 0; i < ps->vertices.size(); i++) {
    Offset3D_CornerContext cxt;
    cxt.basept = ps->vertices[i];
    cxt.r = off;
    int baseind = cxt.builder.vertexIndex(cxt.basept);
    if (corner_rounds[i].size() < 3) continue;
    std::vector<IndexedFace> stubs;
    for (auto oth : corner_rounds[i]) {
      EdgeKey key(i, oth);
      if (!edge_startarc.count(key)) continue;
      std::vector<Vector3d> arc;
      if (i < oth) arc = edge_startarc[key];
      else {
        arc = edge_endarc[key];
        std::reverse(arc.begin(), arc.end());
      }
      IndexedFace stub;
      for (auto pt : arc) stub.push_back(cxt.builder.vertexIndex(pt));
      stubs.push_back(stub);
    }

    std::vector<Vector3d> vertices;
    cxt.builder.copyVertices(vertices);

    IndexedFace combined = stubs[0];
    stubs.erase(stubs.begin());
    int conn = combined[combined.size() - 1];
    bool done = true;
    while (stubs.size() > 0 && done) {
      done = false;
      for (size_t i = 0; i < stubs.size(); i++) {
        if (stubs[i][0] == conn) {
          for (size_t j = 1; j < stubs[i].size(); j++) {
            combined.push_back(stubs[i][j]);
          }
          stubs.erase(stubs.begin() + i);
          done = true;
          break;
        }
      }
      conn = combined[combined.size() - 1];
    }
    combined.erase(combined.end() - 1);

    // create normal vector of the opening
    Vector3d norm(0, 0, 0);
    int cs = combined.size();
    for (int i = 0; i < cs; i++) {
      Vector3d p1 = vertices[combined[i]];
      Vector3d p2 = vertices[combined[(i + 1) % cs]];
      Vector3d p3 = vertices[combined[(i + 2) % cs]];
      norm += (p2 - p1).cross(p2 - p3);
    }
    Vector3d xdir, ydir, zdir;
    zdir = norm.normalized();
    ydir = -zdir.cross(Vector3d(1, 0, 0));
    if (ydir.norm() < 1e-3) ydir = -zdir.cross(Vector3d(1, 0, 0));
    ydir.normalized();
    xdir = zdir.cross(ydir).normalized();

    // setup matrix
    Matrix4d mat;
    mat << xdir[0], ydir[0], zdir[0], cxt.basept[0], xdir[1], ydir[1], zdir[1], cxt.basept[1], xdir[2],
      ydir[2], zdir[2], cxt.basept[2], 0, 0, 0, 1;

    cxt.invmat = mat.inverse();

    for (size_t i = 0; i < combined.size(); i++) {
      Vector3d pt = vertices[combined[i]];
      Vector2d pt2 = (cxt.invmat * Vector4d(pt[0], pt[1], pt[2], 1)).head<2>();
      if (i == 0 || pt2[0] < cxt.minx) cxt.minx = pt2[0];
      if (i == 0 || pt2[1] < cxt.miny) cxt.miny = pt2[1];
      if (i == 0 || pt2[0] > cxt.maxx) cxt.maxx = pt2[0];
      if (i == 0 || pt2[1] > cxt.maxy) cxt.maxy = pt2[1];
      cxt.flatloop.push_back(pt2);
    }
    // Lay flat all edge loop

    // now create a geodesic sphere from here

    Vector3d sp_left = cxt.basept + Vector3d(-off, 0, 0);
    Vector3d sp_right = cxt.basept + Vector3d(off, 0, 0);
    Vector3d sp_front = cxt.basept + Vector3d(0, -off, 0);
    Vector3d sp_back = cxt.basept + Vector3d(0, off, 0);
    Vector3d sp_bot = cxt.basept + Vector3d(0, 0, -off);
    Vector3d sp_top = cxt.basept + Vector3d(0, 0, off);

    int hier = (int)(log(abs_eff_fn) / log(2)) + 1;

    // cerate basic octaeder
    std::vector<std::vector<Vector3d>> triangles;
    triangles.push_back({sp_front, sp_right, sp_top});
    triangles.push_back({sp_right, sp_back, sp_top});
    triangles.push_back({sp_back, sp_left, sp_top});
    triangles.push_back({sp_left, sp_front, sp_top});
    triangles.push_back({sp_front, sp_bot, sp_right});
    triangles.push_back({sp_right, sp_bot, sp_back});
    triangles.push_back({sp_back, sp_bot, sp_left});
    triangles.push_back({sp_left, sp_bot, sp_front});
    for (int i = 0; i < hier; i++) {
      // subdivide them
      std::vector<std::vector<Vector3d>> tri_new;
      for (auto tri : triangles) {
        Vector3d p12 = (tri[0] + tri[1]) / 2.0;
        Vector3d p23 = (tri[1] + tri[2]) / 2.0;
        Vector3d p31 = (tri[2] + tri[0]) / 2.0;
        p12 = (p12 - cxt.basept).normalized() * cxt.r + cxt.basept;
        p23 = (p23 - cxt.basept).normalized() * cxt.r + cxt.basept;
        p31 = (p31 - cxt.basept).normalized() * cxt.r + cxt.basept;
        tri_new.push_back({p31, tri[0], p12});
        tri_new.push_back({p12, tri[1], p23});
        tri_new.push_back({p23, tri[2], p31});
        tri_new.push_back({p12, p23, p31});
      }
      triangles = tri_new;
    }
    // now filter them // TODO apply adding hier again
    assert(triangles.size() > 0);
    double delta = (triangles[0][0] - triangles[0][1]).norm() * 0.5;
    for (auto tri : triangles) {
      bool p1_inside = offset3D_inside(cxt, tri[0], delta);
      bool p2_inside = offset3D_inside(cxt, tri[1], delta);
      bool p3_inside = offset3D_inside(cxt, tri[2], delta);
      if (p1_inside && p2_inside && p3_inside) {
        int ind1 = cxt.builder.vertexIndex(tri[0]);
        int ind2 = cxt.builder.vertexIndex(tri[1]);
        int ind3 = cxt.builder.vertexIndex(tri[2]);

        cxt.builder.appendPolygon({ind1, ind2, ind3});
        cxt.triangles.push_back({ind1, ind2, ind3});
      }
    }

    std::vector<Vector3d> dummyvert;
    indexedFaceList triangles_merged = mergeTrianglesSub(cxt.triangles, dummyvert);
    // assume biggest one is main
    assert(triangles_merged.size() > 0);
    int bigind = 0;
    for (size_t i = 1; i < triangles_merged.size(); i++)
      if (triangles_merged[i].size() > triangles_merged[bigind].size()) bigind = i;
    auto inner = triangles_merged[bigind];

    cxt.builder.copyVertices(vertices);

    // now connect combined with inner_tri
    int combined_ind = 0;
    int combined_n = combined.size();

    int inner_ind = -1;
    int inner_n = inner.size();

    double dist_min;
    for (size_t i = 0; i < inner.size(); i++) {
      double dist = (vertices[inner[i]] - vertices[combined[combined_ind]]).norm();
      if (inner_ind == -1 || dist < dist_min) {
        inner_ind = i;
        dist_min = dist;
      }
    }
    int inner_ind_start = inner_ind;
    do {
      double dist1 =
        (vertices[inner[(inner_ind + inner_n - 1) % inner_n]] - vertices[combined[combined_ind]])
          .norm();  // prog inner
      double dist2 = (vertices[inner[inner_ind]] - vertices[combined[(combined_ind + 1) % combined_n]])
                       .norm();  // prog combined
      if (dist1 < dist2) {
        cxt.builder.appendPolygon({inner[inner_ind], inner[(inner_ind + inner_n - 1) % inner_n],
                                   combined[combined_ind]});  // prog inner
        inner_ind = (inner_ind + inner_n - 1) % inner_n;
      } else {
        cxt.builder.appendPolygon({inner[inner_ind], combined[(combined_ind + 1) % combined_n],
                                   combined[combined_ind]});  // prog combined
        combined_ind = (combined_ind + 1) % combined_n;
      }
    } while (combined_ind != 0 || inner_ind != inner_ind_start);
    int n = combined.size();
    for (int i = 0; i < n; i++) {  // unterer kranz
      if (off > 0) cxt.builder.appendPolygon({baseind, combined[i], combined[(i + 1) % n]});
      else cxt.builder.appendPolygon({combined[i], baseind, combined[(i + 1) % n]});
    }

    auto result_u = cxt.builder.build();
    std::shared_ptr<PolySet> result_s = std::move(result_u);
    subgeoms.push_back(result_s);  // corners
  }
  if (off > 0) {
    auto r = union_geoms(subgeoms);

    return r;

  } else {
    return difference_geoms(subgeoms);
  }
}

std::unique_ptr<const Geometry> createFilletInt(std::shared_ptr<const PolySet> ps,
                                                std::vector<bool> corner_selected, double r, int fn,
                                                double minang);

Vector3d createFilletRound(Vector3d pt)
{
  double x = ((int)(pt[0] * 1e6)) / 1e6;
  double y = ((int)(pt[1] * 1e6)) / 1e6;
  double z = ((int)(pt[2] * 1e6)) / 1e6;
  return Vector3d(x, y, z);
}
std::unique_ptr<const Geometry> addFillets(std::shared_ptr<const Geometry> result,
                                           const Geometry::Geometries& children, double r, int fn)
{
  std::unordered_set<Vector3d> points;
  Vector3d pt;
  std::shared_ptr<const PolySet> psr = PolySetUtils::getGeometryAsPolySet(result);
  for (const Vector3d& pt : psr->vertices) {
    points.insert(createFilletRound(pt));
  }

  for (const auto& child : children) {
    std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(child.second);
    for (const Vector3d& pt : ps->vertices) {
      points.erase(createFilletRound(pt));
    }
  }

  std::vector<bool> corner_selected;
  for (size_t i = 0; i < psr->vertices.size(); i++)
    corner_selected.push_back(points.count(createFilletRound(psr->vertices[i])) > 0 ? true : false);

  return createFilletInt(psr, corner_selected, r, fn, 30.0);
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
  if (children.size() == 1 && op != OpenSCADOperator::OFFSET)
    return ResultObject::constResult(children.front().second);

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
    const CsgOpNode *csgOpNode = dynamic_cast<const CsgOpNode *>(&node);
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
    if (actualchildren.size() == 1) return ResultObject::constResult(actualchildren.front().second);
#ifdef ENABLE_MANIFOLD
    if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
      std::shared_ptr<const ManifoldGeometry> csgResult =
        ManifoldUtils::applyOperator3DManifold(actualchildren, op);
      if (csgOpNode != nullptr && csgOpNode->r != 0) {
        std::unique_ptr<const Geometry> geom_u =
          addFillets(csgResult, actualchildren, csgOpNode->r, csgOpNode->fn);
        std::shared_ptr<const Geometry> geom_s(geom_u.release());
        return ResultObject::mutableResult(geom_s);
        // csgResult = ManifoldUtils::createManifoldFromGeometry(geom_s);
      }
      return ResultObject::mutableResult(csgResult);
    }
#endif
#ifdef ENABLE_CGAL
    return ResultObject::constResult(std::shared_ptr<const Geometry>(
      CGALUtils::applyUnion3D(*csgOpNode, actualchildren.begin(), actualchildren.end())));
#else
    assert(false && "No boolean backend available");
#endif
    break;
  }
  case OpenSCADOperator::OFFSET: {
    std::string instance_name;
    AssignmentList inst_asslist;
    ModuleInstantiation *instance = new ModuleInstantiation(instance_name, inst_asslist, Location::NONE);
    auto node1 = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);

    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    const OffsetNode *offNode = dynamic_cast<const OffsetNode *>(&node);
    std::shared_ptr<const Geometry> geom;
    switch (actualchildren.size()) {
    case 0: return {}; break;
    case 1: geom = {actualchildren.front().second}; break;
    default:
#ifdef ENABLE_CGAL
      geom = {CGALUtils::applyUnion3D(*node1, actualchildren.begin(), actualchildren.end())};
#endif
      break;
    }

    std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);
    if (ps != nullptr) {
      auto ps_offset = offset3D(ps, offNode->delta, offNode->discretizer);

      geom = std::move(ps_offset);
      return ResultObject::mutableResult(geom);
    }
  }
  default: {
#ifdef ENABLE_MANIFOLD
    if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
      std::shared_ptr<const ManifoldGeometry> csgResult =
        ManifoldUtils::applyOperator3DManifold(children, op);
      do {
        const CsgOpNode *csgOpNode = dynamic_cast<const CsgOpNode *>(&node);
        if (csgOpNode == nullptr) break;
        if (csgOpNode->r == 0) break;
        std::unique_ptr<const Geometry> geom_u =
          addFillets(csgResult, children, csgOpNode->r, csgOpNode->fn);
        std::shared_ptr<const Geometry> geom_s(geom_u.release());
        //	csgResult = ManifoldUtils::createManifoldFromGeometry(geom_s);
        return ResultObject::mutableResult(geom_s);
      } while (0);
      return ResultObject::mutableResult(csgResult);
    }
#endif
#ifdef ENABLE_CGAL
    const CsgOpNode *csgOpNode = dynamic_cast<const CsgOpNode *>(&node);
    return ResultObject::constResult(CGALUtils::applyOperator3D(*csgOpNode, children, op));
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
  Color4f resultcolor;
#ifdef ENABLE_CGAL
  using CGALPoint2 = CGAL::Point_2<CGAL_DoubleKernel>;
  // Collect point cloud
  std::list<CGALPoint2> points;
  for (const auto& p : children) {
    if (p) {
      for (const auto& o : p->outlines()) {
        if (resultcolor.r() < 0) resultcolor = o.color;
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
      outline.color = resultcolor;
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
std::vector<std::shared_ptr<const Barcode1d>> GeometryEvaluator::collectChildren1D(
  const AbstractNode& node)
{
  std::vector<std::shared_ptr<const Barcode1d>> children;
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
      if (chgeom->getDimension() != 1) {
        LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(),
            "Ignoring other child object for 1D operation");
        children.push_back(nullptr);  // replace 3D geometry with empty geometry
      } else {
        if (chgeom->isEmpty()) {
          children.push_back(nullptr);
        } else {
          const auto barcode1d = std::dynamic_pointer_cast<const Barcode1d>(chgeom);
          assert(barcode1d);
          children.push_back(barcode1d);
        }
      }
    } else {
      children.push_back(nullptr);
    }
  }
  return children;
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
  if (key.empty()) return {};
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

  if (children.size() == 1 && op != OpenSCADOperator::OFFSET) {
    if (children[0]) {
      return std::make_unique<Polygon2d>(*children[0]);  // Copy
    } else {
      return nullptr;
    }
  }

  Clipper2Lib::ClipType clipType;
  switch (op) {
  case OpenSCADOperator::UNION:
  case OpenSCADOperator::OFFSET:       clipType = Clipper2Lib::ClipType::Union; break;
  case OpenSCADOperator::INTERSECTION: clipType = Clipper2Lib::ClipType::Intersection; break;
  case OpenSCADOperator::DIFFERENCE:   clipType = Clipper2Lib::ClipType::Difference; break;
  default:
    LOG(message_group::Error, "Unknown boolean operation %1$d", int(op));
    return nullptr;
    break;
  }

  auto pol = ClipperUtils::apply(children, clipType);

  if (op == OpenSCADOperator::OFFSET) {
    const OffsetNode *offNode = dynamic_cast<const OffsetNode *>(&node);
    // ClipperLib documentation: The formula for the number of steps in a full
    // circular arc is ... Pi / acos(1 - arc_tolerance / abs(delta))
    double n = offNode->discretizer.getCircularSegmentCount(offNode->delta).value_or(3);
    double arc_tolerance = std::abs(offNode->delta) * (1 - cos_degrees(180 / n));
    auto r1 = ClipperUtils::applyOffset(*pol, offNode->delta, offNode->join_type, offNode->miter_limit,
                                        arc_tolerance);
    return r1;
  }
  return pol;
}

std::unique_ptr<Barcode1d> GeometryEvaluator::applyToChildren1D(const AbstractNode& node,
                                                                OpenSCADOperator op)
{
  node.progress_report();
  std::vector<std::shared_ptr<const Barcode1d>> children = collectChildren1D(node);

  if (children.empty()) {
    return nullptr;
  }
  Barcode1d result;
  for (auto child : children) {
    for (const auto& e : child->edges()) {
      result.addEdge(e);
    }
  }
  return std::make_unique<Barcode1d>(result);
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
      ResultObject res = applyToChildren(node, OpenSCADOperator::OFFSET);
      auto mutableGeom = res.asMutableGeometry();
      if (mutableGeom) mutableGeom->setConvexity(1);
      geom = mutableGeom;
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
          switch (geom->getDimension()) {
          case 1: {
            auto barcode = std::dynamic_pointer_cast<Barcode1d>(res.asMutableGeometry());
            assert(barcode);

            geom = barcode;
            barcode->transform3d(node.matrix);
          } break;
          case 2: {
            auto polygons = std::dynamic_pointer_cast<Polygon2d>(res.asMutableGeometry());
            assert(polygons);

            geom = polygons;
            polygons->transform3d(node.matrix);
          } break;
          case 3: {
            auto mutableGeom = res.asMutableGeometry();
            if (mutableGeom) mutableGeom->transform(node.matrix);
            geom = mutableGeom;
          } break;
          }  // switch
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

VectorOfVector2d alterprofile(VectorOfVector2d vertices, double scalex, double scaley, double origin_x,
                              double origin_y, double offset_x, double offset_y, double rot)
{
  VectorOfVector2d result;
  double ang = rot * 3.14 / 180.0;
  double c = cos(ang);
  double s = sin(ang);
  int n = vertices.size();
  for (int i = 0; i < n; i++) {
    double x = (vertices[i][0] - origin_x) * scalex;
    double y = (vertices[i][1] - origin_y) * scaley;
    double xr = (x * c - y * s) + origin_x + offset_x;
    double yr = (y * c + x * s) + origin_y + offset_y;
    result.push_back(Vector2d(xr, yr));
  }
  return result;
}

void calculate_path_dirs(Vector3d prevpt, Vector3d curpt, Vector3d nextpt, Vector3d vec_x_last,
                         Vector3d vec_y_last, Vector3d *vec_x, Vector3d *vec_y)
{
  Vector3d diff1, diff2;
  diff1 = curpt - prevpt;
  diff2 = nextpt - curpt;
  double xfac = 1.0, yfac = 1.0, beta;

  if (diff1.norm() > 0.001) diff1.normalize();
  if (diff2.norm() > 0.001) diff2.normalize();
  Vector3d diff = diff1 + diff2;

  if (diff.norm() < 0.001) {
    printf("User Error!\n");
    return;
  }
  if (vec_y_last.norm() < 0.001) {  // Needed in first step only
    vec_y_last = diff2.cross(vec_x_last);
    if (vec_y_last.norm() < 0.001) {
      vec_x_last[0] = 1;
      vec_x_last[1] = 0;
      vec_x_last[2] = 0;
      vec_y_last = diff.cross(vec_x_last);
    }
    if (vec_y_last.norm() < 0.001) {
      vec_x_last[0] = 0;
      vec_x_last[1] = 1;
      vec_x_last[2] = 0;
      vec_y_last = diff.cross(vec_x_last);
    }
    if (vec_y_last.norm() < 0.001) {
      vec_x_last[0] = 0;
      vec_x_last[1] = 0;
      vec_x_last[2] = 1;
      vec_y_last = diff.cross(vec_x_last);
    }
  } else {
    // make vec_last normal to diff1
    Vector3d xn = vec_y_last.cross(diff1).normalized();
    Vector3d yn = diff1.cross(vec_x_last).normalized();

    // now fix the angle between xn and yn
    Vector3d vec_xy_ = (xn + yn).normalized();
    Vector3d vec_xy = vec_xy_.cross(diff1).normalized();
    vec_x_last = (vec_xy_ + vec_xy).normalized();
    vec_y_last = diff1.cross(xn).normalized();
  }

  diff = (diff1 + diff2).normalized();

  *vec_y = diff.cross(vec_x_last);
  if (vec_y->norm() < 0.001) {
    vec_x_last[0] = 1;
    vec_x_last[1] = 0;
    vec_x_last[2] = 0;
    *vec_y = diff.cross(vec_x_last);
  }
  if (vec_y->norm() < 0.001) {
    vec_x_last[0] = 0;
    vec_x_last[1] = 1;
    vec_x_last[2] = 0;
    *vec_y = diff.cross(vec_x_last);
  }
  if (vec_y->norm() < 0.001) {
    vec_x_last[0] = 0;
    vec_x_last[1] = 0;
    vec_x_last[2] = 1;
    *vec_y = diff.cross(vec_x_last);
  }
  vec_y->normalize();

  *vec_x = vec_y_last.cross(diff);
  if (vec_x->norm() < 0.001) {
    vec_y_last[0] = 1;
    vec_y_last[1] = 0;
    vec_y_last[2] = 0;
    *vec_x = vec_y_last.cross(diff);
  }
  if (vec_x->norm() < 0.001) {
    vec_y_last[0] = 0;
    vec_y_last[1] = 1;
    vec_y_last[2] = 0;
    *vec_x = vec_y_last.cross(diff);
  }
  if (vec_x->norm() < 0.001) {
    vec_y_last[0] = 0;
    vec_y_last[1] = 0;
    vec_y_last[2] = 1;
    *vec_x = vec_y_last.cross(diff);
  }
  vec_x->normalize();

  if (diff1.norm() > 0.001 && diff2.norm() > 0.001) {
    beta = (*vec_x).dot(diff1);
    xfac = sqrt(1 - beta * beta);
    beta = (*vec_y).dot(diff1);
    yfac = sqrt(1 - beta * beta);
  }
  (*vec_x) /= xfac;
  (*vec_y) /= yfac;
}

std::vector<Vector3d> calculate_path_profile(Vector3d *vec_x, Vector3d *vec_y, Vector3d curpt,
                                             const std::vector<Vector2d>& profile)
{
  std::vector<Vector3d> result;
  for (unsigned int i = 0; i < profile.size(); i++) {
    result.push_back(Vector3d(curpt[0] + (*vec_x)[0] * profile[i][0] + (*vec_y)[0] * profile[i][1],
                              curpt[1] + (*vec_x)[1] * profile[i][0] + (*vec_y)[1] * profile[i][1],
                              curpt[2] + (*vec_x)[2] * profile[i][0] + (*vec_y)[2] * profile[i][1]));
  }
  return result;
}

static std::unique_ptr<Geometry> extrudePath(const PathExtrudeNode& node, const Polygon2d& poly)
{
  PolySetBuilder builder;
  builder.setConvexity(node.convexity);
  std::vector<Vector3d> path_os;
  std::vector<double> length_os;
  gboolean intersect = false;
  if (node.path.size() < 2) return builder.build();

  // Round the corners with radius
  int xdir_offset = 0;  // offset in point list to apply the xdir
  std::vector<Vector3d> path_round;
  unsigned int m = node.path.size();
  for (unsigned i = 0; i < node.path.size(); i++) {
    int draw_arcs = 0;
    Vector3d diff1, diff2, center, arcpt;
    int secs;
    double ang;
    Vector3d cur = node.path[i].head<3>();
    double r = node.path[i][3];
    do {
      if (i == 0 && node.closed == 0) break;
      if (i == m - 1 && node.closed == 0) break;

      Vector3d prev = node.path[(i + m - 1) % m].head<3>();
      Vector3d next = node.path[(i + 1) % m].head<3>();
      diff1 = (prev - cur).normalized();
      diff2 = (next - cur).normalized();
      Vector3d diff = (diff1 + diff2).normalized();

      ang = acos(diff1.dot(-diff2));
      double arclen = ang * r;
      center = cur + (r / cos(ang / 2.0)) * diff;

      secs = node.fn;
      int secs_a, secs_s;
      secs_a = (int)ceil(180.0 * ang / (G_PI * node.fa));
      if (secs_a > secs) secs = secs_a;

      secs_s = (int)ceil(arclen / node.fs);
      if (secs_s > secs) secs = secs_s;

      if (r == 0) break;
      if (secs == 0) break;
      draw_arcs = 1;
    } while (false);
    if (draw_arcs) {
      draw_arcs = 1;
      Vector3d diff1n = diff1.cross(diff2.cross(diff1)).normalized();
      for (int j = 0; j <= secs; j++) {
        arcpt =
          center - diff1 * r * sin(ang * j / (double)secs) - diff1n * r * cos(ang * j / (double)secs);
        path_round.push_back(arcpt);
      }
      if (node.closed > 0 && i == 0) xdir_offset = secs;  // user wants to apply xdir on this point
    } else path_round.push_back(cur);
  }

  // xdir_offset is claculated in in next step automatically
  //
  // Create oversampled path with fs. for streights
  path_os.push_back(path_round[xdir_offset]);
  length_os.push_back(0);
  m = path_round.size();
  int ifinal = node.closed ? m : m - 1;

  for (int i = 1; i <= ifinal; i++) {
    Vector3d prevPt = path_round[(i + xdir_offset - 1) % m];
    Vector3d curPt = path_round[(i + xdir_offset) % m];
    Vector3d seg = curPt - prevPt;
    double length_seg = seg.norm();
    size_t split = ceil(length_seg / node.fs);
    if (node.twist == 0 && node.scale_x == 1.0 && node.scale_y == 1.0) split = 1;
    for (size_t j = 1; j <= split; j++) {
      double ratio = (double)j / (double)split;
      path_os.push_back(prevPt + seg * ratio);
      length_os.push_back((i - 1 + (double)j / (double)split) / (double)(path_round.size() - 1));
    }
  }
  if (node.closed) {  // let close do its last pt itself
    path_os.pop_back();
    length_os.pop_back();
  }

  Vector3d lastPt, curPt, nextPt;

  // in case of custom profile,poly shall exactly have one dummy outline,will be replaced
  for (const Outline2d& profile2d : poly.outlines()) {
    Vector3d vec_x_last(node.xdir_x, node.xdir_y, node.xdir_z);
    Vector3d vec_y_last(0, 0, 0);
    vec_x_last.normalize();

    std::vector<Vector3d> lastProfile;
    std::vector<Vector3d> startProfile;
    unsigned int m = path_os.size();
    unsigned int mfinal = (node.closed == true) ? m + 1 : m - 1;
    for (unsigned int i = 0; i <= mfinal; i++) {
      std::vector<Vector3d> curProfile;
      double cur_twist;

#ifdef ENABLE_PYTHON
      if (node.twist_func != NULL) {
        cur_twist = python_doublefunc(node.twist_func, length_os[i]);
      } else
#endif
        cur_twist = node.twist * length_os[i];

      double cur_scalex = 1.0 + (node.scale_x - 1.0) * length_os[i];
      double cur_scaley = 1.0 + (node.scale_y - 1.0) * length_os[i];
      Outline2d profilemod;
#ifdef ENABLE_PYTHON
      if (node.profile_func != NULL) {
        Outline2d tmpx = python_getprofile(node.profile_func, node.fn, length_os[i % m]);
        profilemod.vertices = alterprofile(tmpx.vertices, cur_scalex, cur_scaley, node.origin_x,
                                           node.origin_y, 0, 0, cur_twist);
      } else
#endif
        profilemod.vertices = alterprofile(profile2d.vertices, cur_scalex, cur_scaley, node.origin_x,
                                           node.origin_y, 0, 0, cur_twist);

      unsigned int n = profilemod.vertices.size();
      curPt = path_os[i % m];
      if (i > 0) lastPt = path_os[(i - 1) % m];
      else lastPt = path_os[i % m];
      if (node.closed == true) {
        nextPt = path_os[(i + 1) % m];
      } else {
        if (i < m - 1) nextPt = path_os[(i + 1) % m];
        else nextPt = path_os[i % m];
      }
      Vector3d vec_x, vec_y;
      if (i != m + 1) {
        calculate_path_dirs(lastPt, curPt, nextPt, vec_x_last, vec_y_last, &vec_x, &vec_y);
        curProfile = calculate_path_profile(&vec_x, &vec_y, curPt, profilemod.vertices);
      } else curProfile = startProfile;
      if (i == 1 && node.closed == true) startProfile = curProfile;

      if ((node.closed == false && i == 1) || (i >= 2)) {  // create ring
        // collision detection
        Vector3d vec_z_last = vec_x_last.cross(vec_y_last);
        // check that all new points are above old plane lastPt, vec_z_last
        for (unsigned int j = 0; j < n; j++) {
          double dist = (curProfile[j] - lastPt).dot(vec_z_last);
          if (dist < 0) intersect = true;
        }

        for (unsigned int j = 0; j < n; j++) {
          builder.beginPolygon(3);
          builder.addVertex(builder.vertexIndex(Vector3d(
            lastProfile[(j + 0) % n][0], lastProfile[(j + 0) % n][1], lastProfile[(j + 0) % n][2])));
          builder.addVertex(builder.vertexIndex(Vector3d(
            lastProfile[(j + 1) % n][0], lastProfile[(j + 1) % n][1], lastProfile[(j + 1) % n][2])));
          builder.addVertex(builder.vertexIndex(Vector3d(
            curProfile[(j + 1) % n][0], curProfile[(j + 1) % n][1], curProfile[(j + 1) % n][2])));
          builder.beginPolygon(3);
          builder.addVertex(builder.vertexIndex(Vector3d(
            lastProfile[(j + 0) % n][0], lastProfile[(j + 0) % n][1], lastProfile[(j + 0) % n][2])));
          builder.addVertex(builder.vertexIndex(Vector3d(
            curProfile[(j + 1) % n][0], curProfile[(j + 1) % n][1], curProfile[(j + 1) % n][2])));
          builder.addVertex(builder.vertexIndex(Vector3d(
            curProfile[(j + 0) % n][0], curProfile[(j + 0) % n][1], curProfile[(j + 0) % n][2])));
        }
      }
      if (node.closed == false && (i == 0 || i == m - 1)) {
        Polygon2d face_poly;
        face_poly.addOutline(profilemod);
        std::unique_ptr<PolySet> ps_face = face_poly.tessellate();

        if (i == 0) {
          // Flip vertex ordering for bottom polygon
          for (auto& polygon : ps_face->indices) {
            std::reverse(polygon.begin(), polygon.end());
          }
        }
        for (auto& p3d : ps_face->indices) {
          std::vector<Vector2d> p2d;
          for (unsigned int i = 0; i < p3d.size(); i++) {
            Vector3d pt = ps_face->vertices[p3d[i]];
            p2d.push_back(Vector2d(pt[0], pt[1]));
          }
          std::vector<Vector3d> newprof =
            calculate_path_profile(&vec_x, &vec_y, (i == 0) ? curPt : nextPt, p2d);
          builder.beginPolygon(newprof.size());
          for (Vector3d pt : newprof) {
            builder.addVertex(builder.vertexIndex(pt));
          }
        }
      }

      vec_x_last = vec_x.normalized();
      vec_y_last = vec_y.normalized();

      lastProfile = curProfile;
    }
  }
  if (intersect == true && node.allow_intersect == false) {
    LOG(message_group::Warning, "Model is self intersecting. Result is unpredictable. ");
  }
  return builder.build();
}

/*!
  input: List of 2D objects arranged in 3D, each with identical outline count and vertex count
  output: 3D PolySet
 */
Response GeometryEvaluator::visit(State& state, const SkinNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom =
      isSmartCached(node) ? smartCacheGet(node, false)
                          : skinPolygonSequence(node, collectChildren2D(node), node.modinst->location(),
                                                this->tree.getDocumentPath());
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}
/*!
  input: List of 2D objects arranged in 3D, each with identical outline count and vertex count
  output: 3D PolySet
 */

double concat_round(double x)
{
  if (x > 0) return ((int)(x * 1000 + 0.5)) / 1000.0;
  if (x < 0) return -((int)(-x * 1000 + 0.5)) / 1000.0;
  return 0;
}
Response GeometryEvaluator::visit(State& state, const ConcatNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      Geometry::Geometries children = collectChildren3D(node);
      PolySetBuilder builder(0, 0, 3, true);
      for (const auto& child : children) {
        const auto ps = std::dynamic_pointer_cast<const PolySet>(child.second);
        if (ps == nullptr) {
          LOG(message_group::Error, "Concat only works for PolySet Data");
          continue;
        }
        for (size_t i = 0; i < ps->indices.size(); i++) {
          const auto& face = ps->indices[i];
          builder.beginPolygon(face.size());
          for (int ind : face) {
            Vector3d pt = ps->vertices[ind];
            pt[0] = concat_round(pt[0]);
            pt[1] = concat_round(pt[1]);
            pt[2] = concat_round(pt[2]);
            builder.addVertex(pt);
          }
          if (ps->color_indices.size() > i) builder.endPolygon(ps->colors[ps->color_indices[i]]);
          else builder.endPolygon();
        }
      }
      geom = builder.build();
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
Response GeometryEvaluator::visit(State& state, const LinearExtrudeNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
      const std::shared_ptr<const Geometry> geometry = res.constptr();
      if (geometry) {
        const auto polygons = std::dynamic_pointer_cast<const Polygon2d>(geometry);
        const auto barcode1d = std::dynamic_pointer_cast<const Barcode1d>(geometry);

        if (polygons != nullptr) geom = extrudePolygon(node, *polygons);
        if (barcode1d != nullptr) geom = extrudeBarcode(node, *barcode1d);

        if (geom == nullptr) geom = {};
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

Response GeometryEvaluator::visit(State& state, const PathExtrudeNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      std::shared_ptr<Geometry> geometry = applyToChildren2D(node, OpenSCADOperator::UNION);
      if (geometry) {
        const auto polygons = std::dynamic_pointer_cast<const Polygon2d>(geometry);
        auto extruded = extrudePath(node, *polygons);
        //	printf("extrude = %p\n",extruded);
        assert(extruded);
        geom = std::move(extruded);
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
      ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
      const std::shared_ptr<const Geometry> geometry = res.constptr();
      const auto polygons = std::dynamic_pointer_cast<const Polygon2d>(geometry);
      const auto barcode1d = std::dynamic_pointer_cast<const Barcode1d>(geometry);

      if (polygons != nullptr) geom = rotatePolygon(node, *polygons);
      if (barcode1d != nullptr) geom = rotateBarcode(node, *barcode1d);
      if (geom == nullptr) geom = {};
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

static int pullObject_calccut(const PullNode& node, Vector3d p1, Vector3d p2, Vector3d& r)
{
  Vector3d dir = p2 - p1;
  Vector3d res;
  Vector3d v1, v2;
  Vector3d z(0, 0, 1);
  v1 = node.dir.cross(dir);
  if (v1.norm() < 0.001) {
    v1 = Vector3d(dir[1], dir[2], dir[2]);
    v1 = node.dir.cross(v1);
  }
  v2 = node.dir.cross(v1);
  if (linsystem(dir, v1, v2, node.anchor - p1, res, nullptr)) return 1;

  if (res[0] < 0 || res[0] > 1) return 1;
  r = p1 + dir * res[0];
  return 0;
}
static void pullObject_addtri(PolySetBuilder& builder, Vector3d a, Vector3d b, Vector3d c)
{
  builder.beginPolygon(3);
  builder.addVertex(c);
  builder.addVertex(b);
  builder.addVertex(a);
}

static std::unique_ptr<PolySet> pullObject(const PullNode& node, const PolySet *ps)
{
  PolySetBuilder builder(0, 0, 3, true);
  auto ps_tess = PolySetUtils::tessellate_faces(*ps);
  for (unsigned int i = 0; i < ps_tess->indices.size(); i++) {
    auto& pol = ps_tess->indices[i];

    // count upper points
    int upper = 0;
    int lowind = 0;
    int highind = 0;

    for (int j = 0; j < 3; j++) {
      Vector3d pt = ps_tess->vertices[pol[j]];
      float dist = (pt - node.anchor).dot(node.dir);
      if (dist > 0) {
        upper++;
        highind += j;
      } else {
        lowind += j;
      }
    }
    switch (upper) {
    case 0:
      builder.beginPolygon(3);
      for (int j = 0; j < 3; j++) {
        builder.addVertex(ps_tess->vertices[pol[j]]);
      }
      break;
    case 1: {
      std::vector<int> pol1;
      pol1.push_back(pol[(highind + 1) % 3]);
      pol1.push_back(pol[(highind + 2) % 3]);
      pol1.push_back(pol[highind]);
      // pol1[2] ist oben
      //
      Vector3d p02, p12;
      if (pullObject_calccut(node, ps_tess->vertices[pol1[0]], ps_tess->vertices[pol1[2]], p02)) break;
      if (pullObject_calccut(node, ps_tess->vertices[pol1[1]], ps_tess->vertices[pol1[2]], p12)) break;

      pullObject_addtri(builder, ps_tess->vertices[pol1[0]], ps_tess->vertices[pol1[1]], p12);
      pullObject_addtri(builder, ps_tess->vertices[pol1[0]], p12, p02);

      pullObject_addtri(builder, p02, p12, p12 + node.dir);
      pullObject_addtri(builder, p02, p12 + node.dir, p02 + node.dir);

      pullObject_addtri(builder, p02 + node.dir, p12 + node.dir, ps_tess->vertices[pol1[2]] + node.dir);
    } break;
    case 2: {
      std::vector<int> pol1;
      pol1.push_back(pol[(lowind + 1) % 3]);
      pol1.push_back(pol[(lowind + 2) % 3]);
      pol1.push_back(pol[lowind]);
      // pol1[2] ist unten
      //
      Vector3d p02, p12;
      if (pullObject_calccut(node, ps_tess->vertices[pol1[0]], ps_tess->vertices[pol1[2]], p02)) break;
      if (pullObject_calccut(node, ps_tess->vertices[pol1[1]], ps_tess->vertices[pol1[2]], p12)) break;

      pullObject_addtri(builder, ps_tess->vertices[pol1[2]], p02, p12);

      pullObject_addtri(builder, p12, p02, p02 + node.dir);
      pullObject_addtri(builder, p12, p02 + node.dir, p12 + node.dir);

      pullObject_addtri(builder, p12 + node.dir, p02 + node.dir, ps_tess->vertices[pol1[0]] + node.dir);
      pullObject_addtri(builder, p12 + node.dir, ps_tess->vertices[pol1[0]] + node.dir,
                        ps_tess->vertices[pol1[1]] + node.dir);
    } break;
    case 3:
      builder.beginPolygon(3);
      for (int j = 0; j < 3; j++) {
        Vector3d pt = ps_tess->vertices[pol[2 - j]] + node.dir;
        builder.addVertex(pt);
      }
      break;
    }
  }

  return builder.build();
}

Vector3d cross_pt(Vector3d p1, Vector3d p2, double x)
{
  double f = (x - p1[0]) / (p2[0] - p1[0]);
  double y = p1[1] + (p2[1] - p1[1]) * f;
  double z = p1[2] + (p2[2] - p1[2]) * f;
  return Vector3d(x, y, z);
}

std::vector<std::vector<IndexedColorTriangle>> wrapSlice(PolySetBuilder& builder,
                                                         const std::vector<Vector3d> vertices,
                                                         const std::vector<IndexedColorFace>& polygons,
                                                         const std::vector<Vector4d>& normals,
                                                         std::vector<double> xsteps)
{
  std::vector<Vector3f> builder_vertices;
  std::vector<std::vector<IndexedColorTriangle>> results;  // nach strips sortiert
  int strips = xsteps.size() + 1;

  // initialize
  Polygon dmy;
  std::vector<Polygon> dmyx;
  std::vector<IndexedColorTriangle> dmyz;
  for (int i = 0; i < strips - 2; i++) {  // TODO check very carefully
    results.push_back(dmyz);
  }

  // sort in buckets of equal normal direction
  indexedFaceList emptyList;
  std::vector<Vector4d> norm_list;
  std::vector<int> color_list;
  std::vector<indexedFaceList> polygons_sorted;
  //  std::vector<Vector4d> normals = calcTriangleNormals(vertices, polygons);
  // sort polygons into buckets of same orientation
  for (unsigned int i = 0; i < polygons.size(); i++) {
    Vector4d norm = normals[i];
    const IndexedColorFace& triangle = polygons[i];

    int bucket_ind = -1;
    for (unsigned int j = 0; bucket_ind == -1 && j < norm_list.size(); j++) {
      const auto& cur = norm_list[j];
      if (triangle.color == color_list[j]) {
        if (cur.head<3>().dot(norm.head<3>()) > 0.99999 && fabs(cur[3] - norm[3]) < 0.001) {
          bucket_ind = j;
        }
        if (cur.norm() < 1e-6 && norm.norm() < 1e-6) bucket_ind = j;  // zero vector matches zero vector
      }
    }
    if (bucket_ind == -1) {
      bucket_ind = norm_list.size();
      norm_list.push_back(norm);
      color_list.push_back(triangle.color);
      polygons_sorted.push_back(emptyList);
    }
    polygons_sorted[bucket_ind].push_back(triangle.face);
  }

  int color_ind = 0;
  for (auto& bucket : polygons_sorted) {
    std::vector<std::vector<Polygon>> stripPolygons;  // nach strips sortiert
    std::vector<Polygon> stripTops;                   // obere Randpunkte eines Streifens
    std::vector<Polygon> stripBots;                   // obere Randpunkte eines Streifens
    std::vector<std::vector<Polygon>> tmpresults;     // nach strips sortiert
    for (int i = 0; i < strips; i++) {
      stripPolygons.push_back(dmyx);
      stripTops.push_back(dmy);
      stripBots.push_back(dmy);
      tmpresults.push_back(dmyx);
    }

    // now split all  input
    int cnt = 0;
    for (const auto& poly : bucket) {
      cnt++;
      int cutnum = 0;
      int n = poly.size();
      Polygon chain;
      if (n < 1) continue;
      Vector3d curpt = vertices[poly[n - 1]];
      chain.push_back(curpt);
      size_t curlevel = 0;  // 0 ebene0, 1, zwishen, 2, ebene1, 3 zwischen
      while ((curlevel >> 1) + 1 < xsteps.size() && curpt[0] > xsteps[(curlevel >> 1) + 1])
        curlevel += 2;
      if (curpt[0] == xsteps[(curlevel >> 1) + 1]) curlevel++;
      size_t curpols = stripPolygons[curlevel >> 1].size();
      for (int i = 0; i < n; i++) {
        Vector3d newpt = vertices[poly[i]];
        size_t newlevel = 0;
        while ((newlevel >> 1) + 1 < xsteps.size() && newpt[0] > xsteps[(newlevel >> 1) + 1])
          newlevel += 2;
        if (newpt[0] == xsteps[(newlevel >> 1) + 1]) newlevel++;

        while (curlevel != newlevel) {
          cutnum++;
          if (newlevel < curlevel) {  // down
            if (!(curlevel & 1))      // in die mitte
            {
              Vector3d ptx = cross_pt(curpt, newpt, xsteps[(curlevel >> 1)]);
              chain.push_back(ptx);
              stripPolygons[curlevel >> 1].push_back(chain);
              chain.clear();
              stripBots[curlevel >> 1].push_back(ptx);
              curlevel--;
            } else {  // aus der mitte)
              Vector3d ptx = cross_pt(curpt, newpt, xsteps[((curlevel + 1) >> 1)]);
              chain.push_back(ptx);
              curlevel--;
              stripTops[curlevel >> 1].push_back(ptx);
            }
          }
          if (newlevel > curlevel) {  // up
            if (!(curlevel & 1))      // in die mitte
            {
              Vector3d ptx = cross_pt(curpt, newpt, xsteps[(curlevel >> 1) + 1]);
              chain.push_back(ptx);
              stripPolygons[curlevel >> 1].push_back(chain);
              chain.clear();
              stripTops[curlevel >> 1].push_back(ptx);
              curlevel++;
            } else {  // aus der mitte
              Vector3d ptx = cross_pt(curpt, newpt, xsteps[((curlevel - 1) >> 1) + 1]);
              chain.push_back(ptx);
              curlevel++;
              stripBots[curlevel >> 1].push_back(ptx);
            }
          }
        }
        curpt = newpt;
        chain.push_back(curpt);
      }
      // patching
      if (cutnum == 0) {
        tmpresults[curlevel >> 1].push_back(chain);
      } else {
        if (chain.size() > 1 && curpols < stripPolygons[curlevel >> 1].size()) {
          stripPolygons[curlevel >> 1][curpols].insert(stripPolygons[curlevel >> 1][curpols].begin(),
                                                       chain.begin(), chain.end() - 1);
        }
      }
    }
    auto compare_func = [](const Vector3d& b, const Vector3d& a) {
      if (b[2] > a[2]) return 1.0;
      if (b[2] < a[2]) return -1.0;
      return b[1] - a[1];
    };

    for (int i = 0; i < strips; i++) {
      if (stripPolygons[i].size() == 0) continue;
      std::sort(stripBots[i].begin(), stripBots[i].end(), compare_func);
      std::sort(stripTops[i].begin(), stripTops[i].end(), compare_func);

      Polygon chain;
      Vector3d connpt(0, 0, 0);
      bool done = true;
      while (done) {
        done = false;
        if (chain.size() == 0) {
          if (stripPolygons[i].size() > 0) {
            chain = stripPolygons[i][0];
            stripPolygons[i].erase(stripPolygons[i].begin());
            connpt = chain[chain.size() - 1];
            done = true;
          }
        } else {
          for (size_t j = 0; j < stripPolygons[i].size(); j++) {
            if ((stripPolygons[i][j][0] - connpt).norm() < 1e-3) {
              chain.insert(chain.end(), stripPolygons[i][j].begin(), stripPolygons[i][j].end());
              stripPolygons[i].erase(stripPolygons[i].begin() + j);
              connpt = chain[chain.size() - 1];
              done = true;
              break;
            }
          }
        }
        for (size_t j = 0; j < stripBots[i].size(); j += 2) {
          if ((stripBots[i][j] - connpt).norm() < 1e-3) {
            connpt = stripBots[i][j + 1];
            stripBots[i].erase(stripBots[i].begin() + j, stripBots[i].begin() + j + 2);
            done = true;
            break;
          }
          if ((stripBots[i][j + 1] - connpt).norm() < 1e-3) {
            connpt = stripBots[i][j];
            stripBots[i].erase(stripBots[i].begin() + j, stripBots[i].begin() + j + 2);
            done = true;
            break;
          }
        }
        for (size_t j = 0; j < stripTops[i].size(); j += 2) {
          if ((stripTops[i][j + 1] - connpt).norm() < 1e-3) {
            connpt = stripTops[i][j];
            stripTops[i].erase(stripTops[i].begin() + j, stripTops[i].begin() + j + 2);
            done = true;
            break;
          }
          if ((stripTops[i][j] - connpt).norm() < 1e-3) {
            connpt = stripTops[i][j + 1];
            stripTops[i].erase(stripTops[i].begin() + j, stripTops[i].begin() + j + 2);
            done = true;
            break;
          }
        }
        if (chain.size() > 0 && connpt == chain[0]) {  // harvest
          tmpresults[i].push_back(chain);
          chain.clear();
        }
      }
      if (chain.size() != 0) {
        printf("Error A\n");
      }
      if (stripPolygons[i].size() != 0) {
        printf("Error B\n");
      }
      if (stripBots[i].size() != 0) {
        printf("Error C\n");
      }
      if (stripTops[i].size() != 0) {
        printf("Error D\n");
      }
      // now output
    }
    // TODO strukturen nicht so tief, datem frueher verarbeiten
    for (int i = 0; i < strips; i++) {
      // convert to indexed

      std::vector<IndexedFace> polys_ind;
      for (const auto& poly : tmpresults[i]) {
        IndexedFace poly_ind;
        for (const auto& pt : poly) {
          poly_ind.push_back(builder.vertexIndex(pt));
        }
        polys_ind.push_back(poly_ind);
      }

      builder.copyVertices(builder_vertices);  // TODO sehr ineffizient
      std::vector<IndexedTriangle> triangles;

      GeometryUtils::tessellatePolygonWithHoles(builder_vertices, polys_ind, triangles);

      for (const auto& tri : triangles) {
        IndexedColorTriangle tri_col(tri[0], tri[1], tri[2], color_list[color_ind]);
        results[i].push_back(tri_col);
      }
    }
    color_ind++;
  }
  return results;
}

static std::unique_ptr<PolySet> wrapObject(const WrapNode& node, const PolySet *ps)
{
  if (!Feature::ExperimentalWrapPolygon.is_enabled()) {
    PolySetBuilder builder(0, 0, 3, true);
    int segments1 = 360.0 / node.fa;
    int segments2 = 2 * G_PI * node.r / node.fs;
    int segments = segments1 > segments2 ? segments1 : segments2;
    if (node.fn > 0) segments = node.fn;
    double arclen = 2 * G_PI * node.r / segments;

    for (const auto& p : ps->indices) {
      // find leftmost point
      int n = p.size();
      int minind = 0;
      for (size_t j = 1; j < p.size(); j++) {
        if (ps->vertices[p[j]][0] < ps->vertices[p[minind]][0]) minind = j;
      }
      int forw_ind = minind;
      int back_ind = minind;
      double xcur, xnext;

      xcur = ps->vertices[p[minind]][0];
      std::vector<Vector3d> curslice;
      curslice.push_back(ps->vertices[p[minind]]);

      int end = 0;
      do {
        if (xcur >= 0) xnext = ceil((xcur + 1e-6) / arclen) * arclen;
        else xnext = -floor((-xcur + 1e-6) / arclen) * arclen;
        while (ps->vertices[p[(forw_ind + 1) % n]][0] <= xnext && ((forw_ind + 1) % n) != back_ind) {
          forw_ind = (forw_ind + 1) % n;
          curslice.push_back(ps->vertices[p[forw_ind]]);
        }
        while (ps->vertices[p[(back_ind + n - 1) % n]][0] <= xnext &&
               ((back_ind + n - 1) % n) != forw_ind) {
          back_ind = (back_ind + n - 1) % n;
          curslice.insert(curslice.begin(), ps->vertices[p[back_ind]]);
        }

        Vector3d forw_pt, back_pt;
        if (back_ind == ((forw_ind + 1) % n)) {
          end = 1;
        } else {
          // calculate intermediate forward point
          Vector3d tmp1, tmp2;

          tmp1 = ps->vertices[p[forw_ind]];
          tmp2 = ps->vertices[p[(forw_ind + 1) % n]];
          forw_pt = tmp1 + (tmp2 - tmp1) * (xnext - tmp1[0]) / (tmp2[0] - tmp1[0]);
          curslice.push_back(forw_pt);
          tmp1 = ps->vertices[p[back_ind]];
          tmp2 = ps->vertices[p[(back_ind + n - 1) % n]];
          back_pt = tmp1 + (tmp2 - tmp1) * (xnext - tmp1[0]) / (tmp2[0] - tmp1[0]);
          curslice.insert(curslice.begin(), back_pt);
        }

        double ang, rad;

        for (size_t j = 0; j < curslice.size(); j++) {
          auto& pt = curslice[j];
          ang = pt[0] / node.r;
          rad = node.r - pt[1];
          pt = Vector3d(rad * cos(ang), rad * sin(ang), pt[2]);
        }
        for (size_t j = 0; j < curslice.size() - 2; j++) {
          builder.beginPolygon(curslice.size());
          builder.addVertex(curslice[0]);
          builder.addVertex(curslice[j + 1]);
          builder.addVertex(curslice[j + 2]);
          builder.endPolygon();
        }
        // TODO color alpha
        curslice.clear();
        xcur = xnext;
        curslice.push_back(back_pt);
        curslice.push_back(forw_pt);
      } while (end == 0);
    }
    auto ps1 = builder.build();
    return ps1;
  }
  PolySetBuilder builder(0, 0, 3, true);

  // find maxmal xrange
  double xmin = NAN, xmax = NAN;
  for (const auto& p : ps->indices) {
    for (int i : p) {
      double x = ps->vertices[i][0];
      if (isnan(xmin)) {
        xmin = x;
        xmax = x;
      } else {
        if (x < xmin) xmin = x;
        if (x > xmax) xmax = x;
      }
    }
  }
  // now build scale form xmin to xmax
  std::vector<double> xscale;
  std::vector<Vector2d> polygon;
  int polygonlen;
  if (node.shape != nullptr) {
    Tree tree(node.shape, "");
    GeometryEvaluator geomevaluator(tree);
    std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
    std::shared_ptr<const Polygon2d> pol = std::dynamic_pointer_cast<const Polygon2d>(geom);
    if (pol != nullptr) {
      auto outlines = pol->untransformedOutlines();
      if (outlines.size() > 0) {
        const Outline2d outl = outlines[0];
        polygon = outl.vertices;
      }
    }
    if (polygon.size() == 0) return builder.build();
    polygonlen = polygon.size();
    double off = xmin;
    int ind = 0;
    xscale.push_back(off);
    do {
      auto& p1 = polygon[ind % polygonlen];
      auto& p2 = polygon[(ind + 1) % polygonlen];
      off += (p2 - p1).norm();
      ind++;
      xscale.push_back(off);
    } while (off < xmax);
  } else {  // r given
    int segments1 = 360.0 / node.fa;
    int segments2 = 2 * G_PI * node.r / node.fs;
    int segments = segments1 > segments2 ? segments1 : segments2;
    if (node.fn > 0) segments = node.fn;
    double arclen = 2 * G_PI * node.r / segments;
    //    if(xmin >= 0) xmin = ceil((xmin+1e-6)/arclen)*arclen;
    //    else xmin = -floor((-xmin+1e-6)/arclen)*arclen;
    do {
      Vector2d pt(node.r * cos(xmin / node.r), node.r * sin(xmin / node.r));

      xscale.push_back(xmin);
      polygon.push_back(pt);
      xmin += arclen;
    } while (xmin <= xmax + 1e-6 + arclen);
    polygonlen = polygon.size();
    xscale.push_back(xmin);
  }

  std::vector<indexedFaceList> polygons_sorted;
  std::vector<Vector4d> normals = calcTriangleNormals(ps->vertices, ps->indices);
  std::vector<int> faceParents;
  std::vector<Vector4d> newnormals;
  std::vector<IndexedColorFace> tri_color;
  for (size_t i = 0; i < ps->indices.size(); i++) {
    const auto& tri = ps->indices[i];
    if (i < ps->color_indices.size())
      tri_color.push_back(IndexedColorFace({.face = tri, .color = ps->color_indices[i]}));
    else tri_color.push_back(IndexedColorFace({.face = tri, .color = -1}));
  }
  std::vector<IndexedColorFace> tri_color_merged =
    mergeTriangles(tri_color, normals, newnormals, faceParents, ps->vertices);

  // now build vector from xmin to xmax
  std::vector<std::vector<IndexedColorTriangle>> sliceresult =
    wrapSlice(builder, ps->vertices, tri_color_merged, newnormals, xscale);

  std::vector<Vector3d> builder_vertices;
  builder.copyVertices(builder_vertices);  // TODO sehr ineffizient
  int ind = 0;
  for (const auto& slice : sliceresult) {
    double xbot = xscale[ind];
    double xtop = xscale[ind + 1];
    for (const auto& poly : slice) {
      Polygon ptrans;
      builder.beginPolygon(3);
      for (int j = 0; j < 3; j++) {
        Vector3d pt = builder_vertices[poly[j]];

        auto& p0 = polygon[ind > 1 ? ind - 1 : 0];
        auto& p1 = polygon[ind];
        auto& p2 = polygon[ind < polygonlen - 1 ? ind + 1 : polygonlen - 1];
        auto& p3 = polygon[ind < polygonlen - 2 ? ind + 2 : polygonlen - 1];

        Vector2d dir0 = (p1 - p0).normalized();
        Vector2d dir0n = Vector2d(-dir0[1], dir0[0]);

        Vector2d dir1u = (p2 - p1);
        Vector2d dir1 = dir1u.normalized();
        Vector2d dir1n = Vector2d(-dir1[1], dir1[0]);

        Vector2d dir2 = (p3 - p2).normalized();
        Vector2d dir2n = Vector2d(-dir2[1], dir2[0]);

        Vector2d dirn = dir0n * (xtop - pt[0]) / (xtop - xbot) + dir2n * (pt[0] - xbot) / (xtop - xbot);
        dirn = (dirn + dir1n).normalized();

        Vector2d px =
          p1 + dir1u * (pt[0] - xscale[ind]) / (xscale[ind + 1] - xscale[ind]) + dirn * pt[1];
        Vector3d pt_tran = Vector3d(px[0], px[1], pt[2]);
        pt_tran[0] = concat_round(pt_tran[0]);
        pt_tran[1] = concat_round(pt_tran[1]);
        pt_tran[2] = concat_round(pt_tran[2]);

        builder.addVertex(pt_tran);
      }
      if (poly[3] != -1) builder.endPolygon(ps->colors[poly[3]]);
      else builder.endPolygon();
    }
    ind++;
  }
  auto ps1 = builder.build();
  return ps1;
}

Response GeometryEvaluator::visit(State& state, const PullNode& node)
{
  std::shared_ptr<const Geometry> newgeom;
  std::shared_ptr<const Geometry> geom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (geom) {
    std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);
    if (ps != nullptr) {
      std::unique_ptr<Geometry> ps_pulled = pullObject(node, ps.get());
      newgeom = std::move(ps_pulled);
      addToParent(state, node, newgeom);
      node.progress_report();
    }
  }
  return Response::ContinueTraversal;
}

static std::unique_ptr<PolySet> debugObject(const DebugNode& node, const PolySet *ps)
{
  auto psx = std::make_unique<PolySet>(ps->getDimension(), ps->convexValue());
  *psx = *ps;

  if (psx->color_indices.size() < psx->indices.size()) {
    auto cs = ColorMap::inst()->defaultColorScheme();
    Color4f def_color = ColorMap::getColor(cs, RenderColor::OPENCSG_FACE_FRONT_COLOR);
    int defind = psx->colors.size();
    psx->colors.push_back(def_color);
    while (psx->color_indices.size() < psx->indices.size()) {
      psx->color_indices.push_back(defind);
    }
  }
  Color4f debug_color = Color4f(255, 0, 0, 255);
  int colorind = psx->colors.size();
  psx->colors.push_back(debug_color);
  for (size_t i = 0; i < node.faces.size(); i++) {
    size_t ind = (size_t)node.faces[i];
    if (ind >= 0 && ind < psx->color_indices.size()) psx->color_indices[ind] = colorind;
  }

  return psx;
}

Response GeometryEvaluator::visit(State& state, const DebugNode& node)
{
  std::shared_ptr<const Geometry> newgeom;
  std::shared_ptr<const Geometry> geom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (geom) {
    std::shared_ptr<const PolySet> ps = nullptr;
    if (std::shared_ptr<const ManifoldGeometry> mani =
          std::dynamic_pointer_cast<const ManifoldGeometry>(geom))
      ps = mani->toPolySet();
    else ps = std::dynamic_pointer_cast<const PolySet>(geom);
    if (ps != nullptr) {
      std::unique_ptr<Geometry> ps_pulled = debugObject(node, ps.get());
      newgeom = std::move(ps_pulled);
      addToParent(state, node, newgeom);
      node.progress_report();
    }
  }
  return Response::ContinueTraversal;
}

static std::unique_ptr<PolySet> repairObject(const RepairNode& node, const PolySet *ps)
{
  auto psx = std::make_unique<PolySet>(ps->getDimension(), ps->convexValue());
  *psx = *ps;

  int color_ind = -1;
  if (node.color.isValid()) {
    auto it = std::find(psx->colors.begin(), psx->colors.end(), node.color);
    if (it == psx->colors.end()) {
      color_ind = psx->colors.size();
      psx->colors.push_back(node.color);
    } else {
      color_ind = it - psx->colors.begin();
    }
  }

  indexedFaceList defects = mergeTrianglesSub(ps->indices, ps->vertices);
  for (size_t i = 0; i < defects.size(); i++) {
    auto face = defects[i];
    auto fence = defects[i];
    IndexedFace fence_new;
    int cnt = 0;
    int ind = 0;
    int l = face.size();
    while (l >= 3) {
      const Vector3d& p0 = ps->vertices[face[(ind + 0) % l]];
      const Vector3d& p1 = ps->vertices[face[(ind + 1) % l]];
      const Vector3d& p2 = ps->vertices[face[(ind + 2) % l]];
      Vector3d n = (p1 - p0).cross(p2 - p1);
      // check that all vertices are below the plane
      bool valid = true;
      for (auto ind : fence) {
        const auto& pt = ps->vertices[ind];
        double dist = (p1 - pt).dot(n);
        if (dist > 1e-3) {
          valid = false;
          break;
        }
      }
      if (valid) {
        IndexedFace patch;
        patch.push_back(face[(ind + 0) % l]);
        patch.push_back(face[(ind + 1) % l]);
        patch.push_back(face[(ind + 2) % l]);
        psx->indices.push_back(patch);

        if (color_ind != -1) {
          psx->color_indices.resize(psx->indices.size() - 1, -1);
          psx->color_indices.push_back(color_ind);
        }

        fence_new.push_back(face[(ind + 1) % l]);
        face.erase(face.begin() + ((ind + 1) % l));
        l--;
        cnt = 0;
      } else ind = (ind + 1) % l;
      cnt++;
      if (cnt > l) {
        if (fence_new.size() == 0) break;
        fence = fence_new;
        fence_new.clear();
      }
    }
    if (face.size() > 0) {
      printf("Recovery\n");
      // find smallest distance between 2 pts
      int imin = -1, jmin = -1;
      double distmin = 1e9;
      for (int i = 0; i < l - 1; i++) {
        const auto& p1 = ps->vertices[face[i]];
        const auto& p1n = ps->vertices[face[(i + 1) % l]];
        const auto& p1d = (p1n - p1).normalized();
        for (int j = i + 1; j < l; j++) {
          const auto& p2 = ps->vertices[face[j]];
          const auto& p2n = ps->vertices[face[(j + 1) % l]];
          const auto& p2d = (p2n - p2).normalized();
          double dist = (p1 - p2).norm();
          if (fabs(p1d.dot((p2 - p1).normalized())) > 0.5)
            continue;  // shortcut must be somehow perpendicular
          if (fabs(p2d.dot((p2 - p1).normalized())) > 0.5)
            continue;  // shortcut must be somehow perpendicular
          if (dist < distmin) {
            imin = i;
            jmin = j;
            distmin = dist;
          }
        }
      }
      printf("min is %d/%d %g\n", imin, jmin, distmin);
      // now build 2 parts
      if (imin != -1) {
        IndexedFace part1, part2;
        for (int i = 0; i <= imin; i++) part1.push_back(face[i]);
        for (int i = jmin; i < l; i++) part1.push_back(face[i]);
        for (int i = imin; i < jmin; i++) part2.push_back(face[i]);
        // just skip the current one and schedule 2 new
        defects.push_back(part1);
        defects.push_back(part2);
      }
    }
  }

  return psx;
}

Response GeometryEvaluator::visit(State& state, const RepairNode& node)
{
  std::shared_ptr<const Geometry> newgeom;
  std::shared_ptr<const Geometry> geom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (geom) {
    std::shared_ptr<const PolySet> ps = nullptr;
    if (std::shared_ptr<const ManifoldGeometry> mani =
          std::dynamic_pointer_cast<const ManifoldGeometry>(geom))
      ps = mani->toPolySet();
    else ps = std::dynamic_pointer_cast<const PolySet>(geom);
    if (ps != nullptr) {
      std::unique_ptr<Geometry> ps_pulled = repairObject(node, ps.get());
      newgeom = std::move(ps_pulled);
      addToParent(state, node, newgeom);
      node.progress_report();
    }
  }
  return Response::ContinueTraversal;
}

Response GeometryEvaluator::visit(State& state, const WrapNode& node)
{
  std::shared_ptr<const Geometry> newgeom;
  std::shared_ptr<const Geometry> geom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (geom) {
    std::shared_ptr<const PolySet> ps = std::dynamic_pointer_cast<const PolySet>(geom);
    if (ps != nullptr) {
    } else ps = PolySetUtils::getGeometryAsPolySet(geom);
    if (ps != nullptr) {
      std::unique_ptr<Geometry> ps_wrapped = wrapObject(node, ps.get());
      newgeom = std::move(ps_wrapped);
      addToParent(state, node, newgeom);
      node.progress_report();
    }
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
