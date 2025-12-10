#include "geometry/Barcode1d.h"

#include <sstream>
#include <utility>
#include <cstddef>
#include <string>
#include <memory>

#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "utils/printutils.h"
#include "Feature.h"
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/manifoldutils.h"
#endif
#include "geometry/cgal/cgalutils.h"
#include "Feature.h"
#include "geometry/PolySet.h"
#include "glview/RenderSettings.h"

Barcode1d::Barcode1d(Edge1d edge) : sanitized(true) { addEdge(std::move(edge)); }

std::unique_ptr<Geometry> Barcode1d::copy() const { return std::make_unique<Barcode1d>(*this); }

BoundingBox Edge1d::getBoundingBox() const
{
  BoundingBox bbox;
  bbox.extend(Vector3d(begin, 0, 0));
  bbox.extend(Vector3d(end, 0, 0));
  // Note: this uses ->edge, and so automatically gets trans3d applied
  return bbox;
}

/*!
   Class for holding 2D geometry.

   This class will hold 2D geometry consisting of a number of closed
   polygons. Each polygon can contain holes and islands. Both polygons,
   holes and island contours may intersect each other.

   We can store sanitized vs. unsanitized polygons. Sanitized polygons
   will have opposite winding order for holes and is guaranteed to not
   have intersecting geometry. The winding order will be counter-clockwise
   for positive edge and clockwise for holes. Sanitization is typically
   done by ClipperUtils, but if you create geometry which you know is sanitized,
   the flag can be set manually.
 */

size_t Barcode1d::memsize() const
{
  size_t mem = 0;
  mem += 2 * this->theedges.size() * sizeof(double);
  mem += 2 * this->trans3dEdges.size() * sizeof(double);
  mem += sizeof(Barcode1d);
  return mem;
}

BoundingBox Barcode1d::getBoundingBox() const
{
  BoundingBox bbox;
  for (const auto& o : this->edges()) {
    bbox.extend(o.getBoundingBox());
  }
  return bbox;
}

std::string Barcode1d::dump() const
{
  std::ostringstream out;
  for (const auto& o : this->theedges) {
    out << "contour:\n";
    out << "  " << o.begin << " " << o.end;
    out << "\n";
  }
  if (trans3dState != Transform3dState::NONE) {
    out << "with trans3d: [";
    for (int j = 0; j < 4; j++) {
      out << "[";
      for (int i = 0; i < 4; i++) {
        double v(this->trans3d(j, i));
        out << v;
        if (i != 3) out << ", ";
      }
      out << "]";
      if (j != 3) out << ", ";
    }
    out << "]\n";
  }
  return out.str();
}

bool Barcode1d::isEmpty() const { return this->theedges.empty(); }

void Barcode1d::transform(const Transform2d& mat)
{
  if (mat.matrix().determinant() == 0) {
    LOG(message_group::Warning, "Scaling a 2D object with 0 - removing object");
    this->theedges.clear();
    trans3dState = Transform3dState::NONE;
    return;
  }
  if (trans3dState != Transform3dState::NONE) mergeTrans3d();
  for (auto& o : this->theedges) {
    o.begin = mat(0, 0) * o.begin + mat(0, 2);
    o.end = mat(0, 0) * o.end + mat(0, 2);
  }
}
std::shared_ptr<Polygon2d> Barcode1d::to2d(void) const
{
  Polygon2d result;
  for (auto e : untransformedEdges()) {
    Vector2d v1(e.begin, -0.25);
    Vector2d v2(e.begin, 0.25);
    Vector2d v3(e.end, 0.25);
    Vector2d v4(e.end, -0.25);

    Outline2d o;
    o.color = e.color;
    o.vertices = {v1, v2, v3, v4};
    result.addOutline(o);
  }
  return std::make_shared<Polygon2d>(result);
}
void Barcode1d::resize(const Vector2d& newsize, const Eigen::Matrix<bool, 2, 1>& autosize)
{
  auto bbox = this->getBoundingBox();

  // Find largest dimension
  int maxdim = (newsize[1] && newsize[1] > newsize[0]) ? 1 : 0;

  // Default scale (scale with 1 if the new size is 0)
  Vector2d scale(newsize[0] > 0 ? newsize[0] / bbox.sizes()[0] : 1,
                 newsize[1] > 0 ? newsize[1] / bbox.sizes()[1] : 1);

  // Autoscale where applicable
  double autoscale = newsize[maxdim] > 0 ? newsize[maxdim] / bbox.sizes()[maxdim] : 1;
  Vector2d newscale(!autosize[0] || (newsize[0] > 0) ? scale[0] : autoscale,
                    !autosize[1] || (newsize[1] > 0) ? scale[1] : autoscale);

  Transform2d t;
  t.matrix() << newscale[0], 0, 0, 0, newscale[1], 0, 0, 0, 1;

  this->transform(t);
}

bool Barcode1d::is_convex() const { return true; }

double Barcode1d::area() const { return 0; }

/*!
   Triangulates this polygon2d and returns a 2D-in-3D PolySet.

   This is used for various purposes:
   * Geometry evaluation for roof, linear_extrude, rotate_extrude
   * Rendering (both preview and render mode)
   * Polygon area calculation
   *
   * One use-case is special: For geometry construction in Manifold mode, we require this function to
   * guarantee that vertices and their order are untouched (apart from adding a zero 3rd dimension)
   *
 */
void Barcode1d::transform3d(const Transform3d& mat)
{
  // Check whether it can be a 2d transform, and avoid the 3d overhead
  if (trans3dState == Transform3dState::NONE && mat(2, 0) == 0 && mat(2, 1) == 0 && mat(2, 2) == 1 &&
      mat(2, 3) == 0 && mat(0, 2) == 0 && mat(1, 2) == 0 && mat(3, 2) == 0) {
    Transform2d t;
    t.matrix() << mat(0, 0), mat(0, 1), mat(0, 3), mat(1, 0), mat(1, 1), mat(1, 3), mat(3, 0), mat(3, 1),
      mat(3, 3);
    if (t.matrix().determinant() == 0) {
      LOG(message_group::Warning, "Scaling a 2D object with 0 - removing object");
      this->theedges.clear();
      trans3dState = Transform3dState::NONE;
      return;
    }
    transform(t);
    // A 2D transformation may flip the winding order of a polygon.
    // If that happens with a sanitized polygon, we need to reverse
    // the winding order for it to be correct.
    //    if (sanitized && t.matrix().determinant() < 0)
    //      for (auto &o : this->theedges)
    //        std::reverse(o.vertices.begin(), o.vertices.end());
  } else {
    if (mat.matrix().determinant() == 0) {
      LOG(message_group::Warning, "Scaling a 2D object with 0 - removing object");
      this->theedges.clear();
      trans3dState = Transform3dState::NONE;
      return;
    }
    trans3d = (trans3dState == Transform3dState::NONE) ? mat : mat * trans3d;
    trans3dState = Transform3dState::PENDING;
  }
}

void Barcode1d::setColor(const Color4f& c)
{
  for (auto& e : this->theedges) {
    e.color = c;
  }
}

// This returns the edges after applying any Transform3d that might be Transform3dState::PENDING.
// If there is no Transform3d, this returns the edges vector.
// If there is a Transform3dState::CACHED Transform3d, this uses the cache.
// Else it creates and returns the cache
const Barcode1d::Edges1d& Barcode1d::transformedEdges() const
{
  if (trans3dState == Transform3dState::NONE) return theedges;
  /*
  if (trans3dState != Transform3dState::CACHED) {
    // Need to remove const from the cache object.  It maintains proper const semantics to the public API
though. Barcode1d::Edges2d &cache= const_cast<Barcode1d::Edges2d&>(trans3dEdges); cache= theedges;
    applyTrans3dToEdges(cache);
// TODO    const_cast<Barcode1d*>(this)->trans3dState= Transform3dState::CACHED;
  }
  */
  return trans3dEdges;
}
// This flattens the 3D transform into the 2D transform that it would have been
// originally.
void Barcode1d::mergeTrans3d()
{
  if (trans3dState == Transform3dState::CACHED) theedges.swap(trans3dEdges);
  else if (trans3dState == Transform3dState::PENDING) applyTrans3dToEdges(theedges);
  trans3dEdges.clear();
  trans3dState = Transform3dState::NONE;
}
void Barcode1d::applyTrans3dToEdges(Barcode1d::Edges1d& edges) const
{
  Transform2d t;
  t.matrix() << trans3d(0, 0), trans3d(0, 1), trans3d(0, 3), trans3d(1, 0), trans3d(1, 1), trans3d(1, 3),
    trans3d(3, 0), trans3d(3, 1), trans3d(3, 3);
  for (auto& o : edges) {
    o.begin = trans3d(0, 0) * o.begin;
    o.end = trans3d(0, 0) * o.end;
  }
  // A 2D transformation may flip the winding order of a polygon.
  // If that happens with a sanitized polygon, we need to reverse
  // the winding order for it to be correct.
  if (sanitized && t.matrix().determinant() < 0)
    for (auto& o : edges) {
      double tmp = o.begin;
      o.begin = o.end;
      o.end = tmp;
    }
}
