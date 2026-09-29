#include "geometry/patch.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Eigen/Eigenvalues>

#include "geometry/Polygon2d.h"
#include "utils/printutils.h"

namespace {

// -------------------- Small 2D helpers --------------------

// Signed area of a closed 2D polygon (positive = CCW, negative = CW).
double signedArea2D(const std::vector<Vector2d>& poly)
{
  double area = 0.0;
  const size_t n = poly.size();
  for (size_t i = 0; i < n; i++) {
    const Vector2d& a = poly[i];
    const Vector2d& b = poly[(i + 1) % n];
    area += a.x() * b.y() - b.x() * a.y();
  }
  return 0.5 * area;
}

// Reverses a ring's 2D points, its parallel 3D points, and (if present) its
// parallel tangent vectors together, so the vertex-by-vertex correspondence
// between uv[i], pos3d[i] and normal[i] is preserved.
void reverseRingInPlace(std::vector<Vector2d>& uv, std::vector<Vector3d>& pos3d,
                        std::vector<Vector3d>& normal)
{
  std::reverse(uv.begin(), uv.end());
  std::reverse(pos3d.begin(), pos3d.end());
  if (!normal.empty()) std::reverse(normal.begin(), normal.end());
}

// Forces 'outer' to be wound CCW and every entry of 'holes' to be wound CW
// (the standard convention for a polygon-with-holes), keeping the parallel
// 3D point arrays (and, when present, tangent arrays) in sync with any
// reversal.
void ensureOrientation(std::vector<Vector2d>& outer_uv, std::vector<Vector3d>& outer_pos,
                       std::vector<Vector3d>& outer_normal, std::vector<std::vector<Vector2d>>& holes_uv,
                       std::vector<std::vector<Vector3d>>& holes_pos,
                       std::vector<std::vector<Vector3d>>& holes_normal)
{
  if (signedArea2D(outer_uv) < 0) reverseRingInPlace(outer_uv, outer_pos, outer_normal);
  for (size_t hi = 0; hi < holes_uv.size(); hi++) {
    if (holes_uv[hi].size() < 3) continue;
    if (signedArea2D(holes_uv[hi]) > 0) {
      static std::vector<Vector3d> emptyNormal;
      std::vector<Vector3d>& hn = (hi < holes_normal.size()) ? holes_normal[hi] : emptyNormal;
      reverseRingInPlace(holes_uv[hi], holes_pos[hi], hn);
    }
  }
}

// -------------------- Domain polygon (outer ring + holes) --------------------
// Builds a Polygon2d from the projected boundary rings so we can reuse the
// existing, hole-aware Polygon2d::point_inside().
Polygon2d buildDomainPolygon(const std::vector<Vector2d>& outer_uv,
                             const std::vector<std::vector<Vector2d>>& holes_uv)
{
  Polygon2d poly;

  Outline2d outerOutline;
  outerOutline.vertices.assign(outer_uv.begin(), outer_uv.end());
  outerOutline.positive = true;
  poly.addOutline(outerOutline);

  for (const auto& h : holes_uv) {
    Outline2d holeOutline;
    holeOutline.vertices.assign(h.begin(), h.end());
    holeOutline.positive = false;
    poly.addOutline(holeOutline);
  }
  return poly;
}

// -------------------- Distance to the domain boundary --------------------
// Shortest distance from p to a single closed polyline (ring): the minimum
// distance to any of its edges.
double distanceToRing(const Vector2d& p, const std::vector<Vector2d>& ring)
{
  double best = std::numeric_limits<double>::max();
  const size_t n = ring.size();
  for (size_t i = 0; i < n; i++) {
    const Vector2d& a = ring[i];
    const Vector2d& b = ring[(i + 1) % n];
    Vector2d ab = b - a;
    double len2 = ab.squaredNorm();
    double t = (len2 > 1e-18) ? std::clamp((p - a).dot(ab) / len2, 0.0, 1.0) : 0.0;
    Vector2d closest = a + ab * t;
    best = std::min(best, (p - closest).norm());
  }
  return best;
}

// Shortest distance from p to the domain boundary, i.e. to the nearest
// point on the outer ring or on any hole ring.
double distanceToBoundary(const Vector2d& p, const std::vector<Vector2d>& outer_uv,
                          const std::vector<std::vector<Vector2d>>& holes_uv)
{
  double best = distanceToRing(p, outer_uv);
  for (const auto& h : holes_uv) best = std::min(best, distanceToRing(p, h));
  return best;
}

double smoothstep01(double x)
{
  x = std::clamp(x, 0.0, 1.0);
  return x * x * (3.0 - 2.0 * x);
}

// -------------------- Boundary-edge encroachment test --------------------
// A point p "encroaches" on a segment (a,b) if it lies inside that
// segment's diametral circle - the (smallest possible) circle that passes
// through a and b, centered at their midpoint. This is a standard,
// precisely justified test from constrained-Delaunay mesh generation
// (Ruppert/Chew): if a segment's diametral circle contains no other point,
// that segment is *guaranteed* to survive as an edge of the (unconstrained)
// Delaunay triangulation. Checking it reduces to a single sign: p sees a
// and b at an obtuse angle, i.e. (a-p).(b-p) < 0.
//
// This is used below to keep interior grid points from being placed close
// enough to any boundary-ring edge to threaten it - unlike a flat distance
// threshold, it scales automatically with each edge's actual local length,
// so it doesn't over-exclude where the ring is coarse and under-exclude
// where it's fine.
//
// The strict "< 0.0" misses one real case: p sitting exactly ON a or b
// itself, i.e. an interior/collar point landing exactly on top of an
// existing boundary vertex (not just inside its edge's diametral circle).
// There (a-p) or (b-p) is the zero vector, so the dot product is exactly
// 0.0 - not negative - even though that point obviously should have been
// excluded (it produces a literal duplicate-vertex, zero-area triangle,
// which is exactly what this check exists to prevent). This is not a rare
// edge case in practice: a periodic tube's interior grid always starts its
// u-columns exactly AT the domain's own seam coordinate (see
// triangulatePeriodic()'s seam handling), which is the same coordinate a
// ring's own seam-crossing vertex gets clamped to by wrapToPeriod() - for
// evenly-spaced rings that land the seam exactly on one of their own
// vertices (as a symmetric hole layout easily can), grid column 0 then
// coincides EXACTLY with that ring vertex, every time, not just as a
// numerical near-miss. A tiny epsilon makes the test inclusive of its own
// boundary instead of missing it.
bool pointEncroachesEdge(const Vector2d& p, const Vector2d& a, const Vector2d& b)
{
  return (a - p).dot(b - p) < 1e-9;
}

bool pointEncroachesBoundary(const Vector2d& p, const std::vector<Vector2d>& outer_uv,
                             const std::vector<std::vector<Vector2d>>& holes_uv)
{
  auto checkRing = [&](const std::vector<Vector2d>& ring) {
    const size_t n = ring.size();
    for (size_t i = 0; i < n; i++) {
      if (pointEncroachesEdge(p, ring[i], ring[(i + 1) % n])) return true;
    }
    return false;
  };
  if (checkRing(outer_uv)) return true;
  for (const auto& h : holes_uv) {
    if (checkRing(h)) return true;
  }
  return false;
}

// -------------------- Cubic (Hermite-style) triangle patch --------------------
//
// Boundary points (outer ring + hole rings) keep their EXACT input 3D
// position - this never changes, whether or not tangents are supplied (see
// the "boundary points keep their exact input position" comment further
// down). What tangents change is only how the INTERIOR of each boundary
// triangle is curved between those fixed corner points.
//
// This is deliberately NOT classic PN-Triangles: PN-Triangles take N as a
// *surface normal* and project the edge vector onto the tangent plane
// (i.e. they DISCARD the component of (Pj-Pi) along N). Here, N is the
// *tangent/leaving direction* of the surface at a boundary ring (e.g. the
// axis a circular port's wall leaves that port along) - exactly the
// direction PN-Triangles would throw away. So instead we build ordinary
// cubic Hermite control points directly from that tangent.
//
// A ring-id per corner point tells us whether an edge runs between two
// points of the SAME ring (in which case the tangent must NOT bend it -
// use the original straight/linear control points) or between two
// DIFFERENT rings (outer <-> hole), in which case the two tangents pull
// the edge's near-corner control points forward/backward respectively.
//
// The ring-id ordering convention (smaller ring id = "outer side", larger
// = "hole side") together with a caller-side global sign-normalization
// pass (see normalizeTangentSigns() below) is what keeps "leaving A" vs
// "arriving at B" consistent, independent of the arbitrary vertex order
// Bowyer-Watson happens to store a triangle's corners in.

void edgeControlPoints(const Vector3d& Pi, const Vector3d& Pj, const Vector3d& Ti, const Vector3d& Tj,
                       int ringI, int ringJ, Vector3d& outNearI, Vector3d& outNearJ)
{
  if (ringI == ringJ) {
    // Same ring: keep the original straight/linear control points so the
    // ring's own shape is never bent by a tangent meant for a *different*
    // boundary.
    outNearI = Pi + (Pj - Pi) / 3.0;
    outNearJ = Pj + (Pi - Pj) / 3.0;
    return;
  }
  const double L = (Pj - Pi).norm();
  if (ringI < ringJ) {
    outNearI = Pi + Ti * (L / 3.0);
    outNearJ = Pj - Tj * (L / 3.0);
  } else {
    outNearI = Pi - Ti * (L / 3.0);
    outNearJ = Pj + Tj * (L / 3.0);
  }
}

Vector3d evalCubicTriangle(const Vector3d& P1, const Vector3d& P2, const Vector3d& P3,
                           const Vector3d& N1, const Vector3d& N2, const Vector3d& N3, int ring1,
                           int ring2, int ring3, double u, double v, double w)
{
  const Vector3d b300 = P1, b030 = P2, b003 = P3;
  Vector3d b210, b120, b021, b012, b102, b201;

  edgeControlPoints(P1, P2, N1, N2, ring1, ring2, b210, b120);
  edgeControlPoints(P2, P3, N2, N3, ring2, ring3, b021, b012);
  edgeControlPoints(P3, P1, N3, N1, ring3, ring1, b102, b201);

  const Vector3d E = (b210 + b120 + b021 + b012 + b102 + b201) / 6.0;
  const Vector3d V = (P1 + P2 + P3) / 3.0;
  const Vector3d b111 = E + (E - V) * 0.5;

  const double u2 = u * u, v2 = v * v, w2 = w * w, u3 = u2 * u, v3 = v2 * v, w3 = w2 * w;
  return b300 * u3 + b030 * v3 + b003 * w3 + 3 * b210 * u2 * v + 3 * b120 * u * v2 + 3 * b021 * v2 * w +
         3 * b012 * v * w2 + 3 * b102 * w2 * u + 3 * b201 * w * u2 + 6 * b111 * u * v * w;
}

// -------------------- Base surface: barycentric/cubic base position + normal --------------------
// Gives any interior (u,v) query point a well-defined base 3D position and
// face normal, via a coarse triangulation of the boundary points. If
// per-point tangents were supplied, the interior is curved with
// evalCubicTriangle(); otherwise it falls back to the original flat
// barycentric blend, unchanged.
struct LoftBaseSurface {
  std::vector<Vector2d> uv;      // 2D coordinates (boundary points only: outer + holes)
  std::vector<Vector3d> pos3d;   // parallel 3D positions
  std::vector<Vector3d> normal;  // parallel tangent/leaving directions; may be all-zero (unused)
  std::vector<int> ringId;       // parallel ring id: 0 = outer, i+1 = holes[i]
  std::vector<std::array<int, 3>> tris;

  bool sample(const Vector2d& p, Vector3d& outPos, Vector3d& outNormal) const
  {
    for (const auto& t : tris) {
      const Vector2d& a = uv[t[0]];
      const Vector2d& b = uv[t[1]];
      const Vector2d& c = uv[t[2]];
      double denom = (b.y() - c.y()) * (a.x() - c.x()) + (c.x() - b.x()) * (a.y() - c.y());
      if (std::fabs(denom) < 1e-12) continue;
      double w0 = ((b.y() - c.y()) * (p.x() - c.x()) + (c.x() - b.x()) * (p.y() - c.y())) / denom;
      double w1 = ((c.y() - a.y()) * (p.x() - c.x()) + (a.x() - c.x()) * (p.y() - c.y())) / denom;
      double w2 = 1.0 - w0 - w1;
      const double eps = -1e-9;
      if (w0 < eps || w1 < eps || w2 < eps) continue;

      const Vector3d& p0 = pos3d[t[0]];
      const Vector3d& p1 = pos3d[t[1]];
      const Vector3d& p2 = pos3d[t[2]];

      const bool haveTangents =
        !normal.empty() && (normal[t[0]].squaredNorm() > 1e-12 || normal[t[1]].squaredNorm() > 1e-12 ||
                            normal[t[2]].squaredNorm() > 1e-12);

      if (!haveTangents) {
        outPos = w0 * p0 + w1 * p1 + w2 * p2;
      } else {
        outPos = evalCubicTriangle(p0, p1, p2, normal[t[0]], normal[t[1]], normal[t[2]], ringId[t[0]],
                                   ringId[t[1]], ringId[t[2]], w0, w1, w2);
      }

      Vector3d n = (p1 - p0).cross(p2 - p0);
      double nlen = n.norm();
      outNormal = (nlen > 1e-12) ? (n / nlen) : Vector3d(0, 0, 1);
      return true;
    }
    return false;
  }
};

// Cheap deterministic hash-noise in [0,1), used only to jitter points fed
// into bowyerWatson() (see below).
double hashNoise01(double seed)
{
  double s = std::sin(seed * 127.1 + 311.7) * 43758.5453;
  return s - std::floor(s);
}

// Returns a copy of 'pts' with a tiny deterministic per-point offset. This
// is ONLY meant to be fed into bowyerWatson() as topology input - the
// original, unperturbed coordinates are what everything else (domain
// membership tests, the final vertex positions) keeps using.
//
// WHY THIS IS NEEDED: an axis-aligned regular grid (which is exactly what
// the interior sampling below produces) is a classic pathological input for
// Bowyer-Watson - every 2x2 grid cell's four corners are exactly cocircular,
// which can corrupt the incremental algorithm's "bad-triangle cavity is a
// single closed loop" assumption. A small perturbation, well above floating
// point noise but far below anything visible in the final geometry, avoids
// this - a standard trick for making incremental Delaunay implementations
// robust against structured input.
//
// 'keys', if given, must be the same length as 'pts' and supplies the hash
// seed for each point INSTEAD OF its array position. This matters for
// triangulatePeriodic()'s ghosted point lists: a point near the domain's
// u-edge appears there TWICE - once at its real coordinate, once shifted by
// +-period - and the whole ghosting trick depends on that pair staying
// EXACTLY 'period' apart, since that is what lets a triangle built from one
// copy get correctly recognized, after re-mapping, as the same edge a
// triangle from the other copy also uses. Jittering by array position gives
// the two copies two unrelated random offsets, silently breaking that exact
// spacing - independently of any of this function's own geometry, and only
// visible as an occasional non-manifold edge (multiplicity 3 instead of 1)
// right at whichever ring vertex happens to sit exactly on the seam.
// Keying the jitter by each point's ORIGINAL (pre-ghosting) index instead
// gives every copy of the same source point the identical offset, so the
// pair stays exactly 'period' apart and the seam recovery stays exact. The
// non-periodic call site below has no duplicate points to keep in sync, so
// it simply omits 'keys' and keeps the old per-position behavior.
std::vector<Vector2d> jitterForDelaunay(const std::vector<Vector2d>& pts, double domainScale,
                                        const std::vector<int>& keys = {})
{
  std::vector<Vector2d> out;
  out.reserve(pts.size());
  const double mag = std::max(domainScale, 1e-9) * 1e-5;
  for (size_t i = 0; i < pts.size(); i++) {
    const double key = keys.empty() ? static_cast<double>(i) : static_cast<double>(keys[i]);
    double jx = (hashNoise01(key * 2.0) - 0.5) * mag;
    double jy = (hashNoise01(key * 2.0 + 1.0) - 0.5) * mag;
    out.push_back(pts[i] + Vector2d(jx, jy));
  }
  return out;
}

// -------------------- Unconstrained Bowyer-Watson Delaunay --------------------
//
// Each triangle also carries the index of its neighbor across each of its
// three edges (-1 = none, i.e. the outer void beyond the super-triangle).
// This adjacency is what lets point insertion below avoid ever scanning the
// full triangle list (see the long comment on bowyerWatson() itself).
struct DTriangle {
  int a, b, c;
  int nab = -1, nbc = -1, nca = -1;  // neighbor across edge (a,b), (b,c), (c,a)
};

// True if point p lies strictly inside the circumcircle of triangle (a,b,c).
// A free function (no mesh state needed) so it can be shared between
// bowyerWatson()'s own per-triangle test and its constrained-edge recovery
// pass below, which evaluates candidate triangles that aren't part of the
// mesh (yet, or at all).
bool pointInCircumcircle(const Vector2d& a, const Vector2d& b, const Vector2d& c, const Vector2d& p)
{
  double ax = a.x() - p.x(), ay = a.y() - p.y();
  double bx = b.x() - p.x(), by = b.y() - p.y();
  double cx = c.x() - p.x(), cy = c.y() - p.y();
  double det = (ax * ax + ay * ay) * (bx * cy - cx * by) - (bx * bx + by * by) * (ax * cy - cx * ay) +
               (cx * cx + cy * cy) * (ax * by - bx * ay);
  return det > 1e-12;
}

// Delaunay triangulation of a plain 2D point set. 'pts' is left unmodified;
// returned indices refer into 'pts'.
//
// 'constraints' is an optional list of (pts-)index pairs that MUST end up
// as real edges of the returned mesh, however the unconstrained Delaunay
// insertion below would otherwise have triangulated that area - see the
// long comment above the constrained-edge recovery pass, further down in
// this function, for why that matters and how it's done.
//
// PERFORMANCE: the original implementation of this function found, for
// every inserted point, both (a) which triangles are "bad" (p lies inside
// their circumcircle) and (b) which of a bad triangle's edges are shared
// with another bad triangle, by scanning the ENTIRE current triangle list
// (and, for (b), the entire bad list again per edge) - O(n) work per point,
// O(n^2) overall, the single biggest cost in the whole patch() pipeline for
// any nontrivial point count (confirmed by profiling the tube-with-holes
// case, which is by far the largest point set this file builds).
//
// Neither of those scans is actually necessary. This version instead:
//   1. Maintains triangle-to-triangle adjacency (the n?? fields above),
//      updated incrementally as triangles are created/retired.
//   2. Locates a triangle CONTAINING the new point via a "visibility walk":
//      starting from a hint (the triangle created for the previous point -
//      spatially close for every caller in this file, since points are
//      always fed in as contiguous rings or row-by-row grids), repeatedly
//      cross whichever edge the point lies on the far side of. A triangle
//      containing p is always itself "bad" (p inside a triangle implies p
//      inside that triangle's own circumcircle), so this both locates AND
//      seeds the bad region in one step - expected O(sqrt(n)) or better for
//      spatially coherent insertion order, instead of O(n).
//   3. Flood-fills the rest of the bad region outward from that seed via
//      adjacency (a bad triangle's neighbor is only worth visiting if it is
//      ALSO bad), instead of testing every triangle in the mesh - cost
//      proportional to the (small, roughly constant-size) bad region itself.
//   4. Walks the bad region's boundary edges as a single ordered loop (using
//      each edge's known adjacency, not a nested O(bad^2) shared-edge scan)
//      to fan new triangles around the point and relink their neighbors.
//
// A full-scan fallback (walk failed, or landed on a dead end / numerical
// tie) is kept for robustness - it only ever fires for the rare point where
// the hint-based walk doesn't pan out, not on every insertion.
std::vector<DTriangle> bowyerWatson(const std::vector<Vector2d>& pts,
                                    const std::vector<std::pair<int, int>>& constraints = {})
{
  if (pts.size() < 3) return {};

  // Defensive bound: a healthy triangulation of N points has ~2N LIVE
  // triangles (retired/"dead" ones are never removed from the underlying
  // vector - see 'dead' below - so this tracks a running live count rather
  // than the vector's own size, which also includes dead entries).
  const size_t maxSaneTris = std::max<size_t>(200, pts.size() * 20);

  double minX = pts[0].x(), maxX = minX, minY = pts[0].y(), maxY = minY;
  for (const auto& p : pts) {
    minX = std::min(minX, p.x());
    maxX = std::max(maxX, p.x());
    minY = std::min(minY, p.y());
    maxY = std::max(maxY, p.y());
  }
  double dmax = std::max(maxX - minX, maxY - minY);
  if (dmax < 1e-9) dmax = 1.0;
  double midx = (minX + maxX) * 0.5, midy = (minY + maxY) * 0.5;

  std::vector<Vector2d> P = pts;
  int i0 = static_cast<int>(P.size());
  P.push_back(Vector2d(midx - 20 * dmax, midy - dmax));
  int i1 = static_cast<int>(P.size());
  P.push_back(Vector2d(midx, midy + 20 * dmax));
  int i2 = static_cast<int>(P.size());
  P.push_back(Vector2d(midx + 20 * dmax, midy - dmax));

  auto makeCCW = [&](DTriangle t) {
    double area = (P[t.b].x() - P[t.a].x()) * (P[t.c].y() - P[t.a].y()) -
                  (P[t.c].x() - P[t.a].x()) * (P[t.b].y() - P[t.a].y());
    if (area < 0) std::swap(t.b, t.c);
    return t;
  };

  auto inCircumcircle = [&](const DTriangle& t, const Vector2d& p) {
    return pointInCircumcircle(P[t.a], P[t.b], P[t.c], p);
  };

  // > 0 iff p is strictly to the left of the directed edge a->b - i.e. on
  // the "inside" side for a CCW triangle whose edge this is.
  auto orient = [&](int a, int b, const Vector2d& p) {
    return (P[b].x() - P[a].x()) * (p.y() - P[a].y()) - (p.x() - P[a].x()) * (P[b].y() - P[a].y());
  };

  std::vector<DTriangle> tris;
  std::vector<char> dead;
  std::vector<int> visitedStamp;
  size_t liveCount = 0;

  auto addTriangle = [&](DTriangle t) -> int {
    tris.push_back(t);
    dead.push_back(0);
    visitedStamp.push_back(-1);
    liveCount++;
    return static_cast<int>(tris.size()) - 1;
  };

  addTriangle(makeCCW({i0, i1, i2}));

  // Visibility walk: from 'start', repeatedly cross whichever edge p lies
  // outside of, towards p. Returns the (live) triangle containing p, or -1
  // if the walk hits a dead end (dead triangle, or a boundary edge with no
  // neighbor to cross to while still outside) - the caller falls back to a
  // full scan in that case.
  auto locate = [&](int start, const Vector2d& p) -> int {
    if (start < 0 || static_cast<size_t>(start) >= tris.size()) return -1;
    int t = start;
    const int stepLimit = static_cast<int>(tris.size()) + 8;
    for (int steps = 0; steps < stepLimit; steps++) {
      if (t < 0 || dead[t]) return -1;
      const DTriangle& tri = tris[t];
      if (orient(tri.a, tri.b, p) < -1e-12) {
        if (tri.nab < 0) return -1;
        t = tri.nab;
        continue;
      }
      if (orient(tri.b, tri.c, p) < -1e-12) {
        if (tri.nbc < 0) return -1;
        t = tri.nbc;
        continue;
      }
      if (orient(tri.c, tri.a, p) < -1e-12) {
        if (tri.nca < 0) return -1;
        t = tri.nca;
        continue;
      }
      return t;
    }
    return -1;
  };

  int hint = 0;
  int stamp = 0;
  std::vector<int> bad;
  std::vector<int> stack;
  struct BEdge {
    int u, v, ext;
  };
  std::vector<BEdge> bedges;

  for (int pi = 0; pi < static_cast<int>(pts.size()); pi++) {
    const Vector2d& p = P[pi];

    int seed = locate(hint, p);
    if (seed < 0 || !inCircumcircle(tris[seed], p)) {
      seed = -1;
      for (size_t t = 0; t < tris.size(); t++) {
        if (!dead[t] && inCircumcircle(tris[t], p)) {
          seed = static_cast<int>(t);
          break;
        }
      }
      if (seed < 0) continue;  // point lies exactly on an existing circumcircle etc. - skip it
    }

    // Flood-fill the bad region outward from 'seed' via adjacency.
    stamp++;
    bad.clear();
    stack.clear();
    stack.push_back(seed);
    visitedStamp[seed] = stamp;
    while (!stack.empty()) {
      int t = stack.back();
      stack.pop_back();
      bad.push_back(t);
      const DTriangle& tri = tris[t];
      const int nb[3] = {tri.nab, tri.nbc, tri.nca};
      for (int n : nb) {
        if (n < 0 || dead[n] || visitedStamp[n] == stamp) continue;
        if (inCircumcircle(tris[n], p)) {
          visitedStamp[n] = stamp;
          stack.push_back(n);
        }
      }
    }
    auto isBad = [&](int t) { return t >= 0 && visitedStamp[t] == stamp; };

    // Boundary edges of the (star-shaped) bad region, each directed as it
    // appears in its own owning bad triangle (CCW), together with the
    // external (non-bad) triangle - if any - to relink there.
    bedges.clear();
    for (int t : bad) {
      const DTriangle& tri = tris[t];
      if (!isBad(tri.nab)) bedges.push_back({tri.a, tri.b, tri.nab});
      if (!isBad(tri.nbc)) bedges.push_back({tri.b, tri.c, tri.nbc});
      if (!isBad(tri.nca)) bedges.push_back({tri.c, tri.a, tri.nca});
    }

    // Chain the boundary edges into one or more ordered loops by vertex
    // adjacency (edge (u,v) is followed by an edge starting at v) -
    // O(boundary length) via a start-vertex lookup, no nested scan.
    //
    // A numerically healthy bad region's boundary is a single simple loop,
    // but a highly regular/cocircular input (see jitterForDelaunay() - an
    // axis-aligned grid is exactly that) can occasionally still produce a
    // SELF-TOUCHING ("pinched") boundary: two sub-loops sharing one
    // vertex, i.e. some vertex is the start of MORE than one boundary
    // edge. An earlier version of this code assumed a single loop (one
    // unordered_map<int,int> from start-vertex to edge, which can only
    // remember ONE edge per start vertex) and silently DROPPED the point
    // being inserted whenever that assumption failed - invisibly losing
    // whatever point happened to trigger a pinched bad region, which is
    // exactly what a regular gridded interior sample (patchTubeWithHoles's
    // uniform (u,v) grid) is prone to. A dropped point leaves its would-be
    // neighbors triangulated around a small gap instead - precisely the
    // "phantom hole with no corresponding real hole" defect this was
    // tracked down from.
    //
    // Handling multiple (possibly pinched) loops instead is straightforward
    // once you stop assuming there is only one: repeatedly take any not-yet-
    // used boundary edge as a new loop's start and follow the chain until it
    // closes, using a MULTImap so a vertex that starts several edges can
    // still be walked (picking any one of its still-unused outgoing edges
    // keeps the walk single-valued at each step; which one is picked doesn't
    // matter for correctness, only which sub-loop a given edge ends up
    // grouped into). Each resulting loop is independently star-shaped around
    // p, so fanning each one separately - below - is exactly as valid as
    // fanning a single simple loop always was; a shared pinch vertex simply
    // ends up used by fan triangles from more than one loop, same as any
    // ordinary shared mesh vertex.
    std::unordered_multimap<int, int> byStart;
    byStart.reserve(bedges.size() * 2);
    for (size_t i = 0; i < bedges.size(); i++) byStart.emplace(bedges[i].u, static_cast<int>(i));

    std::vector<char> used(bedges.size(), 0);
    std::vector<std::vector<BEdge>> boundaryLoops;
    bool chainFailed = false;
    for (size_t startI = 0; startI < bedges.size(); startI++) {
      if (used[startI]) continue;
      std::vector<BEdge> loop;
      loop.reserve(bedges.size());
      int cur = static_cast<int>(startI);
      const int startU = bedges[startI].u;
      size_t guard = 0;
      while (true) {
        if (used[cur]) {
          chainFailed = true;
          break;
        }
        used[cur] = 1;
        loop.push_back(bedges[cur]);
        const int nextU = bedges[cur].v;
        if (nextU == startU) break;
        int found = -1;
        auto range = byStart.equal_range(nextU);
        for (auto it = range.first; it != range.second; ++it) {
          if (!used[it->second]) {
            found = it->second;
            break;
          }
        }
        if (found < 0) {
          chainFailed = true;
          break;
        }
        cur = found;
        if (++guard > bedges.size() + 4) {
          chainFailed = true;
          break;
        }
      }
      if (chainFailed) break;
      boundaryLoops.push_back(std::move(loop));
    }
    size_t loopedEdges = 0;
    for (const auto& l : boundaryLoops) loopedEdges += l.size();
    if (chainFailed || loopedEdges != bedges.size()) {
      // Still couldn't decompose the boundary into closed loops at all -
      // skip this point rather than risk building a corrupt mesh (should be
      // rarer still than the single-loop case this replaces, and was always
      // the fallback behavior here).
      continue;
    }

    for (int t : bad) dead[t] = 1;
    liveCount -= bad.size();

    auto setNeighborForEdge = [&](DTriangle& t, int va, int vb, int nbIdx) {
      if ((t.a == va && t.b == vb) || (t.a == vb && t.b == va)) t.nab = nbIdx;
      else if ((t.b == va && t.c == vb) || (t.b == vb && t.c == va)) t.nbc = nbIdx;
      else t.nca = nbIdx;
    };

    // Fan new triangles around p, one per boundary edge, independently per
    // loop, and relink each one's three neighbors: the external triangle
    // across its outer edge (found by matching vertex identity - robust
    // regardless of which of a/b/c order that triangle happens to store
    // them in), and its two fan-neighbors, found directly from its own
    // loop's cyclic order.
    int firstNewIdx = -1;
    for (const auto& loop : boundaryLoops) {
      const int k = static_cast<int>(loop.size());
      std::vector<int> newIdx(k);
      for (int i = 0; i < k; i++) newIdx[i] = addTriangle(makeCCW({loop[i].u, loop[i].v, pi}));

      for (int i = 0; i < k; i++) {
        DTriangle& nt = tris[newIdx[i]];
        const int prev = (i - 1 + k) % k;
        const int next = (i + 1) % k;
        setNeighborForEdge(nt, loop[i].u, loop[i].v, loop[i].ext);
        setNeighborForEdge(nt, loop[i].v, pi, newIdx[next]);
        setNeighborForEdge(nt, pi, loop[i].u, newIdx[prev]);

        if (loop[i].ext >= 0) setNeighborForEdge(tris[loop[i].ext], loop[i].v, loop[i].u, newIdx[i]);
      }
      if (firstNewIdx < 0) firstNewIdx = newIdx[0];
    }

    hint = firstNewIdx;
    if (liveCount > maxSaneTris) return {};
  }

  // -------- Constrained edge recovery --------
  //
  // The insertion loop above produces an UNCONSTRAINED Delaunay
  // triangulation: it is free to pick whichever edges are locally
  // Delaunay-legal, with no knowledge that some of the input points form
  // boundary rings whose consecutive points MUST end up connected by an
  // actual mesh edge - concat() and a neighboring patch() call both rely
  // on that boundary polyline being reproduced exactly, unmodified, so
  // their shared points line up. For a concave or otherwise irregular
  // ring, another point (from the same ring, a different ring, or the
  // interior grid) can legitimately fall inside a boundary edge's
  // diametral circle, so the unconstrained triangulation quietly
  // "shortcuts" past it instead - silently breaking that promise.
  // Centroid-based domain filtering (triangulateAndFilterToDomain()) does
  // not fix this: it only decides which side of a boundary survives, it
  // cannot restore an edge the triangulation never built in the first
  // place.
  //
  // Each caller-supplied 'constraints' pair is recovered here by brute
  // force: find every current mesh edge (p,q) that segment (u,v) properly
  // crosses, remove the (at most two) triangles bordering each such edge,
  // and re-triangulate the resulting "channel" polygon on either side of
  // (u,v) so (u,v) itself becomes a real mesh edge. This only runs for the
  // (typically small) set of ring segments that weren't already
  // Delaunay-legal, not for every edge, and - unlike the insertion loop
  // above - does not need triangle adjacency: at the point counts patch()
  // deals with, a handful of full scans over the current triangle list is
  // cheap next to the O(n) insertion work already done.
  for (const auto& c : constraints) {
    const int u = c.first;
    const int v = c.second;
    if (u == v) continue;
    const Vector2d& pu = P[u];
    const Vector2d& pv = P[v];

    // Already present as some triangle's edge?
    bool exists = false;
    for (size_t t = 0; t < tris.size() && !exists; t++) {
      if (dead[t]) continue;
      const DTriangle& tt = tris[t];
      const bool hasU = (tt.a == u || tt.b == u || tt.c == u);
      const bool hasV = (tt.a == v || tt.b == v || tt.c == v);
      if (hasU && hasV) exists = true;
    }
    if (exists) continue;

    // Every live edge that (u,v) properly crosses (both endpoints
    // strictly on opposite sides of it, and the crossing point strictly
    // between u and v) - remembering each crossed edge's parameter along
    // (u,v) is enough to recover correct visit order without walking
    // triangle adjacency at all.
    struct Crossing {
      int p, q;
      double t;
    };
    std::vector<Crossing> crossings;
    std::set<std::pair<int, int>> seenEdges;
    const Vector2d duv = pv - pu;

    for (size_t ti = 0; ti < tris.size(); ti++) {
      if (dead[ti]) continue;
      const DTriangle& tt = tris[ti];
      const int verts[3] = {tt.a, tt.b, tt.c};
      for (int e = 0; e < 3; e++) {
        const int p = verts[e];
        const int q = verts[(e + 1) % 3];
        if (p == u || p == v || q == u || q == v)
          continue;  // touches an endpoint - not a proper crossing
        const std::pair<int, int> key = (p < q) ? std::make_pair(p, q) : std::make_pair(q, p);
        if (seenEdges.count(key)) continue;

        // Segment-segment intersection: pu + t*duv == P[p] + s*(P[q]-P[p]).
        const Vector2d dpq = P[q] - P[p];
        const double denom = duv.x() * dpq.y() - duv.y() * dpq.x();
        if (std::fabs(denom) < 1e-14) continue;  // parallel - can't properly cross
        const Vector2d w = P[p] - pu;
        const double t = (w.x() * dpq.y() - w.y() * dpq.x()) / denom;
        const double s = (w.x() * duv.y() - w.y() * duv.x()) / denom;
        const double eps = 1e-9;
        if (t <= eps || t >= 1 - eps || s <= eps || s >= 1 - eps)
          continue;  // not a proper interior crossing

        seenEdges.insert(key);
        crossings.push_back({p, q, t});
      }
    }
    // (u,v) doesn't cross anything live - leave it. Shouldn't normally
    // happen once 'exists' is false, but a numerically borderline case is
    // safer left unenforced than forced.
    if (crossings.empty()) continue;

    // Every triangle touching a crossed edge lies inside the channel to
    // be rebuilt.
    std::set<int> removeSet;
    for (const auto& cr : crossings) {
      for (size_t ti = 0; ti < tris.size(); ti++) {
        if (dead[ti]) continue;
        const DTriangle& tt = tris[ti];
        const bool hasP = (tt.a == cr.p || tt.b == cr.p || tt.c == cr.p);
        const bool hasQ = (tt.a == cr.q || tt.b == cr.q || tt.c == cr.q);
        if (hasP && hasQ) removeSet.insert(static_cast<int>(ti));
      }
    }

    // Order crossings by how far along (u,v) they sit, then split their
    // endpoints into the two chains either side of the segment - this
    // directly gives each chain in correct geometric order without
    // needing to walk triangle adjacency at all.
    std::sort(crossings.begin(), crossings.end(),
              [](const Crossing& a, const Crossing& b) { return a.t < b.t; });

    std::vector<int> upper = {u}, lower = {u};
    std::vector<char> added(P.size(), 0);
    added[u] = added[v] = 1;
    bool degenerate = false;
    for (const auto& cr : crossings) {
      for (int vx : {cr.p, cr.q}) {
        if (added[vx]) continue;
        added[vx] = 1;
        const Vector2d d = P[vx] - pu;
        const double side = duv.x() * d.y() - duv.y() * d.x();
        if (side > 1e-12) upper.push_back(vx);
        else if (side < -1e-12) lower.push_back(vx);
        else degenerate = true;  // a vertex numerically on (u,v) itself - bail below
      }
    }
    // A vertex landing (numerically) exactly on the segment, or either
    // chain ending up without a single interior vertex despite triangles
    // being marked for removal, means the corridor wasn't the simple
    // shape this recovery assumes - safer to leave this one edge
    // unenforced than to risk tearing a hole in the mesh.
    if (degenerate || upper.size() < 2 || lower.size() < 2) continue;
    upper.push_back(v);
    lower.push_back(v);

    for (int t : removeSet) {
      dead[t] = 1;
      liveCount--;
    }

    // Re-triangulate each chain: recursively split on the vertex whose
    // circumcircle with the chain's current two ends excludes every other
    // vertex still in the chain - the standard way to retriangulate the
    // "pseudo-polygon" left behind by removing the triangles a segment
    // crosses. Every recursive call adds a triangle carrying (u,v) itself
    // (directly, or via one of its own sub-edges further down), so (u,v)
    // ends up a real mesh edge regardless of which split point is chosen;
    // the fallback below (no split vertex passes the empty-circumcircle
    // test) only affects triangle shape, never correctness.
    std::function<void(const std::vector<int>&, int, int)> triangulateChain =
      [&](const std::vector<int>& chain, int lo, int hi) {
        if (hi - lo < 2) return;
        int best = lo + 1;
        if (hi - lo > 2) {
          for (int m = lo + 1; m < hi; m++) {
            bool ok = true;
            for (int k = lo + 1; k < hi && ok; k++) {
              if (k == m) continue;
              if (pointInCircumcircle(P[chain[lo]], P[chain[m]], P[chain[hi]], P[chain[k]])) ok = false;
            }
            if (ok) {
              best = m;
              break;
            }
          }
        }
        addTriangle(makeCCW({chain[lo], chain[best], chain[hi]}));
        triangulateChain(chain, lo, best);
        triangulateChain(chain, best, hi);
      };
    triangulateChain(upper, 0, static_cast<int>(upper.size()) - 1);
    triangulateChain(lower, 0, static_cast<int>(lower.size()) - 1);

    if (liveCount > maxSaneTris) return {};
  }

  std::vector<DTriangle> result;
  result.reserve(liveCount);
  for (size_t t = 0; t < tris.size(); t++) {
    if (dead[t]) continue;
    const DTriangle& tt = tris[t];
    if (tt.a < i0 && tt.b < i0 && tt.c < i0) result.push_back(tt);
  }
  return result;
}

// Runs bowyerWatson() on a jittered copy of 'pts' (recovering 'constraints'
// as real mesh edges - see bowyerWatson()'s own comment on that), then
// keeps only the triangles whose centroid is inside 'domain'. Shared
// between the base surface (boundary points only) and the final mesh
// (boundary + interior grid points) below - both are "triangulate
// everything, then discard what's outside the domain" in exactly the same
// way, and both need their boundary rings' segments to survive intact.
std::vector<DTriangle> triangulateAndFilterToDomain(
  const std::vector<Vector2d>& pts, const Polygon2d& domain, double domainScale,
  const std::vector<std::pair<int, int>>& constraints = {})
{
  auto jittered = jitterForDelaunay(pts, domainScale);
  auto tris = bowyerWatson(jittered, constraints);

  std::vector<DTriangle> kept;
  kept.reserve(tris.size());
  for (const auto& t : tris) {
    Vector2d centroid = (pts[t.a] + pts[t.b] + pts[t.c]) / 3.0;
    if (domain.point_location(centroid) < PointLocation2d::Inside) continue;
    kept.push_back(t);
  }
  return kept;
}

// Builds the consecutive-point-pair constraints for 'outer' followed by
// each ring of 'holes', in exactly the point order LoftBaseSurface's
// 'base.uv' and the final mesh's 'allUV' both lay their boundary points
// out in (outer first, then each hole in turn) - so this one list of
// index pairs is valid for triangulating either point set, as long as
// interior points (which both callers only ever append AFTER all
// boundary points) are added later.
std::vector<std::pair<int, int>> buildRingConstraints(const std::vector<Vector2d>& outer_uv,
                                                      const std::vector<std::vector<Vector2d>>& holes_uv)
{
  std::vector<std::pair<int, int>> constraints;
  auto addRing = [&](size_t base, size_t n) {
    if (n < 2) return;
    for (size_t i = 0; i < n; i++) {
      constraints.emplace_back(static_cast<int>(base + i), static_cast<int>(base + (i + 1) % n));
    }
  };
  size_t offset = outer_uv.size();
  addRing(0, outer_uv.size());
  for (const auto& h : holes_uv) {
    addRing(offset, h.size());
    offset += h.size();
  }
  return constraints;
}

// Centroid of a list of 3D points (used only to establish a consistent
// global sign convention for tangents - see normalizeTangentSigns()).
Vector3d centroid3D(const std::vector<Vector3d>& pts)
{
  Vector3d c(0, 0, 0);
  for (const auto& p : pts) c += p;
  return pts.empty() ? c : c / static_cast<double>(pts.size());
}

// Makes sure every supplied tangent points, on balance, from the outer ring
// towards the (first) hole ring - i.e. all point "outer -> hole", not some
// forward and some backward. Without this, a tangent supplied by the
// caller in an arbitrary orientation (e.g. the plane normal of a 2D shape,
// which has no inherent "forward" vs "backward") can flip the curvature
// direction unpredictably from one boundary point to the next, producing
// self-intersecting surfaces instead of a simple bend.
//
// This is a coarse, whole-ring heuristic (single flip per ring based on the
// ring's average tangent vs. the outer->hole axis), not a per-point fix -
// it assumes each ring's tangent field is already roughly consistent with
// itself (true for a tangent derived from a single rigid 2D shape
// transform, as produced by python_patch_ring_from_shape()).
void normalizeTangentSigns(const std::vector<Vector3d>& outer_pos, std::vector<Vector3d>& outer_normal,
                           const std::vector<std::vector<Vector3d>>& holes_pos,
                           std::vector<std::vector<Vector3d>>& holes_normal)
{
  if (holes_pos.empty()) return;

  const Vector3d outerC = centroid3D(outer_pos);
  const Vector3d holeC = centroid3D(holes_pos[0]);
  Vector3d axis = holeC - outerC;
  if (axis.squaredNorm() < 1e-12) return;
  axis.normalize();

  auto flipRingIfNeeded = [&](std::vector<Vector3d>& ringNormal) {
    if (ringNormal.empty()) return;
    Vector3d avg(0, 0, 0);
    for (const auto& n : ringNormal) avg += n;
    if (avg.dot(axis) < 0) {
      for (auto& n : ringNormal) n = -n;
    }
  };

  flipRingIfNeeded(outer_normal);
  for (size_t hi = 0; hi < holes_normal.size(); hi++) {
    flipRingIfNeeded(holes_normal[hi]);
  }
}

// -------------------- Automatic projection --------------------
//
// Builds a proj() perpendicular to the given axis (unit vector): u,v are
// the point's coordinates in the plane orthogonal to 'axis'. Same
// construction as the user's earlier hand-written make_axial_proj(), just
// in C++ and derived automatically instead of from two named points.
std::function<Vector2d(const Vector3d&)> makeAxialProjection(Vector3d axis)
{
  axis.normalize();
  Vector3d ref = (std::fabs(axis.y()) < 0.9) ? Vector3d(0, 1, 0) : Vector3d(1, 0, 0);
  Vector3d u_axis = axis.cross(ref).normalized();
  Vector3d v_axis = axis.cross(u_axis);  // already unit: axis, u_axis orthonormal
  return
    [u_axis, v_axis](const Vector3d& p) -> Vector2d { return Vector2d(p.dot(u_axis), p.dot(v_axis)); };
}

// Fits the best plane through 'pts' (least-squares, via the covariance
// matrix's eigenvectors) and returns a proj() onto it: u,v are the
// coordinates along the two directions of largest spread; the direction of
// smallest spread (the fitted plane's own normal) is dropped. Correct
// choice when outer and holes are all roughly coplanar (a flat panel with
// cutouts) - looking straight through the panel's own thickness.
std::function<Vector2d(const Vector3d&)> makePlaneProjection(const std::vector<Vector3d>& pts)
{
  Vector3d c = centroid3D(pts);
  Eigen::Matrix3d cov = Eigen::Matrix3d::Zero();
  for (const auto& p : pts) {
    Vector3d d = p - c;
    cov += d * d.transpose();
  }
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(cov);
  // Eigen returns eigenvalues/-vectors sorted ASCENDING: col(0) = smallest
  // variance (the panel's own normal, to be dropped), col(2)/col(1) = the
  // two largest-variance (in-plane) directions we keep as u/v.
  Vector3d u_axis = solver.eigenvectors().col(2);
  Vector3d v_axis = solver.eigenvectors().col(1);
  return [c, u_axis, v_axis](const Vector3d& p) -> Vector2d {
    Vector3d d = p - c;
    return Vector2d(d.dot(u_axis), d.dot(v_axis));
  };
}

// -------------------- Degenerate-hole detection & local re-embedding --------------------
//
// A hole ring's projected shape can become degenerate (near-zero extent in
// one direction) if its own boundary curve's natural spread doesn't align
// with whatever directions the *shared* projection happens to keep - e.g.
// a small hole drilled radially through a tube wall, when the tube itself
// is (correctly, for the tube's own sake) projected top-down, dropping
// exactly the axial coordinate the hole's own extent depends on. This is
// not fixable by picking a "smarter" shared projection: a rotationally
// symmetric outer/tube-partner ring and an arbitrarily oriented hole
// fundamentally need different 2D treatment - see the discussion that led
// here.
//
// The fix: only the TOPOLOGY actually matters for the Delaunay+domain-
// filter approach below - which points bound which loop, and that loops
// don't overlap - not that every ring share one single global projection
// formula. So each hole is projected with the shared 'effectiveProj' as
// usual, and only if THAT comes out degenerate, it is instead flattened
// with its OWN local best-fit plane (same PCA technique as
// makePlaneProjection(), just scoped to this one hole's own points) and
// then placed at the position its centroid *would* have under the shared
// projection - a single point is never degenerate, so this placement is
// always well-defined even when the hole's full extent isn't.

// True if 'uv's spread is too "flat" in one direction to form a sane
// simple polygon: computes the 2x2 covariance matrix's eigenvalues
// directly (no need to pull in Eigen for a 2x2) and checks their ratio
// against 'aspectThreshold' (the minor/major VARIANCE ratio - since
// variance scales with length squared, a desired minor/major LENGTH ratio
// of e.g. 20% corresponds to an aspectThreshold of 0.2*0.2 = 0.04).
// Defaults to a strict near-zero threshold, tight enough to only catch
// genuinely degenerate (essentially zero-area) shapes - callers that also
// want to catch a merely "too thin to look reasonable" shape (not
// literally zero-area) pass a larger threshold explicitly.
bool isDegenerate2D(const std::vector<Vector2d>& uv, double aspectThreshold = 0.02)
{
  if (uv.size() < 3) return true;
  Vector2d c(0, 0);
  for (const auto& p : uv) c += p;
  c /= static_cast<double>(uv.size());

  double sxx = 0, syy = 0, sxy = 0;
  for (const auto& p : uv) {
    Vector2d d = p - c;
    sxx += d.x() * d.x();
    syy += d.y() * d.y();
    sxy += d.x() * d.y();
  }
  const double tr = sxx + syy;
  const double det = sxx * syy - sxy * sxy;
  const double disc = std::sqrt(std::max(0.0, tr * tr - 4 * det));
  const double lambdaMax = (tr + disc) * 0.5;
  const double lambdaMin = (tr - disc) * 0.5;
  if (lambdaMax < 1e-18) return true;
  return (lambdaMin / lambdaMax) < aspectThreshold;
}

// Locally flattens 'pts' (a single hole ring's own 3D boundary points) via
// its own best-fit plane, then places the result so its centroid sits at
// 'placeAt' - wherever the *shared* projection maps this hole's centroid
// to (see the long comment above). makePlaneProjection() already centers
// its output on the input points' own centroid, so 'placeAt' is simply
// added on top - no extra centroid bookkeeping needed here.
std::vector<Vector2d> localHoleReembed(const std::vector<Vector3d>& pts, const Vector2d& placeAt)
{
  auto localProj = makePlaneProjection(pts);
  std::vector<Vector2d> out;
  out.reserve(pts.size());
  for (const auto& p : pts) out.push_back(localProj(p) + placeAt);
  return out;
}

// True if any vector in 'normals' carries real tangent data (as opposed to
// being absent, or present but filled with placeholder zero vectors - which
// is how some callers represent "no tangents supplied" instead of an
// actually-empty array). Mirrors the same "haveTangents" test
// LoftBaseSurface::sample() already uses per-triangle, just applied to a
// whole array up front - so a caller-side representation quirk can't by
// itself block the tube-unroll fast path below.
bool hasAnyTangent(const std::vector<Vector3d>& normals)
{
  for (const auto& n : normals) {
    if (n.squaredNorm() > 1e-12) return true;
  }
  return false;
}

bool hasAnyHoleTangent(const std::vector<std::vector<Vector3d>>& holesNormal)
{
  for (const auto& hn : holesNormal) {
    if (hasAnyTangent(hn)) return true;
  }
  return false;
}

// Finds the "tube partner" hole - the single FARTHEST hole from the outer
// ring's own centroid, same heuristic used by computeAutoProj() below - and
// its axis direction. Shared by computeAutoProj() (for the ordinary
// top-down axial case) and the tube-with-extra-holes path further down
// (which needs to know the axis and which hole is the genuine tube partner
// *before* committing to a projection style). Returns false when nothing
// qualifies as tube-like.
bool findTubeAxis(const std::vector<Vector3d>& outer, const std::vector<std::vector<Vector3d>>& holes,
                  int& outIdx, Vector3d& outAxis, double& outR)
{
  const Vector3d outerCentroid = centroid3D(outer);
  double R = 0.0;
  for (const auto& p : outer) R = std::max(R, (p - outerCentroid).norm());
  if (R < 1e-9) R = 1.0;
  outR = R;

  double maxD = 0.0;
  outIdx = -1;
  Vector3d axis(0, 0, 0);
  for (size_t hi = 0; hi < holes.size(); hi++) {
    if (holes[hi].empty()) continue;
    Vector3d holeCentroid = centroid3D(holes[hi]);
    Vector3d toHole = holeCentroid - outerCentroid;
    double d = toHole.norm();
    if (d > maxD) {
      maxD = d;
      axis = (d > 1e-9) ? (toHole / d) : Vector3d(0, 0, 0);
      outIdx = static_cast<int>(hi);
    }
  }
  outAxis = axis;
  return outIdx >= 0 && (maxD > 0.5 * R) && axis.squaredNorm() > 1e-9;
}

// Picks a proj() automatically when the caller doesn't supply one (see the
// long comment in patch.h). Decides between the "tube" (axial) and "panel"
// (best-fit-plane) case by comparing how far the tube-partner hole's
// centroid sits from the outer ring's own centroid ('D', from
// findTubeAxis()) against the outer ring's own characteristic size ('R'):
// a hole clearly offset from the outer ring (D large relative to R) means
// the caller is connecting two separate rings in space (a tube/port), so
// the axial projection is used; holes close to the outer ring's own
// centroid (D small relative to R) mean they're most likely cutouts within
// the same flat panel, so the best-fit-plane projection is used instead.
//
// Only reached for the plain 2-ring tube case (a wall segment with no
// extra holes) or the panel case - patch() itself intercepts the
// tube-with-extra-holes case earlier and routes it to
// patchTubeWithHoles() instead, which needs to know the axis before
// picking a domain style at all (see the long comment there).
std::function<Vector2d(const Vector3d&)> computeAutoProj(const std::vector<Vector3d>& outer,
                                                         const std::vector<std::vector<Vector3d>>& holes)
{
  int tubeIdx;
  Vector3d axis;
  double R;
  const bool tubeLike = findTubeAxis(outer, holes, tubeIdx, axis, R);

  // findTubeAxis()'s heuristic (hole far from the outer ring's own centroid
  // -> treat as a tube/port, view along the axis between them) only looks
  // at DISTANCE, not at whether the outer ring actually has any extent left
  // to look at once that axis is dropped. It can fire for two rings that
  // are, in fact, coplanar (or nearly so) with each other AND with the very
  // axis connecting their centroids - e.g. two same-size holes offset along
  // an axis that itself lies IN both rings' own plane (a handle spanning
  // between two side ports on a wall, rather than a straight tube through
  // it). Looking "along" such an axis is looking at the rings edge-on: the
  // outer ring's own projected shape collapses to a line just like a
  // degenerate hole would (see isDegenerate2D() above) - not because of a
  // bad hole, but because the chosen axis was the wrong one to project
  // along in the first place. When that happens, fall back to the best-fit
  // plane instead, which for genuinely coplanar rings finds exactly that
  // shared plane and gives both rings a proper, non-degenerate 2D shape -
  // letting any supplied tangents bow the surface out of that plane instead
  // of leaving it a flat sliver.
  if (tubeLike) {
    auto axialProj = makeAxialProjection(axis);
    std::vector<Vector2d> outerTrial;
    outerTrial.reserve(outer.size());
    for (const auto& p : outer) outerTrial.push_back(axialProj(p));
    // A generous 0.04 (variance ratio) here, not the default 0.02: this is
    // deliberately not "is it exactly degenerate" but "is it thin enough
    // that an axial view wouldn't look reasonable" - i.e. the outer ring's
    // extent along the dropped axis is less than roughly 20% of its extent
    // across it. Two rings whose planes are only slightly different (not
    // perfectly coplanar) still want the best-fit-plane fallback, not just
    // the perfectly-coplanar case the stricter default would catch.
    if (!isDegenerate2D(outerTrial, 0.04)) {
      return axialProj;
    }
  }

  std::vector<Vector3d> allPts = outer;
  for (const auto& h : holes) allPts.insert(allPts.end(), h.begin(), h.end());
  return makePlaneProjection(allPts);
}

// -------------------- Hole-containment check (bridge vs. panel hole) --------------------
//
// The bridge classification below (findTubeAxis()'s distance heuristic +
// "the outer ring looks flat along that axis") is only a proxy for "these
// are two genuinely disjoint rings" - it can also fire for an off-center
// cutout that is perfectly ordinary (coplanar with 'outer', truly a hole
// IN it), if that cutout happens to sit far enough from outer's centroid
// to pass the distance threshold and the outer ring's own shape happens to
// look "flat" along the outer-to-hole axis for unrelated reasons (e.g. an
// elongated panel). Without this check, such a panel hole would be
// mis-routed to patchBridgeTwoRings() - which does not model containment
// at all, it just bridges two rings by index - producing a nonsensical
// result instead of the ordinary "outer area minus hole" surface the
// caller actually asked for.
//
// Decided independently of the axial projection used for the distance
// heuristic: both rings are projected onto their OWN shared best-fit plane
// (the same construction computeAutoProj() itself falls back to for a
// flat panel), and the hole is considered "contained" - i.e. an ordinary
// panel hole, NOT a bridge - when a strict majority of its own boundary
// points land inside the outer ring's projected shape there. A majority
// vote (rather than just the hole's centroid) is used because the hole
// ring's own shape can be non-convex or eccentric enough that its centroid
// alone is not a reliable proxy for "is this ring nested inside outer".
bool holeContainedInBestFitPlane(const std::vector<Vector3d>& outer, const std::vector<Vector3d>& hole)
{
  if (hole.empty()) return false;

  std::vector<Vector3d> allPts = outer;
  allPts.insert(allPts.end(), hole.begin(), hole.end());
  auto planeProj = makePlaneProjection(allPts);

  Polygon2d outerPoly;
  Outline2d o;
  o.vertices.reserve(outer.size());
  for (const auto& p : outer) o.vertices.push_back(planeProj(p));
  o.positive = true;
  outerPoly.addOutline(o);

  size_t insideCount = 0;
  for (const auto& p : hole) {
    if (outerPoly.point_location(planeProj(p)) == PointLocation2d::Inside) insideCount++;
  }
  return insideCount * 2 > hole.size();  // strict majority
}

// -------------------- Direct bridge between two disjoint rings --------------------
//
// Every OTHER construction in this file models a patch as "outer boundary's
// own 2D area, with holes cut out of it" - a Polygon2d domain, triangulated
// and filtered. That model is only sound when the outer ring's projected
// shape genuinely CONTAINS the hole's (a true tube/annulus, as for a
// straight-ish wall segment between two same-axis rings, or a cutout
// within a flat panel). It breaks down for two rings that are roughly
// coplanar with EACH OTHER and with the axis connecting their centers -
// e.g. two side ports in the same wall that a handle needs to arc
// between - because no projection turns two genuinely disjoint,
// side-by-side circles into one nested inside the other: viewed along
// the axis between them, both collapse to lines (computeAutoProj()'s own
// degenerate check catches this); viewed along their shared normal (the
// best-fit plane), they come out as two SEPARATE, non-overlapping disks,
// not an annulus - a "cut this hole out of that area" domain has nothing
// sensible to build there, which is why patch() ended up simply not
// connecting the two rings at all (0 cross-ring triangles) instead of
// forming a tube.
//
// Solved without any 2D domain, Delaunay, or jitter at all: the two rings
// are matched up point-by-point BY INDEX (sound whenever both come from
// the same kind of profile at the same point count - the motivating case,
// two taps of a handle built the same way), and each matched pair is
// connected by its own independent cubic Hermite curve using that point's
// own position and (if supplied) tangent - the classic "patch between two
// profile curves with end tangents" construction, the same one CAD tools
// use for this exact case. The curves are sampled at a uniform resolution
// along their own length and triangulated as a plain, regular quad-strip
// grid (closed around the ring, open between the two ends): connectivity
// is already fully known from the two rings' own point order, so there is
// nothing for a domain-triangulation step to figure out.
//
// The bow's DIRECTION follows directly from the supplied tangents (used
// as-is, not sign-adjusted): unlike the rest of this file,
// normalizeTangentSigns()'s "align with the outer-to-hole axis" heuristic
// is not applicable here (that axis is, by construction of this very
// code path, roughly PERPENDICULAR to a sensible bow tangent - see the
// "coin-flip" debug warning it would otherwise print) - so if the arc
// comes out pinched or bulges the wrong way, the fix is in the caller's
// own tangent directions (e.g. matching roty() signs between the two
// rings so both tangents point outward the same way), not in a sign
// heuristic here that has no reliable signal to work from.
std::unique_ptr<PolySet> patchBridgeTwoRings(const std::vector<Vector3d>& ringA,
                                             const std::vector<Vector3d>& ringB,
                                             const std::vector<Vector3d>& tangentA,
                                             const std::vector<Vector3d>& tangentB,
                                             double grid_spacing_uv,
                                             const std::function<double(const Vector3d&)>& displacement)
{
  const size_t n = ringA.size();
  if (n < 3 || ringB.size() != n) return nullptr;

  double maxLen = 0.0;
  for (size_t i = 0; i < n; i++) maxLen = std::max(maxLen, (ringB[i] - ringA[i]).norm());
  const int steps = std::max(2, static_cast<int>(maxLen / grid_spacing_uv) + 1);

  const bool haveTangents = tangentA.size() == n && tangentB.size() == n;

  std::vector<Vector3d> grid;  // (steps+1) rows x n cols, row-major
  grid.reserve(static_cast<size_t>(steps + 1) * n);
  for (int s = 0; s <= steps; s++) {
    const double t = static_cast<double>(s) / steps;
    for (size_t i = 0; i < n; i++) {
      if (s == 0) {
        grid.push_back(ringA[i]);
        continue;
      }
      if (s == steps) {
        grid.push_back(ringB[i]);
        continue;
      }
      const Vector3d& P0 = ringA[i];
      const Vector3d& P1 = ringB[i];
      if (!haveTangents) {
        grid.push_back(P0 * (1.0 - t) + P1 * t);
        continue;
      }
      // Standard cubic Bezier/Hermite control points, tangents scaled by
      // L/3 - the same scaling edgeControlPoints() uses elsewhere in this
      // file, for a consistent "how strongly does the tangent pull"
      // feel between the two constructions.
      const double L = (P1 - P0).norm();
      const Vector3d b0 = P0;
      const Vector3d b1 = P0 + tangentA[i] * (L / 3.0);
      const Vector3d b2 = P1 - tangentB[i] * (L / 3.0);
      const Vector3d b3 = P1;
      const double u = 1.0 - t;
      grid.push_back(b0 * (u * u * u) + b1 * (3 * u * u * t) + b2 * (3 * u * t * t) + b3 * (t * t * t));
    }
  }

  auto idx = [n](int s, size_t i) { return s * static_cast<int>(n) + static_cast<int>(i); };

  // Apply displacement() to the INTERIOR rows only (s in [1, steps-1]).
  // The two boundary rows (s==0 / s==steps) are 'ringA'/'ringB' themselves
  // and must keep their exact input positions for watertight stitching with
  // whatever neighboring patch()/concat() call produced those rings - same
  // convention as every other construction in this file. displacement() is
  // evaluated on the plain (undisplaced) bridge surface built above, then
  // pushed along that surface's own local normal, estimated per-vertex from
  // its immediate neighbors (central differences along the ring direction
  // and along the bridge direction, crossed) since there is no Delaunay
  // triangle to take a face normal from here. The push fades to zero at
  // both ends via the same smoothstep()/blendDist convention used for the
  // interior-grid displacement elsewhere in this file, so the interior
  // bulges smoothly out of the two unmodified boundary rows instead of
  // meeting them with a visible kink.
  if (displacement) {
    const double blendDist = std::max(2.0 * grid_spacing_uv, 1e-9);
    std::vector<Vector3d> displaced = grid;
    for (int s = 1; s < steps; s++) {
      const double t = static_cast<double>(s) / steps;
      const double distToEnd = std::min(t, 1.0 - t) * maxLen;
      const double falloff = smoothstep01(distToEnd / blendDist);
      for (size_t i = 0; i < n; i++) {
        const Vector3d& pos = grid[idx(s, i)];
        const Vector3d tangentU = grid[idx(s, (i + 1) % n)] - grid[idx(s, (i + n - 1) % n)];
        const Vector3d tangentV = grid[idx(s + 1, i)] - grid[idx(s - 1, i)];
        Vector3d normal = tangentU.cross(tangentV);
        const double nlen = normal.norm();
        if (nlen < 1e-12) continue;  // degenerate local frame - leave this vertex undisplaced
        normal /= nlen;
        const double d = displacement(pos) * falloff;
        displaced[idx(s, i)] = pos + normal * d;
      }
    }
    grid = std::move(displaced);
  }

  auto polyset = std::make_unique<PolySet>(3);
  polyset->setTriangular(true);
  polyset->vertices = grid;
  polyset->indices.reserve(static_cast<size_t>(steps) * n * 2);
  for (int s = 0; s < steps; s++) {
    for (size_t i = 0; i < n; i++) {
      const size_t i2 = (i + 1) % n;
      const int a = idx(s, i), b = idx(s, i2), c = idx(s + 1, i2), d = idx(s + 1, i);
      polyset->indices.push_back({a, b, c});
      polyset->indices.push_back({a, c, d});
    }
  }

  return polyset;
}

// -------------------- Tube-unroll domain (periodic u) --------------------
//
// Used only for a tube-shaped patch (outer ring + its genuine tube-partner
// hole, found by findTubeAxis()) that ALSO carries further holes needing a
// real, non-degenerate 2D shape regardless of their own orientation - e.g.
// a hole drilled straight through the tube's wall, pointing at the axis.
// The ordinary top-down axial projection (makeAxialProjection) drops the
// axis coordinate entirely, which is exactly the coordinate such a hole's
// own extent depends on - no per-hole trick can fix that (see the
// discussion that led here). This projection keeps BOTH the
// circumferential and the axial (height) extent as genuine, independently
// measured coordinates instead: u = arc length around the axis, v = height
// along it. A hole drilled at any angle now has real, non-collapsed extent
// in both.
//
// The price: the outer ring and its tube-partner hole, each being a full
// circle of revolution around the very axis this projection unrolls
// around, each map to a straight horizontal LINE (constant v, u spanning a
// full period) rather than a closed 2D loop - fundamentally, not as an
// implementation gap (see the "why can't ANY such projection avoid this"
// discussion earlier). They are therefore deliberately NOT treated as
// Polygon2d outer/hole loops here at all: the domain is a plain rectangle
// [u0, u0+period] x [vmin, vmax] (vmin/vmax = the two rings' own heights),
// with only the EXTRA holes cut out of it as real hole loops. The two
// rings still contribute their exact 3D boundary points to the base
// surface and final mesh, same as any boundary ring elsewhere in this
// file - concat() compatibility with neighboring patch() calls is
// unaffected.
//
// The other price is periodicity: u wraps around every 'period' units
// (the tube's circumference). The seam is placed diametrically opposite
// the extra holes' average angular position, and triangulation is done on
// three side-by-side copies of every point (u, u-period, u+period) so
// triangles spanning the seam are found; each result triangle is then
// mapped back to its original (non-ghost) point indices and de-duplicated
// (see triangulatePeriodic() below) - a standard technique for triangulating
// on a periodic domain.
//
// Deliberately does not support outer_normal/holes_normal (curvature):
// combining that with this periodic domain is a separate, bigger
// undertaking not needed for the case that motivated this (a straight
// mounting hole through an otherwise-straight tube wall) - patch() below
// only routes here when both are empty.
struct TubeUnroll {
  Vector3d axisOrigin, axisDir, e1, e2;
  double refRadius;
  double u0;  // start ("seam") of the fundamental domain in u
};

double wrapToPeriod(double u, double u0, double period)
{
  double t = std::fmod(u - u0, period);
  if (t < 0) t += period;
  return u0 + t;
}

Vector2d tubeUnrollProject(const TubeUnroll& tu, const Vector3d& p)
{
  Vector3d d = p - tu.axisOrigin;
  double v = d.dot(tu.axisDir);
  Vector3d perp = d - v * tu.axisDir;
  double angle = std::atan2(perp.dot(tu.e2), perp.dot(tu.e1));
  double u = angle * tu.refRadius;
  const double period = 2.0 * M_PI * tu.refRadius;
  return Vector2d(wrapToPeriod(u, tu.u0, period), v);
}

std::unique_ptr<PolySet> patchTubeWithHoles(const std::vector<Vector3d>& outer,
                                            const std::vector<std::vector<Vector3d>>& holes,
                                            int tubePartnerIdx, const Vector3d& axis,
                                            double grid_spacing_uv,
                                            const std::function<double(const Vector3d&)>& displacement)
{
  const std::vector<Vector3d>& partner = holes[tubePartnerIdx];

  TubeUnroll tu;
  tu.axisDir = axis.normalized();
  tu.axisOrigin = centroid3D(outer);
  Vector3d ref = (std::fabs(tu.axisDir.y()) < 0.9) ? Vector3d(0, 1, 0) : Vector3d(1, 0, 0);
  tu.e1 = tu.axisDir.cross(ref).normalized();
  tu.e2 = tu.axisDir.cross(tu.e1);

  double R = 0.0;
  for (const auto& p : outer) {
    Vector3d d = p - tu.axisOrigin;
    R = std::max(R, (d - d.dot(tu.axisDir) * tu.axisDir).norm());
  }
  for (const auto& p : partner) {
    Vector3d d = p - tu.axisOrigin;
    R = std::max(R, (d - d.dot(tu.axisDir) * tu.axisDir).norm());
  }
  if (R < 1e-9) R = 1.0;
  tu.refRadius = R;
  const double period = 2.0 * M_PI * R;

  // Seam placement: diametrically opposite the extra holes' average
  // angular position, so they sit centered in the fundamental domain, far
  // from the wraparound.
  Vector3d extraSum(0, 0, 0);
  for (size_t hi = 0; hi < holes.size(); hi++) {
    if (static_cast<int>(hi) == tubePartnerIdx || holes[hi].empty()) continue;
    Vector3d d = centroid3D(holes[hi]) - tu.axisOrigin;
    Vector3d perp = d - d.dot(tu.axisDir) * tu.axisDir;
    if (perp.squaredNorm() > 1e-12) extraSum += perp.normalized();
  }
  double holesAngle = 0.0;
  if (extraSum.squaredNorm() > 1e-12) {
    holesAngle = std::atan2(extraSum.dot(tu.e2), extraSum.dot(tu.e1));
  }
  tu.u0 = holesAngle * R - M_PI * R;

  auto proj = [&tu](const Vector3d& p) { return tubeUnrollProject(tu, p); };

  std::vector<Vector2d> outer_uv;
  outer_uv.reserve(outer.size());
  for (const auto& p : outer) outer_uv.push_back(proj(p));

  std::vector<std::vector<Vector2d>> holes_uv(holes.size());
  for (size_t hi = 0; hi < holes.size(); hi++) {
    holes_uv[hi].reserve(holes[hi].size());
    for (const auto& p : holes[hi]) holes_uv[hi].push_back(proj(p));
  }

  double vmin = outer_uv[0].y(), vmax = vmin;
  for (const auto& p : outer_uv) {
    vmin = std::min(vmin, p.y());
    vmax = std::max(vmax, p.y());
  }
  for (const auto& p : holes_uv[tubePartnerIdx]) {
    vmin = std::min(vmin, p.y());
    vmax = std::max(vmax, p.y());
  }

  // Domain: the rectangle itself (NOT outer_uv/holes_uv[tubePartnerIdx],
  // which are degenerate lines here - see the long comment above), with
  // only the extra holes cut out.
  Polygon2d domain;
  {
    Outline2d rect;
    rect.vertices = {Vector2d(tu.u0, vmin), Vector2d(tu.u0 + period, vmin),
                     Vector2d(tu.u0 + period, vmax), Vector2d(tu.u0, vmax)};
    rect.positive = true;
    domain.addOutline(rect);
    for (size_t hi = 0; hi < holes.size(); hi++) {
      if (static_cast<int>(hi) == tubePartnerIdx || holes_uv[hi].size() < 3) continue;
      auto ring = holes_uv[hi];
      if (signedArea2D(ring) > 0) std::reverse(ring.begin(), ring.end());
      Outline2d ho;
      ho.vertices.assign(ring.begin(), ring.end());
      ho.positive = false;
      domain.addOutline(ho);
    }
  }
  const double domainScale = std::max(period, vmax - vmin);

  // A triangle produced below, carrying BOTH the point indices into the
  // original (non-ghost) point array (for 3D position lookup) AND the
  // actual ghost-shifted (u,v) it was found at - i.e. a LOCALLY CONTIGUOUS
  // 2D shape, not necessarily inside [u0, u0+period). This distinction
  // matters only for a triangle that crosses the seam: e.g. one vertex
  // near u0 and another near u0+period are genuinely adjacent in 3D (the
  // seam is an artificial cut, not a real edge of the tube), but their
  // plain wrapped-into-[u0,u0+period) uv values are far apart numerically.
  // Storing the ghost uv keeps the triangle's true (small, contiguous)
  // 2D shape intact instead of silently turning it into a huge, wrong-
  // shaped sliver spanning most of the domain width - see
  // PeriodicBaseSurface below for why that distinction is essential.
  struct PeriodicTri {
    std::array<int, 3> idx;
    std::array<Vector2d, 3> uv;
  };

  // Triangulates 'uv' via three side-by-side ghost copies (u, u-period,
  // u+period) so seam-crossing triangles are found, keeps only those whose
  // (ghost-space) centroid lands in the true fundamental domain, and
  // de-duplicates - but, unlike a plain remap-to-original-indices, keeps
  // each kept triangle's actual ghost-shifted uv alongside its original
  // indices (see PeriodicTri above).
  //
  // PERFORMANCE: bowyerWatson() below is an unconstrained, unaccelerated
  // incremental Delaunay (no spatial grid/tree to localize the "which
  // triangles does this new point invalidate" search) - it costs
  // O(pointCount^2). Tripling EVERY point into 3 ghost copies (as a naive
  // implementation of the ghost-copy trick would) needlessly cubes that
  // already-expensive input for the (usually large) interior grid point
  // set, even though only points actually near the seam can ever end up
  // in a seam-crossing triangle. Ghosting only those - within 'margin' of
  // either edge of the fundamental domain - keeps the overwhelming
  // majority of points (everything away from the seam) at their single,
  // real copy, which is the single biggest win available here without
  // replacing bowyerWatson() itself with a spatially accelerated
  // triangulator.
  const double margin = std::min(0.5 * period, std::max(8.0 * grid_spacing_uv, 1e-6));
  auto triangulatePeriodic = [&](const std::vector<Vector2d>& uv) -> std::vector<PeriodicTri> {
    std::vector<Vector2d> ghostUV;
    std::vector<int> ghostSrc;
    ghostUV.reserve(uv.size() + uv.size() / 4);
    ghostSrc.reserve(ghostUV.capacity());
    for (size_t i = 0; i < uv.size(); i++) {
      ghostUV.push_back(uv[i]);
      ghostSrc.push_back(static_cast<int>(i));
      const double distToLow = uv[i].x() - tu.u0;
      const double distToHigh = (tu.u0 + period) - uv[i].x();
      if (distToLow < margin) {
        ghostUV.push_back(Vector2d(uv[i].x() + period, uv[i].y()));
        ghostSrc.push_back(static_cast<int>(i));
      }
      if (distToHigh < margin) {
        ghostUV.push_back(Vector2d(uv[i].x() - period, uv[i].y()));
        ghostSrc.push_back(static_cast<int>(i));
      }
    }
    auto jittered = jitterForDelaunay(ghostUV, domainScale, ghostSrc);
    auto tris = bowyerWatson(jittered);

    std::vector<PeriodicTri> kept;
    std::set<std::array<int, 3>> seen;
    for (const auto& t : tris) {
      Vector2d centroid = (ghostUV[t.a] + ghostUV[t.b] + ghostUV[t.c]) / 3.0;
      if (centroid.x() < tu.u0 || centroid.x() >= tu.u0 + period) continue;
      if (domain.point_location(centroid) < PointLocation2d::OnEdge) continue;
      std::array<int, 3> remapped = {ghostSrc[t.a], ghostSrc[t.b], ghostSrc[t.c]};
      std::array<int, 3> key = remapped;
      std::sort(key.begin(), key.end());
      if (!seen.insert(key).second) continue;  // same triangle via another ghost band
      PeriodicTri pt;
      pt.idx = remapped;
      pt.uv = {ghostUV[t.a], ghostUV[t.b], ghostUV[t.c]};
      kept.push_back(pt);
    }
    return kept;
  };

  // Boundary point set: outer ring + all holes (tube-partner AND extras),
  // in a single flat sequence.
  std::vector<Vector2d> boundaryUV = outer_uv;
  std::vector<Vector3d> boundaryPos = outer;
  for (size_t hi = 0; hi < holes.size(); hi++) {
    boundaryUV.insert(boundaryUV.end(), holes_uv[hi].begin(), holes_uv[hi].end());
    boundaryPos.insert(boundaryPos.end(), holes[hi].begin(), holes[hi].end());
  }

  // -------------------- Periodicity-aware base surface --------------------
  // Deliberately NOT the shared LoftBaseSurface: that class holds ONE
  // shared 2D uv per point index and tests a query point against a
  // triangle's plain uv as-is. For a triangle that crosses the seam, that
  // plain uv is the wrong (huge, wrapped-apart) shape described above -
  // using it would let such a triangle's corrupted 2D footprint "steal"
  // query points from a completely different part of the tube (observed as
  // a stretch of the wall turning flat instead of round, roughly opposite
  // the seam). Here, each triangle keeps its own locally contiguous uv
  // (straight from triangulatePeriodic() above), and sample() additionally
  // tries the query point shifted by -period/0/+period against each
  // triangle - the same ghost-copy idea used for triangulating, just
  // applied per query instead of per input point.
  struct PeriodicBaseSurface {
    std::vector<Vector3d> pos3d;  // indexed by the ORIGINAL (non-ghost) point index
    std::vector<PeriodicTri> tris;
    double period = 0.0;
    double u0 = 0.0;
    double margin = 0.0;  // only points within this of a domain edge can ever need a shifted test

    bool sampleTriAt(const Vector2d& p, const PeriodicTri& t, Vector3d& outPos,
                     Vector3d& outNormal) const
    {
      const Vector2d& a = t.uv[0];
      const Vector2d& b = t.uv[1];
      const Vector2d& c = t.uv[2];
      double denom = (b.y() - c.y()) * (a.x() - c.x()) + (c.x() - b.x()) * (a.y() - c.y());
      if (std::fabs(denom) < 1e-12) return false;
      double w0 = ((b.y() - c.y()) * (p.x() - c.x()) + (c.x() - b.x()) * (p.y() - c.y())) / denom;
      double w1 = ((c.y() - a.y()) * (p.x() - c.x()) + (a.x() - c.x()) * (p.y() - c.y())) / denom;
      double w2 = 1.0 - w0 - w1;
      const double eps = -1e-9;
      if (w0 < eps || w1 < eps || w2 < eps) return false;

      const Vector3d& p0 = pos3d[t.idx[0]];
      const Vector3d& p1 = pos3d[t.idx[1]];
      const Vector3d& p2 = pos3d[t.idx[2]];
      outPos = w0 * p0 + w1 * p1 + w2 * p2;
      Vector3d n = (p1 - p0).cross(p2 - p0);
      double nlen = n.norm();
      outNormal = (nlen > 1e-12) ? (n / nlen) : Vector3d(0, 0, 1);
      return true;
    }

    // Only a query point actually near one of the fundamental domain's own
    // edges can ever land inside a seam-crossing triangle's (necessarily
    // nearby) locally contiguous uv footprint - a triangle far from the
    // seam has an uv range far from p+-period regardless of p, so testing
    // the shifted positions there can never succeed. Skipping those two
    // extra attempts for the (overwhelming majority) of points that aren't
    // near the seam turns the common case back into a single scan over
    // 'tris' instead of three.
    bool sample(const Vector2d& p, Vector3d& outPos, Vector3d& outNormal) const
    {
      for (const auto& t : tris) {
        if (sampleTriAt(p, t, outPos, outNormal)) return true;
      }
      const bool nearSeam = (p.x() - u0 < margin) || ((u0 + period) - p.x() < margin);
      if (!nearSeam) return false;
      for (const auto& t : tris) {
        if (sampleTriAt(Vector2d(p.x() + period, p.y()), t, outPos, outNormal)) return true;
        if (sampleTriAt(Vector2d(p.x() - period, p.y()), t, outPos, outNormal)) return true;
      }
      return false;
    }
  };

  // The base surface is built from outer + the TUBE-PARTNER ring ONLY -
  // deliberately NOT boundaryUV/boundaryPos, which also carry the EXTRA
  // holes. Its only job is to approximate the smooth, continuous
  // cone/cylinder shape an interior grid or collar point near an extra hole
  // should sit on, as if that hole hadn't been cut yet - it is a scaffold
  // for POSITIONING other points, not the final, hole-cut mesh (an extra
  // hole's own boundary points still get their exact input 3D position
  // directly, via boundaryPos/allPos below, regardless of what the base
  // surface does; and the final mesh's own connectivity - which constraints
  // it enforces, how it connects to each hole's edge - is entirely
  // unaffected by this, since finalTris is still built from boundaryUV/
  // allUV with every hole included exactly as before).
  //
  // Including an extra hole's ring here anyway is actively harmful: at this
  // stage NO interior points exist yet (those are generated by sampling
  // THIS base surface), so a small, tightly-clustered ring like tap1/tap2
  // becomes a fully disconnected "island" amid only the two widely-spaced
  // boundary rows - the triangulation has nothing nearby to connect it to,
  // and bridges it with enormous "spoke" triangles reaching all the way out
  // to the nearest row point. Measured for this exact model: up to 100
  // units (the tube's entire wall height), against ~9.7 units of normal
  // ring spacing - a 10x jump. Any interior/collar point whose base sample
  // lands inside one of those spokes gets an interpolated position from two
  // barely-related boundary points, which can easily fold the resulting
  // patch of final geometry back on itself - visible as several stray
  // "phantom" arcs of folded-over, wrongly-oriented triangles, roughly at
  // the extra holes' own height but away from them, wherever a spoke
  // happens to sweep.
  std::vector<Vector2d> baseUV = outer_uv;
  std::vector<Vector3d> basePos = outer;
  baseUV.insert(baseUV.end(), holes_uv[tubePartnerIdx].begin(), holes_uv[tubePartnerIdx].end());
  basePos.insert(basePos.end(), partner.begin(), partner.end());

  PeriodicBaseSurface base;
  base.pos3d = basePos;
  base.period = period;
  base.u0 = tu.u0;
  base.margin = margin;
  base.tris = triangulatePeriodic(baseUV);
  if (base.tris.empty()) return nullptr;

  // Interior grid, sampled directly over the rectangle domain.
  //
  // Boundary-edge encroachment protection (see pointEncroachesBoundary()
  // elsewhere in this file) was previously applied only to the EXTRA
  // holes' own closed loops here - outer_uv/holes_uv[tubePartnerIdx] are
  // open (period-wrapping) polylines, not closed rings, so the ordinary
  // consecutive-edge check (which wraps i+1 back to 0) doesn't apply to
  // them directly, and this omission was believed to be "a minor cosmetic
  // risk, not a watertightness one". A mesh-quality diagnostic on a real
  // model proved that wrong: whenever a grid row lands close enough to
  // outer_uv's or the tube-partner ring's own v-coordinate, an interior
  // point can fall inside one of THEIR boundary edges' diametral circles
  // too, and the unconstrained triangulation "shortcuts" past it exactly
  // like it does near an extra hole - producing a band of near-zero-area
  // sliver triangles hugging that boundary (confirmed: dozens of
  // degenerate triangles, all with two vertices exactly on the outer
  // ring). encroachesRingPeriodic() below is the same encroachment test,
  // but aware that outer_uv/holes_uv[tubePartnerIdx] are really CLOSED
  // rings expressed through a periodic (u-wrapping) coordinate - see its
  // own comment further down for why that distinction matters.
  std::vector<Vector2d> allUV = boundaryUV;
  std::vector<Vector3d> allPos = boundaryPos;
  const int nu = std::max(2, static_cast<int>(period / grid_spacing_uv) + 1);
  const int nv = std::max(2, static_cast<int>((vmax - vmin) / grid_spacing_uv) + 1);
  const double blendDist = std::max(2.0 * grid_spacing_uv, 1e-9);

  auto encroachesExtraHoles = [&](const Vector2d& p) {
    for (size_t hi = 0; hi < holes.size(); hi++) {
      if (static_cast<int>(hi) == tubePartnerIdx) continue;
      const auto& ring = holes_uv[hi];
      for (size_t i = 0; i < ring.size(); i++) {
        if (pointEncroachesEdge(p, ring[i], ring[(i + 1) % ring.size()])) return true;
      }
    }
    return false;
  };

  // outer_uv/holes_uv[tubePartnerIdx] are built by projecting a genuinely
  // CLOSED 3D ring through wrapToPeriod(), so somewhere in its point
  // sequence - wherever the seam happens to fall relative to that ring's
  // OWN point 0, which has nothing to do with where the seam sits relative
  // to the domain - exactly one consecutive pair jumps by ~period instead
  // of by one normal step. (An earlier version of this check assumed that
  // pair was always the array's own wraparound, index size-1 to 0; that
  // isn't generally true - the seam is placed opposite the EXTRA holes,
  // the ring's point 0 is wherever its own parametrization starts, and
  // those two things are unrelated. For this exact reason, the jump can
  // land in the MIDDLE of the array, e.g. between two ordinary-looking
  // indices - and checking that pair naively, as a literal ~period-long
  // edge, doesn't flag nearby points as encroaching either: with the
  // endpoints so far apart, points near just one of them fall outside that
  // edge's obtuse "diametral" region. Points there slipped through
  // entirely, producing the sliver/duplicate-vertex triangles right at the
  // seam - the "phantom of the (nonexistent, far-side) hole" reported
  // opposite a real one is exactly that.)
  //
  // Walk the ring as the CLOSED cycle it geometrically is (including the
  // size-1-to-0 pair), find the one edge whose u-span is close to a full
  // period, and for THAT edge only, unwrap it onto a short edge next to
  // whichever copy is close to p, then test p and both of its
  // period-shifted copies against it - mirroring how triangulatePeriodic()
  // itself ghosts points within 'margin' of the domain's u-edges. Every
  // other edge is short by construction and is checked directly.
  auto encroachesRingPeriodic = [&](const Vector2d& p, const std::vector<Vector2d>& poly) {
    const size_t n = poly.size();
    if (n < 2) return false;
    for (size_t i = 0; i < n; i++) {
      const Vector2d& a = poly[i];
      const Vector2d& b0 = poly[(i + 1) % n];
      const double db = b0.x() - a.x();
      if (std::fabs(db) < period * 0.5) {
        if (pointEncroachesEdge(p, a, b0)) return true;
        continue;
      }
      // This is the seam-crossing edge: unwrap it onto a short edge, then
      // test p and its own period-shifted copies against it.
      Vector2d b = b0;
      if (db > period * 0.5) b = Vector2d(b.x() - period, b.y());
      else b = Vector2d(b.x() + period, b.y());
      for (double shift : {0.0, period, -period}) {
        if (pointEncroachesEdge(Vector2d(p.x() + shift, p.y()), a, b)) return true;
      }
    }
    return false;
  };
  auto encroachesTubeBoundary = [&](const Vector2d& p) {
    return encroachesRingPeriodic(p, outer_uv) || encroachesRingPeriodic(p, holes_uv[tubePartnerIdx]);
  };

  for (int iu = 0; iu < nu; iu++) {
    for (int iv = 0; iv < nv; iv++) {
      Vector2d p(tu.u0 + iu * grid_spacing_uv, vmin + iv * grid_spacing_uv);
      if (domain.point_location(p, 0) == PointLocation2d::Outside) continue;
      if (encroachesExtraHoles(p)) continue;
      if (encroachesTubeBoundary(p)) continue;
      Vector3d pos, normal;
      if (!base.sample(p, pos, normal)) continue;
      double falloff = smoothstep01(distanceToBoundary(p, outer_uv, holes_uv) / blendDist);
      double d = displacement(pos) * falloff;
      allUV.push_back(p);
      allPos.push_back(pos + normal * d);
    }
  }

  // Collar points hugging each EXTRA hole's own boundary, one just outside
  // each of its ring vertices. Needed regardless of how fine grid_spacing_uv
  // is: the uniform grid above always leaves a buffer with NO points
  // immediately around a hole (points that would land inside it, or close
  // enough to threaten its boundary edges via pointEncroachesEdge(), are
  // both excluded on purpose - see the comments on that grid loop and on
  // pointEncroachesEdge() itself). When a hole is smaller than the grid
  // spacing, that buffer can be the ONLY area near the hole with no
  // supporting points at all, and the unconstrained Delaunay then connects
  // the hole's boundary almost directly to whatever points are next
  // nearest - typically far away - producing a large, flat, spike-shaped
  // triangle whose tip sits right on the hole. A collar guarantees there is
  // always at least one ring of points at a small, hole-scaled standoff
  // (never larger than half the grid spacing, so it never collides with the
  // regular grid), giving the triangulation something close by to connect
  // to on every side of the hole instead of reaching across the gap.
  for (size_t hi = 0; hi < holes.size(); hi++) {
    if (static_cast<int>(hi) == tubePartnerIdx || holes_uv[hi].size() < 3) continue;
    const auto& ring = holes_uv[hi];
    Vector2d c(0, 0);
    for (const auto& p : ring) c += p;
    c /= static_cast<double>(ring.size());

    double avgEdge = 0.0;
    for (size_t i = 0; i < ring.size(); i++) {
      avgEdge += (ring[(i + 1) % ring.size()] - ring[i]).norm();
    }
    avgEdge /= static_cast<double>(ring.size());
    const double offset = std::min(0.5 * grid_spacing_uv, 0.5 * avgEdge);
    if (offset < 1e-9) continue;

    for (const auto& rp : ring) {
      Vector2d dir = rp - c;
      double len = dir.norm();
      if (len < 1e-12) continue;
      Vector2d p = rp + (dir / len) * offset;
      if (domain.point_location(p) < PointLocation2d::OnEdge) continue;
      if (encroachesExtraHoles(p)) continue;
      if (encroachesTubeBoundary(p)) continue;
      Vector3d pos, normal;
      if (!base.sample(p, pos, normal)) continue;
      double falloff = smoothstep01(distanceToBoundary(p, outer_uv, holes_uv) / blendDist);
      double d = displacement(pos) * falloff;
      allUV.push_back(p);
      allPos.push_back(pos + normal * d);
    }
  }

  auto finalTris = triangulatePeriodic(allUV);
  if (finalTris.empty()) return nullptr;

  auto polyset = std::make_unique<PolySet>(3);
  polyset->setTriangular(true);
  polyset->vertices = allPos;
  polyset->indices.reserve(finalTris.size());
  for (const auto& t : finalTris) polyset->indices.push_back({t.idx[0], t.idx[1], t.idx[2]});

  return polyset;
}

}  // namespace

std::unique_ptr<PolySet> patch(const std::vector<Vector3d>& outer,
                               const std::vector<std::vector<Vector3d>>& holes,
                               const std::function<Vector2d(const Vector3d&)>& proj,
                               double grid_spacing_uv,
                               const std::function<double(const Vector3d&)>& displacement,
                               const std::vector<Vector3d>& outer_normal,
                               const std::vector<std::vector<Vector3d>>& holes_normal)
{
  if (outer.size() < 3 || grid_spacing_uv <= 0.0) return nullptr;

  // -2) Two disjoint (non-nested) rings needing a direct bridge, not an
  // annulus: exactly one hole, and findTubeAxis()'s distance heuristic says
  // the two rings look connected along an axis, but looking along that axis
  // leaves the OUTER ring itself flat - the same test computeAutoProj()
  // uses to fall back to a best-fit plane, except here that fallback
  // wouldn't help either (it would put the two rings as separate,
  // non-nested disks - see the long comment on patchBridgeTwoRings() for
  // exactly why the usual "outer disk with a hole cut out" domain has no
  // sensible domain to build for that shape). A handle spanning two side
  // ports is the motivating case.
  //
  // Both remaining checks are gates on whether this classification is even
  // correct, not on how to carry it out, so they run before (and
  // independently of) the point-count check patchBridgeTwoRings() itself
  // needs:
  //  - holeContainedInBestFitPlane(): the distance heuristic + "outer looks
  //    flat along this axis" combination is only a PROXY for "these are
  //    two disjoint rings" - it can also fire for an ordinary, merely
  //    off-center coplanar cutout (see that function's own long comment).
  //    Reject the bridge classification outright when the hole is actually
  //    nested inside outer's own best-fit-plane shape; let it fall through
  //    to the ordinary polygon-with-holes path below instead, which is
  //    what an actual panel hole needs.
  //  - the point-count check: patchBridgeTwoRings() matches the two rings
  //    up point-by-point BY INDEX (see its own long comment), which is
  //    only meaningful when both rings have the same point count.
  if (!proj && holes.size() == 1) {
    int tubeIdx;
    Vector3d axis;
    double R;
    if (findTubeAxis(outer, holes, tubeIdx, axis, R)) {
      auto axialProj = makeAxialProjection(axis);
      std::vector<Vector2d> outerTrial;
      outerTrial.reserve(outer.size());
      for (const auto& p : outer) outerTrial.push_back(axialProj(p));
      if (isDegenerate2D(outerTrial, 0.04) && !holeContainedInBestFitPlane(outer, holes[0])) {
        // Confirmed bridge case: not an ordinary contained hole, so the
        // only remaining question is whether patchBridgeTwoRings() can
        // actually be run on it. Resampling one ring to the other's point
        // count would still have to invent a point-by-point correspondence
        // (which point of a resampled 64-point ring is "the same" as
        // point 0 of a 32-point ring?) with no information to base it on
        // beyond point order - silently guessing could easily twist or
        // pinch the bridge into something worse than an outright failure.
        // So a mismatch is rejected explicitly instead.
        if (holes[0].size() != outer.size()) {
          LOG(message_group::Error,
              "patch(): direct bridge between two disjoint rings requires "
              "'outer' and the hole to have the same number of points (got "
              "%1$s and %2$s) - resample one of them to match.",
              std::to_string(outer.size()), std::to_string(holes[0].size()));
          return nullptr;
        }
        static const std::vector<Vector3d> emptyTangent;
        const std::vector<Vector3d>& holeTangent = holes_normal.empty() ? emptyTangent : holes_normal[0];
        auto result =
          patchBridgeTwoRings(outer, holes[0], outer_normal, holeTangent, grid_spacing_uv, displacement);
        if (result) return result;
      }
    }
  }

  // -1) Tube-with-extra-holes fast path: only when the caller didn't force a
  // projection and supplied no tangents (patchTubeWithHoles() doesn't support
  // curvature - see its own long comment), and there's more than just the
  // tube's own two rings (outer + tube-partner) to deal with. findTubeAxis()
  // both confirms this really is a tube (vs. a flat panel with cutouts) and
  // says which hole is the genuine tube partner, so the extra ones can be
  // routed to the periodic tube-unroll domain instead of the ordinary
  // top-down axial projection, which would flatten any hole whose extent
  // runs along the dropped axis (e.g. an exactly radial mounting hole) into
  // a zero-area line - see the long comment above patchTubeWithHoles().
  const bool noTangentsSupplied = !hasAnyTangent(outer_normal) && !hasAnyHoleTangent(holes_normal);

  if (!proj && noTangentsSupplied && holes.size() >= 2) {
    int tubeIdx;
    Vector3d axis;
    double R;
    if (findTubeAxis(outer, holes, tubeIdx, axis, R)) {
      auto result = patchTubeWithHoles(outer, holes, tubeIdx, axis, grid_spacing_uv, displacement);
      if (result) return result;
    }
  }

  // 0) No proj() supplied -> pick one automatically (see patch.h / the long
  // comment on computeAutoProj() above). An explicitly supplied proj is
  // always used as-is and never overridden. 'autoProj' is a named local so
  // that, when used, 'effectiveProj' binds to a real object with function
  // scope lifetime rather than a dangling temporary.
  std::function<Vector2d(const Vector3d&)> autoProj;
  const std::function<Vector2d(const Vector3d&)>& effectiveProj =
    proj ? proj : (autoProj = computeAutoProj(outer, holes));

  // 1) Project the boundary rings into (u,v) space.
  std::vector<Vector2d> outer_uv;
  outer_uv.reserve(outer.size());
  for (const auto& p : outer) outer_uv.push_back(effectiveProj(p));

  std::vector<std::vector<Vector2d>> holes_uv;
  holes_uv.reserve(holes.size());
  for (const auto& h : holes) {
    holes_uv.emplace_back();
    holes_uv.back().reserve(h.size());
    for (const auto& p : h) holes_uv.back().push_back(effectiveProj(p));
  }

  // 1a) Degenerate-hole fallback: a hole whose shape under the SHARED
  // projection is too "flat" (see isDegenerate2D()) is re-flattened with
  // its own local best-fit plane and placed at its centroid's position
  // under the shared projection instead - see the long comment above
  // localHoleReembed(). Holes that already project fine (the normal case,
  // e.g. a genuine tube-partner ring) are left completely untouched.
  for (size_t hi = 0; hi < holes_uv.size(); hi++) {
    if (holes[hi].size() < 3 || !isDegenerate2D(holes_uv[hi])) continue;
    Vector2d placeAt = effectiveProj(centroid3D(holes[hi]));
    holes_uv[hi] = localHoleReembed(holes[hi], placeAt);
  }

  // 1b) Normalize winding: outer CCW, holes CW (see ensureOrientation()).
  // Keep the parallel 3D point arrays ('outer_pos', 'holes_pos' copies) and
  // the (optional) tangent arrays in sync with any reversal.
  std::vector<Vector3d> outer_pos = outer;
  std::vector<std::vector<Vector3d>> holes_pos = holes;
  std::vector<Vector3d> outer_normal_copy = outer_normal;
  std::vector<std::vector<Vector3d>> holes_normal_copy = holes_normal;
  ensureOrientation(outer_uv, outer_pos, outer_normal_copy, holes_uv, holes_pos, holes_normal_copy);

  // 1c) Make sure all supplied tangents point "outer -> hole" on balance,
  // so the cubic patch below bends consistently instead of flipping
  // direction from one boundary point to the next. No-op when no tangents
  // were supplied (both arrays stay empty).
  normalizeTangentSigns(outer_pos, outer_normal_copy, holes_pos, holes_normal_copy);

  Polygon2d domain = buildDomainPolygon(outer_uv, holes_uv);

  // Every ring's consecutive-point segments, as index-pair constraints for
  // the Delaunay triangulations below (base surface and final mesh both
  // lay their boundary points out outer-then-holes, in this same order -
  // see buildRingConstraints()) - so both stitch to a neighboring patch()
  // call along the exact input boundary, not whatever chord an
  // unconstrained Delaunay triangulation would otherwise have preferred
  // for a concave or irregular ring.
  const auto ringConstraints = buildRingConstraints(outer_uv, holes_uv);

  // uv bounding box - needed for the base surface's jitter scale and the
  // interior sampling grid below.
  double umin = outer_uv[0].x(), umax = umin, vmin = outer_uv[0].y(), vmax = vmin;
  for (const auto& p : outer_uv) {
    umin = std::min(umin, p.x());
    umax = std::max(umax, p.x());
    vmin = std::min(vmin, p.y());
    vmax = std::max(vmax, p.y());
  }
  double domainScale = std::max(umax - umin, vmax - vmin);

  // 2) Base surface: gives interior grid points (step 3) a well-defined 3D
  //    base position + normal via barycentric (or, when tangents were
  //    supplied, cubic) interpolation. Built by triangulating just the
  //    boundary points (outer + holes) with the same unconstrained-
  //    Delaunay-plus-domain-filter technique used for the final mesh in
  //    step 4, then discarding triangles outside the domain.
  LoftBaseSurface base;
  {
    base.uv = outer_uv;
    base.pos3d = outer_pos;
    base.normal = outer_normal_copy.empty() ? std::vector<Vector3d>(outer_pos.size(), Vector3d(0, 0, 0))
                                            : outer_normal_copy;
    base.ringId = std::vector<int>(outer_pos.size(), 0);
    for (size_t hi = 0; hi < holes_uv.size(); hi++) {
      base.uv.insert(base.uv.end(), holes_uv[hi].begin(), holes_uv[hi].end());
      base.pos3d.insert(base.pos3d.end(), holes_pos[hi].begin(), holes_pos[hi].end());
      const bool haveHoleNormal = hi < holes_normal_copy.size() && !holes_normal_copy[hi].empty();
      const std::vector<Vector3d> zeros(holes_pos[hi].size(), Vector3d(0, 0, 0));
      const std::vector<Vector3d>& hn = haveHoleNormal ? holes_normal_copy[hi] : zeros;
      base.normal.insert(base.normal.end(), hn.begin(), hn.end());
      base.ringId.insert(base.ringId.end(), holes_pos[hi].size(), static_cast<int>(hi) + 1);
    }

    auto baseTris = triangulateAndFilterToDomain(base.uv, domain, domainScale, ringConstraints);
    base.tris.reserve(baseTris.size());
    for (const auto& t : baseTris) base.tris.push_back({t.a, t.b, t.c});
    if (base.tris.empty()) return nullptr;
  }

  // 3) Combined point set: boundary points (real 3D position known, NEVER
  //    displaced - see below) + interior grid points (3D position/normal
  //    from the base surface, displacement applied with a smooth falloff
  //    towards the boundary).
  std::vector<Vector2d> allUV;
  std::vector<Vector3d> allPos;
  allUV.reserve(outer_uv.size() + 64);
  allPos.reserve(outer_uv.size() + 64);

  // Boundary points (outer ring + hole rings) keep their exact input
  // position. displacement() is intentionally never called for these - a
  // texture/bump function must not be able to move a point that lies on the
  // outer or inner (hole) contour, since those contours are what neighboring
  // patch() calls (e.g. the next ring up/down a vase wall) rely on to line up
  // exactly. The same reasoning is why boundary points are exempt from
  // curvature above (they are stored raw in base.pos3d and only ever used
  // as fixed corner points of the cubic patch, never re-evaluated by it).
  for (size_t i = 0; i < outer_uv.size(); i++) {
    allUV.push_back(outer_uv[i]);
    allPos.push_back(outer_pos[i]);
  }
  for (size_t hi = 0; hi < holes_uv.size(); hi++) {
    for (size_t i = 0; i < holes_uv[hi].size(); i++) {
      allUV.push_back(holes_uv[hi][i]);
      allPos.push_back(holes_pos[hi][i]);
    }
  }

  int nu = std::max(2, static_cast<int>((umax - umin) / grid_spacing_uv) + 1);
  int nv = std::max(2, static_cast<int>((vmax - vmin) / grid_spacing_uv) + 1);

  // Interior grid points fade the displacement out smoothly as they
  // approach the boundary, reaching exactly zero at 'blendDist' or closer.
  // This isn't just cosmetic: without it, the row of interior points
  // immediately next to the boundary gets the full displacement while the
  // boundary itself (see above) gets none, producing a visible step right
  // at the rim - which looks like the boundary itself got bumped even
  // though, strictly, it never moved.
  const double blendDist = std::max(2.0 * grid_spacing_uv, 1e-9);

  for (int iu = 0; iu < nu; iu++) {
    for (int iv = 0; iv < nv; iv++) {
      Vector2d p(umin + iu * grid_spacing_uv, vmin + iv * grid_spacing_uv);
      if (domain.point_location(p) < PointLocation2d::OnEdge) continue;

      // Protects the *topology* of the final mesh, not just the
      // displacement: step 4's triangulation is an UNCONSTRAINED Delaunay
      // triangulation, which has no obligation to keep a boundary ring's
      // own short edges (e.g. between two adjacent points of 'outer' or a
      // hole) as edges of the result. If an interior point lies inside a
      // boundary edge's diametral circle, it can "steal" that edge (the
      // empty-circumcircle rule prefers connecting to it instead),
      // silently dropping the straight boundary edge and replacing it with
      // thinner triangles that bulge slightly off the true boundary line.
      // That breaks watertightness against any neighboring surface built
      // from the same ring (e.g. the next segment of a vase wall, or a cap
      // over the same hole). See pointEncroachesBoundary() above for the
      // exact (and tight - it scales with each edge's own local length,
      // not a fixed distance) test used here.
      if (pointEncroachesBoundary(p, outer_uv, holes_uv)) continue;

      Vector3d pos, normal;
      if (!base.sample(p, pos, normal)) continue;

      double falloff = smoothstep01(distanceToBoundary(p, outer_uv, holes_uv) / blendDist);
      double d = displacement(pos) * falloff;
      allUV.push_back(p);
      allPos.push_back(pos + normal * d);
    }
  }

  // 4) Delaunay triangulation over the whole point set (boundary + interior
  //    grid), then discard triangles outside the domain (e.g. inside a
  //    hole, or outside the outer contour). Same helper as the base
  //    surface above, and the same ring constraints (allUV lays its
  //    boundary points out in the same outer-then-holes order as base.uv,
  //    with only interior grid points appended after, so the index pairs
  //    in 'ringConstraints' are valid here unchanged).
  auto finalTris = triangulateAndFilterToDomain(allUV, domain, domainScale, ringConstraints);
  if (finalTris.empty()) return nullptr;

  auto polyset = std::make_unique<PolySet>(3);
  polyset->setTriangular(true);
  polyset->vertices = allPos;
  polyset->indices.reserve(finalTris.size());
  for (const auto& t : finalTris) {
    polyset->indices.push_back({t.a, t.b, t.c});
  }
  return polyset;
}
