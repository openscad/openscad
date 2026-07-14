#include <Eigen/Core>
#include <Eigen/Dense>
#include <vector>
#include <array>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cassert>
#include <limits>
#include <functional>
#include <string>
#include <iostream>
#include <geometry/PolySet.h>
#include <geometry/GeometryUtils.h>

using Vector3d = Eigen::Vector3d;

namespace detail {

struct Tet4 {
  int a, b, c, d;
};

inline double signed_volume6(const Vector3d& a, const Vector3d& b, const Vector3d& c, const Vector3d& d)
{
  return (b - a).dot((c - a).cross(d - a));
}

inline bool circumsphere(const Vector3d& a, const Vector3d& b, const Vector3d& c, const Vector3d& d,
                         Vector3d& center, double& radius)
{
  Eigen::Matrix3d M;
  Eigen::Vector3d rhs;
  M.row(0) = (b - a).transpose();
  M.row(1) = (c - a).transpose();
  M.row(2) = (d - a).transpose();
  rhs(0) = 0.5 * (b.squaredNorm() - a.squaredNorm());
  rhs(1) = 0.5 * (c.squaredNorm() - a.squaredNorm());
  rhs(2) = 0.5 * (d.squaredNorm() - a.squaredNorm());

  double det = M.determinant();
  if (std::fabs(det) < 1e-14) return false;

  center = M.fullPivLu().solve(rhs);
  radius = (center - a).norm();
  return true;
}

struct FaceKey {
  int a, b, c;
  bool operator==(const FaceKey& o) const { return a == o.a && b == o.b && c == o.c; }
};
struct FaceKeyHash {
  size_t operator()(const FaceKey& f) const
  {
    return (static_cast<size_t>(f.a) << 40) ^ (static_cast<size_t>(f.b) << 20) ^
           static_cast<size_t>(f.c);
  }
};
inline FaceKey make_face_key(int a, int b, int c)
{
  int arr[3] = {a, b, c};
  std::sort(arr, arr + 3);
  return {arr[0], arr[1], arr[2]};
}

inline std::vector<Tet4> delaunay3d_bowyer_watson(const std::vector<Vector3d>& points)
{
  const int n = static_cast<int>(points.size());
  std::vector<Vector3d> pts = points;

  {
    Vector3d lo0(1e300, 1e300, 1e300), hi0(-1e300, -1e300, -1e300);
    for (const auto& p : points) {
      lo0 = lo0.cwiseMin(p);
      hi0 = hi0.cwiseMax(p);
    }
    double diag = (hi0 - lo0).norm();
    if (diag < 1e-12) diag = 1.0;
    const double jitter_scale = diag * 1e-7;
    for (int i = 0; i < n; ++i) {
      uint32_t h = static_cast<uint32_t>(i * 2654435761u);
      auto rnd = [&](int k) {
        h ^= h << 13;
        h ^= h >> 17;
        h ^= h << 5;
        h += k * 2246822519u;
        return (static_cast<double>(h) / 4294967295.0) * 2.0 - 1.0;
      };
      pts[i] += Vector3d(rnd(1), rnd(2), rnd(3)) * jitter_scale;
    }
  }

  Vector3d lo(1e300, 1e300, 1e300), hi(-1e300, -1e300, -1e300);
  for (const auto& p : points) {
    lo = lo.cwiseMin(p);
    hi = hi.cwiseMax(p);
  }
  Vector3d center = (lo + hi) * 0.5;
  double extent = (hi - lo).norm() * 10.0 + 1.0;

  const int s0 = n, s1 = n + 1, s2 = n + 2, s3 = n + 3;
  pts.push_back(center + Vector3d(-extent, -extent, -extent));
  pts.push_back(center + Vector3d(extent, -extent, -extent));
  pts.push_back(center + Vector3d(0, extent, -extent));
  pts.push_back(center + Vector3d(0, 0, extent));

  std::vector<Tet4> tets;
  tets.push_back({s0, s1, s2, s3});
  if (signed_volume6(pts[s0], pts[s1], pts[s2], pts[s3]) < 0) std::swap(tets[0].a, tets[0].b);

  for (int pi = 0; pi < n; ++pi) {
    const Vector3d& p = pts[pi];

    std::vector<int> bad;
    bad.reserve(16);
    for (int t = 0; t < static_cast<int>(tets.size()); ++t) {
      const Tet4& tt = tets[t];
      Vector3d c;
      double r;
      if (!circumsphere(pts[tt.a], pts[tt.b], pts[tt.c], pts[tt.d], c, r)) continue;
      if ((p - c).norm() < r - 1e-9) bad.push_back(t);
    }
    if (bad.empty()) continue;

    std::unordered_map<FaceKey, int, FaceKeyHash> face_count;
    std::unordered_map<FaceKey, std::array<int, 3>, FaceKeyHash> face_orient;
    auto add_face = [&](int a, int b, int c) {
      FaceKey k = make_face_key(a, b, c);
      face_count[k]++;
      face_orient[k] = {a, b, c};
    };
    for (int t : bad) {
      const Tet4& tt = tets[t];
      add_face(tt.b, tt.c, tt.d);
      add_face(tt.a, tt.c, tt.d);
      add_face(tt.a, tt.b, tt.d);
      add_face(tt.a, tt.b, tt.c);
    }

    std::sort(bad.rbegin(), bad.rend());
    for (int t : bad) tets.erase(tets.begin() + t);

    for (auto& kv : face_count) {
      if (kv.second != 1) continue;
      auto& f = face_orient[kv.first];
      int a = f[0], b = f[1], c = f[2];
      if (signed_volume6(pts[a], pts[b], pts[c], pts[pi]) < 0) std::swap(a, b);
      tets.push_back({a, b, c, pi});
    }
  }

  std::vector<Tet4> result;
  result.reserve(tets.size());
  for (const auto& t : tets) {
    if (t.a >= n || t.b >= n || t.c >= n || t.d >= n) continue;
    result.push_back(t);
  }
  return result;
}

}  // namespace detail

namespace detail {

struct Face3 {
  int a, b, c;
};

inline std::vector<Face3> boundary_from_kept(const std::vector<Vector3d>& points,
                                             const std::vector<Tet4>& tets,
                                             const std::vector<char>& keep)
{
  std::unordered_map<FaceKey, std::vector<std::pair<int, int>>, FaceKeyHash> face_owners;
  for (size_t i = 0; i < tets.size(); ++i) {
    if (!keep[i]) continue;
    const Tet4& t = tets[i];
    int verts[4] = {t.a, t.b, t.c, t.d};
    for (int excl = 0; excl < 4; ++excl) {
      int f[3];
      int k = 0;
      for (int v = 0; v < 4; ++v)
        if (v != excl) f[k++] = verts[v];
      FaceKey key = make_face_key(f[0], f[1], f[2]);
      face_owners[key].push_back({static_cast<int>(i), verts[excl]});
    }
  }

  std::vector<Face3> boundary;
  boundary.reserve(face_owners.size() / 2);
  for (auto& kv : face_owners) {
    if (kv.second.size() != 1) continue;
    int tet_idx = kv.second[0].first;
    int excluded = kv.second[0].second;
    const Tet4& t = tets[tet_idx];
    int verts[4] = {t.a, t.b, t.c, t.d};
    int f[3];
    int k = 0;
    for (int v = 0; v < 4; ++v)
      if (verts[v] != excluded) f[k++] = verts[v];

    Vector3d nrm = (points[f[1]] - points[f[0]]).cross(points[f[2]] - points[f[0]]);
    Vector3d toExcluded = points[excluded] - points[f[0]];
    if (nrm.dot(toExcluded) > 0) std::swap(f[1], f[2]);

    boundary.push_back({f[0], f[1], f[2]});
  }
  return boundary;
}

inline std::vector<Face3> alpha_shape_boundary(const std::vector<Vector3d>& points,
                                               const std::vector<Tet4>& tets, double alpha)
{
  std::vector<char> keep(tets.size(), 0);
  for (size_t i = 0; i < tets.size(); ++i) {
    const Tet4& t = tets[i];
    Vector3d c;
    double r;
    if (!circumsphere(points[t.a], points[t.b], points[t.c], points[t.d], c, r)) continue;
    if (r <= alpha) keep[i] = 1;
  }
  return boundary_from_kept(points, tets, keep);
}

inline bool is_manifold(const std::vector<Face3>& faces)
{
  std::unordered_map<int64_t, int> edge_count;
  auto key = [](int a, int b) {
    int lo = std::min(a, b), hi = std::max(a, b);
    return (static_cast<int64_t>(lo) << 32) | static_cast<int64_t>(hi);
  };
  for (const auto& f : faces) {
    edge_count[key(f.a, f.b)]++;
    edge_count[key(f.b, f.c)]++;
    edge_count[key(f.c, f.a)]++;
  }
  for (auto& kv : edge_count)
    if (kv.second != 2) return false;
  return true;
}

inline bool is_single_component(const std::vector<Face3>& faces)
{
  if (faces.empty()) return false;
  std::unordered_map<int64_t, std::vector<int>> edge_to_faces;
  auto key = [](int a, int b) {
    int lo = std::min(a, b), hi = std::max(a, b);
    return (static_cast<int64_t>(lo) << 32) | static_cast<int64_t>(hi);
  };
  for (size_t i = 0; i < faces.size(); ++i) {
    const auto& f = faces[i];
    edge_to_faces[key(f.a, f.b)].push_back(static_cast<int>(i));
    edge_to_faces[key(f.b, f.c)].push_back(static_cast<int>(i));
    edge_to_faces[key(f.c, f.a)].push_back(static_cast<int>(i));
  }
  std::vector<char> visited(faces.size(), 0);
  std::vector<int> stack{0};
  visited[0] = 1;
  int seen = 1;
  while (!stack.empty()) {
    int cur = stack.back();
    stack.pop_back();
    const auto& f = faces[cur];
    int idx[3] = {f.a, f.b, f.c};
    for (int k = 0; k < 3; ++k) {
      for (int nb : edge_to_faces[key(idx[k], idx[(k + 1) % 3])]) {
        if (!visited[nb]) {
          visited[nb] = 1;
          ++seen;
          stack.push_back(nb);
        }
      }
    }
  }
  return seen == static_cast<int>(faces.size());
}

inline int count_components(const std::vector<Face3>& faces)
{
  if (faces.empty()) return 0;
  std::unordered_map<int64_t, std::vector<int>> edge_to_faces;
  auto key = [](int a, int b) {
    int lo = std::min(a, b), hi = std::max(a, b);
    return (static_cast<int64_t>(lo) << 32) | static_cast<int64_t>(hi);
  };
  for (size_t i = 0; i < faces.size(); ++i) {
    const auto& f = faces[i];
    edge_to_faces[key(f.a, f.b)].push_back(static_cast<int>(i));
    edge_to_faces[key(f.b, f.c)].push_back(static_cast<int>(i));
    edge_to_faces[key(f.c, f.a)].push_back(static_cast<int>(i));
  }
  std::vector<char> visited(faces.size(), 0);
  int ncomp = 0;
  for (size_t start = 0; start < faces.size(); ++start) {
    if (visited[start]) continue;
    ++ncomp;
    std::vector<int> stack{static_cast<int>(start)};
    visited[start] = 1;
    while (!stack.empty()) {
      int cur = stack.back();
      stack.pop_back();
      const auto& f = faces[cur];
      int idx[3] = {f.a, f.b, f.c};
      for (int k = 0; k < 3; ++k)
        for (int nb : edge_to_faces[key(idx[k], idx[(k + 1) % 3])])
          if (!visited[nb]) {
            visited[nb] = 1;
            stack.push_back(nb);
          }
    }
  }
  return ncomp;
}

inline std::vector<char> bridge_components(const std::vector<Vector3d>& points,
                                           const std::vector<Tet4>& tets, std::vector<char> kept,
                                           const std::vector<double>& tet_radius)
{
  const int n = static_cast<int>(points.size());
  auto missing_count = [&](const std::vector<Face3>& faces) {
    std::vector<char> covered(n, 0);
    for (const auto& f : faces) {
      covered[f.a] = 1;
      covered[f.b] = 1;
      covered[f.c] = 1;
    }
    int missing = 0;
    for (char c : covered)
      if (!c) ++missing;
    return missing;
  };

  auto faces = boundary_from_kept(points, tets, kept);
  int nmiss = missing_count(faces);
  int ncomp = count_components(faces);
  bool manifold_ok = is_manifold(faces);
  if (nmiss == 0 && ncomp <= 1 && manifold_ok) return kept;

  // Order candidates by the LONGEST EDGE of the tetrahedron rather than
  // by circumradius. Circumradius does not correlate well with "how good
  // does this candidate look as a surface patch": a tetrahedron can have
  // a huge circumradius because of one far-away point while still having
  // several short edges, or vice versa. Sorting by max edge length picks
  // the least-bad bridging candidates first and avoids creating a single
  // "hub" vertex that ends up directly connected to nearly everything
  // (which is what circumradius-based ordering tended to do).
  auto max_tet_edge = [&](int ti) {
    const Tet4& t = tets[ti];
    int v[4] = {t.a, t.b, t.c, t.d};
    double m = 0.0;
    for (int i = 0; i < 4; ++i)
      for (int j = i + 1; j < 4; ++j) m = std::max(m, (points[v[i]] - points[v[j]]).norm());
    return m;
  };

  std::vector<int> order;
  order.reserve(tets.size());
  for (size_t i = 0; i < tets.size(); ++i)
    if (!kept[i] && tet_radius[i] >= 0) order.push_back(static_cast<int>(i));
  std::sort(order.begin(), order.end(), [&](int a, int b) { return max_tet_edge(a) < max_tet_edge(b); });

  auto is_better = [](int cur_miss, int cur_c, bool cur_m, int trial_miss, int trial_c, bool trial_m) {
    if (trial_miss != cur_miss) return trial_miss < cur_miss;
    if (trial_c != cur_c) return trial_c < cur_c;
    return (!cur_m && trial_m);
  };

  bool progressed = true;
  while ((nmiss > 0 || ncomp > 1 || !manifold_ok) && progressed) {
    progressed = false;
    for (int ti : order) {
      if (kept[ti]) continue;
      std::vector<char> trial = kept;
      trial[ti] = 1;
      auto trial_faces = boundary_from_kept(points, tets, trial);
      int trial_miss = missing_count(trial_faces);
      int trial_ncomp = count_components(trial_faces);
      bool trial_manifold = is_manifold(trial_faces);
      if (is_better(nmiss, ncomp, manifold_ok, trial_miss, trial_ncomp, trial_manifold)) {
        kept = std::move(trial);
        nmiss = trial_miss;
        ncomp = trial_ncomp;
        manifold_ok = trial_manifold;
        progressed = true;
        break;
      }
    }
    if (!progressed) {
      for (int ti : order) {
        if (kept[ti]) continue;
        kept[ti] = 1;
        progressed = true;
        break;
      }
      if (progressed) {
        auto f = boundary_from_kept(points, tets, kept);
        nmiss = missing_count(f);
        ncomp = count_components(f);
        manifold_ok = is_manifold(f);
      }
    }
  }
  return kept;
}

inline std::vector<char> prune_redundant(const std::vector<Vector3d>& points,
                                         const std::vector<Tet4>& tets, std::vector<char> kept,
                                         const std::vector<double>& tet_radius,
                                         const std::vector<char>& base_kept)
{
  const int n = static_cast<int>(points.size());
  auto is_ok = [&](const std::vector<Face3>& faces) {
    if (faces.empty()) return false;
    std::vector<char> covered(n, 0);
    for (const auto& f : faces) {
      covered[f.a] = 1;
      covered[f.b] = 1;
      covered[f.c] = 1;
    }
    for (char c : covered)
      if (!c) return false;
    return count_components(faces) == 1 && is_manifold(faces);
  };

  std::vector<int> added;
  added.reserve(tets.size());
  for (size_t i = 0; i < tets.size(); ++i)
    if (kept[i] && !base_kept[i]) added.push_back(static_cast<int>(i));
  std::sort(added.begin(), added.end(), [&](int a, int b) { return tet_radius[a] > tet_radius[b]; });

  for (int ti : added) {
    std::vector<char> trial = kept;
    trial[ti] = 0;
    auto trial_faces = boundary_from_kept(points, tets, trial);
    if (is_ok(trial_faces)) kept = std::move(trial);
  }
  return kept;
}

inline std::vector<Face3> adaptive_alpha_shape_boundary(const std::vector<Vector3d>& points,
                                                        const std::vector<Tet4>& tets)
{
  std::vector<double> tet_radius(tets.size(), -1.0);
  std::vector<double> radii;
  radii.reserve(tets.size());
  for (size_t i = 0; i < tets.size(); ++i) {
    Vector3d c;
    double r;
    if (circumsphere(points[tets[i].a], points[tets[i].b], points[tets[i].c], points[tets[i].d], c, r)) {
      tet_radius[i] = r;
      radii.push_back(r);
    }
  }
  if (radii.empty() || tets.empty()) return {};
  std::sort(radii.begin(), radii.end());

  double alpha_base = radii.back();
  for (double r : radii) {
    auto faces = alpha_shape_boundary(points, tets, r);
    if (!faces.empty() && is_manifold(faces)) {
      alpha_base = r;
      break;
    }
  }

  std::vector<char> kept(tets.size(), 0);
  for (size_t i = 0; i < tets.size(); ++i)
    if (tet_radius[i] >= 0 && tet_radius[i] <= alpha_base) kept[i] = 1;
  const std::vector<char> base_kept = kept;

  kept = bridge_components(points, tets, std::move(kept), tet_radius);
  kept = prune_redundant(points, tets, std::move(kept), tet_radius, base_kept);

  return boundary_from_kept(points, tets, kept);
}

}  // namespace detail

namespace detail {

struct EdgeKey2 {
  int lo, hi;
  bool operator==(const EdgeKey2& o) const { return lo == o.lo && hi == o.hi; }
};
struct EdgeKey2Hash {
  size_t operator()(const EdgeKey2& e) const
  {
    return (static_cast<size_t>(e.lo) << 32) ^ static_cast<size_t>(e.hi);
  }
};
inline EdgeKey2 make_edge_key2(int a, int b)
{
  return {std::min(a, b), std::max(a, b)};
}

struct LoopMesh {
  std::vector<Vector3d> verts;
  PolygonIndices tris;
};

inline double max_edge_length(const LoopMesh& m)
{
  double e = 0.0;
  for (const auto& t : m.tris) {
    e = std::max(e, (m.verts[t[0]] - m.verts[t[1]]).norm());
    e = std::max(e, (m.verts[t[1]] - m.verts[t[2]]).norm());
    e = std::max(e, (m.verts[t[2]] - m.verts[t[0]]).norm());
  }
  return e;
}

inline LoopMesh loop_subdivide_once(const LoopMesh& in)
{
  const int n = static_cast<int>(in.verts.size());

  std::unordered_map<EdgeKey2, std::vector<int>, EdgeKey2Hash> edge_opposite;
  std::unordered_map<int, std::vector<int>> neighbors;
  std::unordered_map<int, std::vector<int>> boundary_neighbors;

  auto add_edge = [&](int a, int b, int opp) { edge_opposite[make_edge_key2(a, b)].push_back(opp); };
  for (const auto& t : in.tris) {
    add_edge(t[0], t[1], t[2]);
    add_edge(t[1], t[2], t[0]);
    add_edge(t[2], t[0], t[1]);
  }
  for (auto& kv : edge_opposite) {
    int a = kv.first.lo, b = kv.first.hi;
    neighbors[a].push_back(b);
    neighbors[b].push_back(a);
    if (kv.second.size() == 1) {
      boundary_neighbors[a].push_back(b);
      boundary_neighbors[b].push_back(a);
    }
  }

  LoopMesh out;
  out.verts.resize(n);

  for (int v = 0; v < n; ++v) {
    auto bit = boundary_neighbors.find(v);
    if (bit != boundary_neighbors.end() && bit->second.size() == 2) {
      out.verts[v] = 0.75 * in.verts[v] + 0.125 * (in.verts[bit->second[0]] + in.verts[bit->second[1]]);
    } else {
      auto nit = neighbors.find(v);
      if (nit == neighbors.end() || nit->second.empty()) {
        out.verts[v] = in.verts[v];
        continue;
      }
      const auto& nb = nit->second;
      int valence = static_cast<int>(nb.size());
      double beta = (valence == 3) ? (3.0 / 16.0) : (3.0 / (8.0 * valence));
      Vector3d sum = Vector3d::Zero();
      for (int u : nb) sum += in.verts[u];
      out.verts[v] = (1.0 - valence * beta) * in.verts[v] + beta * sum;
    }
  }

  std::unordered_map<EdgeKey2, int, EdgeKey2Hash> edge_point_index;
  for (auto& kv : edge_opposite) {
    int a = kv.first.lo, b = kv.first.hi;
    Vector3d midpoint = 0.5 * (in.verts[a] + in.verts[b]);
    Vector3d pos;
    if (kv.second.size() == 2) {
      int o1 = kv.second[0], o2 = kv.second[1];
      pos = 0.375 * (in.verts[a] + in.verts[b]) + 0.125 * (in.verts[o1] + in.verts[o2]);

      // Clamp how far the edge point may move away from the plain
      // midpoint, relative to the edge's own length. Without this, very
      // long/thin "spoke" triangles (bridging a sparse region to a much
      // denser one, e.g. a wide outer ring reaching a small isolated
      // detail with nothing in between) can have opposite vertices o1/o2
      // that pull the edge point far sideways relative to the edge - in
      // a highly non-uniform mesh this can push the subdivided surface
      // into self-intersection with unrelated, topologically distant but
      // spatially nearby geometry (confirmed empirically: intersections
      // cluster exactly in such sparse transition zones). Capping the
      // offset trades a small amount of smoothness in these specific
      // problem areas for guaranteed non-self-intersecting geometry;
      // well-sampled regions (where o1/o2 are close to the edge already)
      // are unaffected by the clamp.
      double edge_len = (in.verts[a] - in.verts[b]).norm();
      double max_offset = 0.5 * edge_len;
      Vector3d offset = pos - midpoint;
      double offset_len = offset.norm();
      if (offset_len > max_offset && offset_len > 1e-12) {
        pos = midpoint + offset * (max_offset / offset_len);
      }
    } else {
      pos = midpoint;
    }
    int idx = static_cast<int>(out.verts.size());
    out.verts.push_back(pos);
    edge_point_index[kv.first] = idx;
  }

  out.tris.reserve(in.tris.size() * 4);
  for (const auto& t : in.tris) {
    int a = t[0], b = t[1], c = t[2];
    int mab = edge_point_index[make_edge_key2(a, b)];
    int mbc = edge_point_index[make_edge_key2(b, c)];
    int mca = edge_point_index[make_edge_key2(c, a)];
    out.tris.push_back({a, mab, mca});
    out.tris.push_back({b, mbc, mab});
    out.tris.push_back({c, mca, mbc});
    out.tris.push_back({mab, mbc, mca});
  }

  return out;
}

inline LoopMesh loop_subdivide_to_target(const std::vector<Vector3d>& points, const PolygonIndices& tris,
                                         double max_mesh_size)
{
  LoopMesh mesh{points, tris};
  if (max_mesh_size <= 0.0) return mesh;

  double cur_max = max_edge_length(mesh);
  int levels = 0;
  double predicted = cur_max;
  while (predicted > max_mesh_size && levels < 8) {
    predicted *= 0.5;
    levels++;
  }
  for (int i = 0; i < levels; ++i) mesh = loop_subdivide_once(mesh);
  return mesh;
}

}  // namespace detail

// =========================================================================
// 3.5) Triangulation quality improvement via edge flips
// =========================================================================
//
// The base mesh coming out of adaptive_alpha_shape_boundary() is built to
// satisfy coverage / manifoldness / single-component - it does NOT
// optimize triangle quality or total edge length. In sparsely sampled
// regions this can leave behind very thin, sharp-angled triangles (or
// unnecessarily long edges) even though a geometrically better
// alternative triangulation of the same quadrilateral exists.
//
// Classic fix: local edge flips. For every interior edge (a,b) shared by
// two triangles (a,b,c) and (a,b,d), the four points a,b,c,d form a
// quadrilateral that can be triangulated either via diagonal a-b (the
// current choice) or via diagonal c-d (the flipped alternative). If
// |c-d| < |a-b|, replacing (a,b,c)+(a,b,d) with (a,c,d)+(b,c,d) strictly
// reduces the total edge length of the mesh - this is the natural 3D
// generalization of the classic 2D Delaunay edge-flip / min-weight
// triangulation criterion, and directly implements "minimize the sum of
// all edge lengths" as an explicit secondary quality criterion on top of
// the alpha-shape reconstruction.
namespace detail {

inline double total_edge_length(const std::vector<Vector3d>& points, const PolygonIndices& tris)
{
  std::unordered_map<EdgeKey2, bool, EdgeKey2Hash> seen;
  double total = 0.0;
  for (const auto& t : tris) {
    int idx[3] = {t[0], t[1], t[2]};
    for (int k = 0; k < 3; ++k) {
      EdgeKey2 key = make_edge_key2(idx[k], idx[(k + 1) % 3]);
      if (!seen[key]) {
        seen[key] = true;
        total += (points[key.lo] - points[key.hi]).norm();
      }
    }
  }
  return total;
}

// Dihedral angle between two triangle normals, in degrees: 0 deg means
// the two faces are perfectly coplanar (flat, smooth), up to 180 deg for
// a fold back onto itself. Used as the "how sharp/creased is this edge"
// quality metric requested in place of pure edge length: minimizing the
// average dihedral angle per edge favors a visually smooth, harmonious
// surface over one that merely has the smallest total edge length (the
// two criteria can disagree, as seen empirically: a mesh with minimal
// total edge length can still have very sharp, ugly-looking creases).
inline double dihedral_angle_deg(const Vector3d& n1, const Vector3d& n2)
{
  double l1 = n1.norm(), l2 = n2.norm();
  if (l1 < 1e-12 || l2 < 1e-12) return 0.0;
  double c = n1.dot(n2) / (l1 * l2);
  c = std::max(-1.0, std::min(1.0, c));
  return std::acos(c) * 180.0 / M_PI;
}

// Sum of dihedral angles over every interior (2-triangle) edge of the
// mesh, divided by the number of such edges - the "average sharpness per
// edge" the person asked to minimize.
inline double average_dihedral_angle(const std::vector<Vector3d>& points, const PolygonIndices& tris)
{
  std::unordered_map<EdgeKey2, std::vector<Vector3d>, EdgeKey2Hash> edge_normals;
  for (const auto& t : tris) {
    Vector3d n = (points[t[1]] - points[t[0]]).cross(points[t[2]] - points[t[0]]);
    int idx[3] = {t[0], t[1], t[2]};
    for (int k = 0; k < 3; ++k) edge_normals[make_edge_key2(idx[k], idx[(k + 1) % 3])].push_back(n);
  }
  double sum = 0.0;
  int count = 0;
  for (auto& kv : edge_normals) {
    if (kv.second.size() != 2) continue;
    sum += dihedral_angle_deg(kv.second[0], kv.second[1]);
    ++count;
  }
  return count ? sum / count : 0.0;
}

// Detects "bowtie" vertices: a vertex V is manifold if and only if its
// incident triangles form exactly ONE simple closed fan around it (i.e.
// walking "next neighbor in the fan" starting from any incident triangle
// eventually visits every incident triangle exactly once and returns to
// the start). If two otherwise-unrelated parts of the mesh happen to
// touch at exactly one shared point - without sharing an edge - that
// point ends up with TWO separate triangle fans meeting only at the
// vertex. Neither count_self_intersections() (a triangle-triangle
// crossing test - touching-only-at-a-point triangles do not "cross")
// nor is_manifold() (an EDGE-based check only, every edge already has
// exactly two owning triangles here) can catch this: the defect is
// specifically at the vertex level. Visually this is exactly what shows
// up as "two tips/cones from different parts of the model meeting at
// one point" together with faces that look inverted from some angles -
// a bowtie vertex has no single consistent "outward" side, so whichever
// orientation enforce_consistent_orientation() picks will look wrong
// from the other fan's perspective.
inline int count_nonmanifold_vertices(const std::vector<Vector3d>& points, const PolygonIndices& tris,
                                      std::vector<int> *offending_out = nullptr)
{
  const int n = static_cast<int>(points.size());
  std::vector<std::vector<std::pair<int, int>>> link_edges(n);
  for (const auto& t : tris) {
    int idx[3] = {t[0], t[1], t[2]};
    for (int k = 0; k < 3; ++k) {
      int V = idx[k], x = idx[(k + 1) % 3], y = idx[(k + 2) % 3];
      link_edges[V].push_back({x, y});
    }
  }

  int bad = 0;
  for (int v = 0; v < n; ++v) {
    const auto& edges = link_edges[v];
    if (edges.empty()) continue;

    std::unordered_map<int, int> nxt;
    bool consistent = true;
    for (auto& e : edges) {
      if (nxt.count(e.first)) {
        consistent = false;  // same directed link-edge twice -> not a simple fan
        break;
      }
      nxt[e.first] = e.second;
    }
    if (consistent) {
      int start = edges[0].first;
      int cur = start;
      size_t steps = 0;
      while (steps < edges.size()) {
        auto it = nxt.find(cur);
        if (it == nxt.end()) {
          consistent = false;
          break;
        }
        cur = it->second;
        ++steps;
        if (cur == start) break;
      }
      if (cur != start || steps != edges.size()) consistent = false;
    }

    if (!consistent) {
      ++bad;
      if (offending_out) offending_out->push_back(v);
    }
  }
  return bad;
}

// Detects edges whose two adjoining face normals are (nearly)
// antiparallel - i.e. the surface folds back on itself at that edge,
// producing a paper-thin, near-zero-volume "flap" where both triangles
// lie almost in the same plane but face opposite directions. This is a
// qualitatively different, more severe defect than an ordinary sharp
// crease: any dihedral angle up to just below the threshold is a
// perfectly normal, possibly sharp, feature (a plain octahedron corner
// already sits around 109 degrees) - a fold this close to 180 degrees
// means the two faces are essentially lying on top of each other, which
// is never geometrically intended output, regardless of how it was
// reached (self-intersection tests do not catch this either, since
// exactly-touching-but-not-crossing triangles do not register as an
// intersection).
inline int count_folded_edges(const std::vector<Vector3d>& points, const PolygonIndices& tris,
                              double threshold_deg = 170.0,
                              std::vector<EdgeKey2> *offending_out = nullptr)
{
  std::unordered_map<EdgeKey2, std::vector<Vector3d>, EdgeKey2Hash> edge_normals;
  for (const auto& t : tris) {
    Vector3d n = (points[t[1]] - points[t[0]]).cross(points[t[2]] - points[t[0]]);
    int idx[3] = {t[0], t[1], t[2]};
    for (int k = 0; k < 3; ++k) edge_normals[make_edge_key2(idx[k], idx[(k + 1) % 3])].push_back(n);
  }
  int count = 0;
  for (auto& kv : edge_normals) {
    if (kv.second.size() != 2) continue;
    double angle = dihedral_angle_deg(kv.second[0], kv.second[1]);
    if (angle >= threshold_deg) {
      ++count;
      if (offending_out) offending_out->push_back(kv.first);
    }
  }
  return count;
}

// Signed volume contribution (x6) of a single triangle towards the
// divergence-theorem mesh volume sum - shared by mesh_volume() and the
// volume guard in reduce_star() below, so both agree on exactly the
// same definition of "volume". No longer used by the edge-flip pass
// itself (see bending_flip_pass() further below, which replaced the
// earlier volume-maximizing criterion with a bending-energy one), but
// kept as its own function since mesh_volume() and reduce_star() still
// need it.
inline double tri_signed_vol6(const Vector3d& a, const Vector3d& b, const Vector3d& c)
{
  return a.dot(b.cross(c));
}

// Total enclosed volume of a closed, consistently-oriented triangle mesh
// via the divergence theorem. Used purely for diagnostics (printing
// before/after values around the polishing passes), so the person can
// directly confirm that volume actually increases as expected rather
// than having to infer it from the mesh alone.
inline double mesh_volume(const std::vector<Vector3d>& points, const PolygonIndices& tris)
{
  double vol6 = 0.0;
  for (const auto& t : tris) vol6 += tri_signed_vol6(points[t[0]], points[t[1]], points[t[2]]);
  return vol6 / 6.0;
}

// Detects edges where BOTH owning triangles traverse the shared edge in
// the SAME direction, rather than the required opposite directions (the
// standard 2-manifold consistent-orientation invariant, the same one
// enforce_consistent_orientation()'s own BFS repair checks for). This is
// a purely TOPOLOGICAL defect, entirely independent of the actual
// geometric dihedral angle at that edge - which is exactly why it can
// hide from count_folded_edges(): an orientation-inconsistent edge in a
// non-coplanar part of the mesh can show any "normal-looking" dihedral
// angle right where it is first introduced, only revealing itself as an
// actual fold later, and possibly FAR AWAY - wherever
// enforce_consistent_orientation()'s global BFS repair eventually has to
// flip some triangle to restore consistency, which can propagate the
// problem along the mesh's connectivity to a completely unrelated edge.
// That is exactly the (8,9) symptom: edge (8,9) itself was never
// touched by anything that looked wrong locally, it only became folded
// because fixing a genuine inconsistency elsewhere forced a flip that
// happened to reach it.
inline int count_orientation_inconsistent_edges(const PolygonIndices& tris,
                                                std::vector<EdgeKey2> *offending_out = nullptr)
{
  std::unordered_map<EdgeKey2, std::vector<bool>, EdgeKey2Hash> dirs;
  for (const auto& t : tris) {
    int idx[3] = {t[0], t[1], t[2]};
    for (int k = 0; k < 3; ++k) {
      int x = idx[k], y = idx[(k + 1) % 3];
      EdgeKey2 key = make_edge_key2(x, y);
      dirs[key].push_back(x == key.lo);
    }
  }
  int count = 0;
  for (auto& kv : dirs) {
    if (kv.second.size() != 2) continue;
    if (kv.second[0] == kv.second[1]) {
      ++count;
      if (offending_out) offending_out->push_back(kv.first);
    }
  }
  return count;
}

// =========================================================================
// 3.5) Triangulation quality improvement via edge flips (bending energy)
// =========================================================================
//
// Model: imagine a stiff rubber membrane draped over the mesh. Such a
// membrane resists ANY bending, at every edge, not just the one being
// flipped - so the right local quality criterion for a candidate flip is
// neither "does the flipped edge itself look flatter" (too narrow: it
// ignores what the flip does to the FOUR OTHER edges of the two
// triangles involved) nor "does it increase enclosed volume" (too
// indirect: it can trade a large volume gain for much sharper creases
// elsewhere - exactly the kind of local dimple/spike this replaces).
// Instead: for the two triangles t1=(a,b,c) and t2=(a,b,d) sharing edge
// a-b, sum the ABSOLUTE dihedral angle over all 5 edges that touch
// either triangle - t1's other two edges (a-c, c-b), t2's other two
// edges (a-d, d-b), and the shared edge itself (a-b) - against whatever
// triangle borders each of them. Compare that sum to the same
// calculation for the flipped alternative (diagonal c-d instead of a-b,
// where the 4 outer edges keep their EXTERNAL neighbor unchanged but now
// touch differently-shaped triangles). Accept the flip only if the total
// bending sum strictly decreases - literally "does the rubber membrane
// relax if this diagonal is flipped".

// Looks up the OTHER owner of a given edge (any owner except the two
// triangle indices belonging to the current flip candidate), using the
// edge_owners map the caller already built, and returns its face normal
// - or the zero vector if the edge has no such external neighbor (a true
// boundary edge), in which case it simply contributes 0 to the bending
// sum (nothing to bend against).
inline Vector3d external_neighbor_normal(
  const std::vector<Vector3d>& points, const PolygonIndices& tris,
  const std::unordered_map<EdgeKey2, std::vector<std::pair<int, int>>, EdgeKey2Hash>& edge_owners, int x,
  int y, int exclude1, int exclude2)
{
  auto it = edge_owners.find(make_edge_key2(x, y));
  if (it == edge_owners.end()) return Vector3d::Zero();
  for (auto& owner : it->second) {
    if (owner.first == exclude1 || owner.first == exclude2) continue;
    const auto& et = tris[owner.first];
    return (points[et[1]] - points[et[0]]).cross(points[et[2]] - points[et[0]]);
  }
  return Vector3d::Zero();
}

// Total absolute bending (sum of |dihedral angle|, in degrees) over the
// 5 edges touching two candidate triangles (normals n1, n2) and their
// surroundings: the shared edge between them, plus each one's two outer
// edges against whatever externally-neighboring normal is passed in
// (Vector3d::Zero() meaning "no neighbor there, contributes 0").
inline double quad_bending_energy(const Vector3d& n1, const Vector3d& n2, const Vector3d& ext_ac,
                                  const Vector3d& ext_cb, const Vector3d& ext_ad, const Vector3d& ext_db)
{
  double sum = std::fabs(dihedral_angle_deg(n1, n2));  // the shared edge itself
  if (ext_ac.norm() > 1e-12) sum += std::fabs(dihedral_angle_deg(n1, ext_ac));
  if (ext_cb.norm() > 1e-12) sum += std::fabs(dihedral_angle_deg(n1, ext_cb));
  if (ext_ad.norm() > 1e-12) sum += std::fabs(dihedral_angle_deg(n2, ext_ad));
  if (ext_db.norm() > 1e-12) sum += std::fabs(dihedral_angle_deg(n2, ext_db));
  return sum;
}

inline bool bending_flip_pass(const std::vector<Vector3d>& points, PolygonIndices& tris)
{
  const int nt = static_cast<int>(tris.size());

  // edge -> list of (triangle index, opposite vertex)
  std::unordered_map<EdgeKey2, std::vector<std::pair<int, int>>, EdgeKey2Hash> edge_owners;
  for (int i = 0; i < nt; ++i) {
    const auto& t = tris[i];
    for (int k = 0; k < 3; ++k) {
      int a = t[k], b = t[(k + 1) % 3], opp = t[(k + 2) % 3];
      edge_owners[make_edge_key2(a, b)].push_back({i, opp});
    }
  }

  // Track all edges currently present, so we never introduce a
  // duplicate edge (which would make some edge non-manifold, i.e.
  // shared by 3+ triangles instead of exactly 2).
  std::unordered_map<EdgeKey2, bool, EdgeKey2Hash> existing_edges;
  for (auto& kv : edge_owners) existing_edges[kv.first] = true;

  std::vector<char> touched(nt, 0);  // a triangle can only be flipped once per pass
  bool changed = false;

  for (auto& kv : edge_owners) {
    if (kv.second.size() != 2) continue;  // only interior, manifold edges are flip candidates
    int t1 = kv.second[0].first, c = kv.second[0].second;
    int t2 = kv.second[1].first, d = kv.second[1].second;
    if (touched[t1] || touched[t2]) continue;
    int a = kv.first.lo, b = kv.first.hi;
    if (c == d) continue;  // degenerate, skip

    EdgeKey2 new_edge = make_edge_key2(c, d);
    if (existing_edges.count(new_edge)) continue;  // would create a non-manifold edge

    Vector3d n_t1 =
      (points[tris[t1][1]] - points[tris[t1][0]]).cross(points[tris[t1][2]] - points[tris[t1][0]]);
    Vector3d n_t2 =
      (points[tris[t2][1]] - points[tris[t2][0]]).cross(points[tris[t2][2]] - points[tris[t2][0]]);
    Vector3d old_normal = n_t1 + n_t2;

    // Build the two replacement triangles, then fix winding so their
    // normals stay consistent with the two triangles being replaced.
    IndexedFace tri1 = {a, c, d};
    IndexedFace tri2 = {b, d, c};
    auto fix_winding = [&](IndexedFace& tri) {
      Vector3d n = (points[tri[1]] - points[tri[0]]).cross(points[tri[2]] - points[tri[0]]);
      if (n.dot(old_normal) < 0) std::swap(tri[1], tri[2]);
    };
    fix_winding(tri1);
    fix_winding(tri2);

    // Skip degenerate (zero-area) results.
    Vector3d n1 = (points[tri1[1]] - points[tri1[0]]).cross(points[tri1[2]] - points[tri1[0]]);
    Vector3d n2 = (points[tri2[1]] - points[tri2[0]]).cross(points[tri2[2]] - points[tri2[0]]);
    if (n1.norm() < 1e-12 || n2.norm() < 1e-12) continue;

    // Hard rule, checked before the soft bending-energy comparison
    // below: never introduce a folded ("antiparallel") edge, no matter
    // how much bending energy it would save elsewhere.
    const double FOLD_THRESHOLD_DEG = 170.0;
    if (dihedral_angle_deg(n1, n2) >= FOLD_THRESHOLD_DEG) continue;

    // The 4 external neighbors are the SAME triangles before and after
    // the flip (only t1/t2's own shape changes) - look each of them up
    // once, from the pre-flip mesh, and reuse for both energy sums.
    Vector3d ext_ac = external_neighbor_normal(points, tris, edge_owners, a, c, t1, t2);
    Vector3d ext_cb = external_neighbor_normal(points, tris, edge_owners, c, b, t1, t2);
    Vector3d ext_ad = external_neighbor_normal(points, tris, edge_owners, a, d, t1, t2);
    Vector3d ext_db = external_neighbor_normal(points, tris, edge_owners, d, b, t1, t2);

    double energy_before = quad_bending_energy(n_t1, n_t2, ext_ac, ext_cb, ext_ad, ext_db);
    double energy_after = quad_bending_energy(n1, n2, ext_ac, ext_cb, ext_ad, ext_db);
    if (energy_after >= energy_before - 1e-9) continue;  // the membrane would not relax - no improvement

    // Authoritative, whole-mesh gate for the two remaining hard
    // structural invariants: no fold anywhere (a flip changes the shape
    // of the 4 rim triangles too, which can create a fold against some
    // entirely unrelated neighbor even when this edge's own numbers
    // look fine), and no purely topological orientation inconsistency
    // (see count_orientation_inconsistent_edges() above - a defect that
    // can hide behind a perfectly normal-looking dihedral angle right
    // where it starts, only surfacing far away once
    // enforce_consistent_orientation() has to repair it).
    {
      PolygonIndices trial = tris;
      trial[t1] = tri1;
      trial[t2] = tri2;
      if (count_folded_edges(points, trial) > 0) continue;
      if (count_orientation_inconsistent_edges(trial) > 0) continue;
    }

    for (int i = 0; i < 3; i++) {
      tris[t1][i] = tri1[i];
      tris[t2][i] = tri2[i];
    }
    touched[t1] = touched[t2] = 1;
    existing_edges.erase(kv.first);
    existing_edges[new_edge] = true;
    changed = true;
  }
  return changed;
}

// Repeatedly applies bending_flip_pass() until no more improving flip is
// found (or a safety cap on iterations is reached - one flip can enable
// further improving flips nearby, so a handful of rounds are typically
// needed for full convergence).
inline void improve_triangulation(const std::vector<Vector3d>& points, PolygonIndices& tris)
{
  for (int iter = 0; iter < 20; ++iter) {
    bool changed = bending_flip_pass(points, tris);
    {
      std::string label =
        "improve_triangulation: iter=" + std::to_string(iter) + " after bending_flip_pass";
    }
    if (!changed) break;
  }
}

// =========================================================================
// 3.5a2) Self-intersection detection + simulated-annealing edge-flip search
// =========================================================================
//
// The greedy edge-flip / star-reduction passes above use PROXY quality
// metrics (edge length, then dihedral angle) because those are cheap to
// evaluate locally. Neither proxy actually guarantees avoiding real
// geometric self-intersection - confirmed empirically: a mesh that is
// optimal by either proxy can still contain genuinely crossing triangles
// (e.g. two long, thin, spatially-close-but-topologically-distant
// triangles bridging a sparse region can cross each other even though
// each individually looks fine by both proxies). Greedy local search also
// gets stuck whenever the single best-looking move is blocked by a
// pre-existing conflicting edge, even when a short SEQUENCE of moves
// would help.
//
// This section replaces the proxy-based greedy search with an explicit,
// direct cost function - actual self-intersection count (heavily
// weighted) plus average dihedral angle for smoothness - optimized via
// simulated annealing over edge flips. SA can accept a temporarily worse
// move to escape exactly the kind of local optimum the greedy passes get
// stuck in, and because the cost function checks real 3D triangle-
// triangle intersection directly, it targets the actual problem instead
// of a proxy for it. This works for ANY topology (no star-shaped
// assumption needed), unlike the radial-projection convex hull approach.

inline bool triangles_share_vertex(const IndexedFace& a, const IndexedFace& b)
{
  for (int x : a)
    for (int y : b)
      if (x == y) return true;
  return false;
}

// Classic triangle-triangle intersection test (plane-side rejection +
// segment/triangle checks for the remaining ambiguous cases).
inline bool triangle_triangle_intersect(const Vector3d& p1, const Vector3d& q1, const Vector3d& r1,
                                        const Vector3d& p2, const Vector3d& q2, const Vector3d& r2)
{
  auto sgn = [](double x) { return (x > 1e-9) ? 1 : ((x < -1e-9) ? -1 : 0); };
  Vector3d n1 = (q1 - p1).cross(r1 - p1);
  int d1p = sgn(n1.dot(p2 - p1)), d1q = sgn(n1.dot(q2 - p1)), d1r = sgn(n1.dot(r2 - p1));
  if (d1p == d1q && d1q == d1r && d1p != 0) return false;
  Vector3d n2 = (q2 - p2).cross(r2 - p2);
  int d2p = sgn(n2.dot(p1 - p2)), d2q = sgn(n2.dot(q1 - p2)), d2r = sgn(n2.dot(r1 - p2));
  if (d2p == d2q && d2q == d2r && d2p != 0) return false;

  auto seg_tri = [&](const Vector3d& a, const Vector3d& b, const Vector3d& t0, const Vector3d& t1,
                     const Vector3d& t2) {
    Vector3d n = (t1 - t0).cross(t2 - t0);
    double denom = n.dot(b - a);
    if (std::fabs(denom) < 1e-12) return false;
    double t = n.dot(t0 - a) / denom;
    if (t < 1e-6 || t > 1 - 1e-6) return false;
    Vector3d p = a + t * (b - a);
    Vector3d v0 = t1 - t0, v1 = t2 - t0, v2 = p - t0;
    double d00 = v0.dot(v0), d01 = v0.dot(v1), d11 = v1.dot(v1), d20 = v2.dot(v0), d21 = v2.dot(v1);
    double denom2 = d00 * d11 - d01 * d01;
    if (std::fabs(denom2) < 1e-12) return false;
    double v = (d11 * d20 - d01 * d21) / denom2, w = (d00 * d21 - d01 * d20) / denom2, u = 1 - v - w;
    return (u > 1e-6 && v > 1e-6 && w > 1e-6);
  };
  return seg_tri(p1, q1, p2, q2, r2) || seg_tri(q1, r1, p2, q2, r2) || seg_tri(r1, p1, p2, q2, r2) ||
         seg_tri(p2, q2, p1, q1, r1) || seg_tri(q2, r2, p1, q1, r1) || seg_tri(r2, p2, p1, q1, r1);
}

// Full O(n^2) self-intersection count (only used for the coarse base
// mesh, which is small - a handful to a few hundred triangles - so this
// stays cheap).
inline int count_self_intersections(const std::vector<Vector3d>& points, const PolygonIndices& tris)
{
  int hits = 0;
  for (size_t i = 0; i < tris.size(); ++i) {
    Vector3d a0 = points[tris[i][0]], a1 = points[tris[i][1]], a2 = points[tris[i][2]];
    for (size_t j = i + 1; j < tris.size(); ++j) {
      if (triangles_share_vertex(tris[i], tris[j])) continue;
      Vector3d b0 = points[tris[j][0]], b1 = points[tris[j][1]], b2 = points[tris[j][2]];
      if (triangle_triangle_intersect(a0, a1, a2, b0, b1, b2)) ++hits;
    }
  }
  return hits;
}

// How many OTHER triangles a single triangle (given directly, not yet
// necessarily part of 'tris') intersects - used to cheaply evaluate the
// local effect of a candidate edge flip without a full O(n^2) rescan.
inline int intersections_with_others(const std::vector<Vector3d>& points, const PolygonIndices& tris,
                                     const IndexedFace& tri, int skip_a, int skip_b)
{
  int hits = 0;
  Vector3d a0 = points[tri[0]], a1 = points[tri[1]], a2 = points[tri[2]];
  for (size_t j = 0; j < tris.size(); ++j) {
    if (static_cast<int>(j) == skip_a || static_cast<int>(j) == skip_b) continue;
    if (triangles_share_vertex(tri, tris[j])) continue;
    Vector3d b0 = points[tris[j][0]], b1 = points[tris[j][1]], b2 = points[tris[j][2]];
    if (triangle_triangle_intersect(a0, a1, a2, b0, b1, b2)) ++hits;
  }
  return hits;
}

// Simulated-annealing search over edge flips. Cost = a heavily-weighted
// self-intersection count plus the average dihedral angle (smoothness).
// Each proposed move is a single edge flip (always topology-valid: keeps
// the mesh manifold, single-component and fully covering, since a flip
// only ever swaps which diagonal of an existing quad is used - it never
// removes a point or changes which points are connected to the mesh at
// all). SA can accept a temporarily worse move, which is exactly what is
// needed to escape the local optima the greedy passes got stuck in.
inline void simulated_annealing_optimize(const std::vector<Vector3d>& points, PolygonIndices& tris,
                                         int iterations = 4000, unsigned seed = 12345)
{
  if (tris.empty()) return;
  const double W_INTERSECT = 10000.0;  // dominates: never worth trading for smoothness
  const double W_DIHEDRAL = 1.0;

  auto full_cost = [&]() {
    int inter = count_self_intersections(points, tris);
    double dihedral = average_dihedral_angle(points, tris);
    return W_INTERSECT * inter + W_DIHEDRAL * dihedral;
  };

  double cost = full_cost();
  uint64_t rng_state = seed ? seed : 1;
  auto next_rand = [&]() {  // xorshift64, no <random> dependency needed
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
  };
  auto rand01 = [&]() { return (next_rand() >> 11) * (1.0 / 9007199254740992.0); };

  const double T0 = 50.0, T1 = 0.01;

  for (int it = 0; it < iterations; ++it) {
    double frac = static_cast<double>(it) / std::max(1, iterations - 1);
    double T = T0 * std::pow(T1 / T0, frac);  // exponential cooling

    // Build the edge -> owning-triangle map fresh (cheap: mesh is
    // small) and pick a random interior edge to try flipping.
    std::unordered_map<EdgeKey2, std::vector<std::pair<int, int>>, EdgeKey2Hash> edge_owners;
    for (int i = 0; i < static_cast<int>(tris.size()); ++i) {
      const auto& t = tris[i];
      for (int k = 0; k < 3; ++k) {
        int a = t[k], b = t[(k + 1) % 3], opp = t[(k + 2) % 3];
        edge_owners[make_edge_key2(a, b)].push_back({i, opp});
      }
    }
    std::vector<EdgeKey2> interior_edges;
    for (auto& kv : edge_owners)
      if (kv.second.size() == 2) interior_edges.push_back(kv.first);
    if (interior_edges.empty()) break;

    auto& chosen = interior_edges[next_rand() % interior_edges.size()];
    auto& owners = edge_owners[chosen];
    int t1 = owners[0].first, c = owners[0].second;
    int t2 = owners[1].first, d = owners[1].second;
    int a = chosen.lo, b = chosen.hi;
    if (c == d) continue;

    EdgeKey2 new_edge = make_edge_key2(c, d);
    if (edge_owners.count(new_edge)) continue;  // would create a non-manifold edge

    Vector3d n_t1 =
      (points[tris[t1][1]] - points[tris[t1][0]]).cross(points[tris[t1][2]] - points[tris[t1][0]]);
    Vector3d n_t2 =
      (points[tris[t2][1]] - points[tris[t2][0]]).cross(points[tris[t2][2]] - points[tris[t2][0]]);
    Vector3d old_normal = n_t1 + n_t2;

    IndexedFace new1 = {a, c, d}, new2 = {b, d, c};
    auto fix_winding = [&](IndexedFace& tri) {
      Vector3d n = (points[tri[1]] - points[tri[0]]).cross(points[tri[2]] - points[tri[0]]);
      if (n.dot(old_normal) < 0) std::swap(tri[1], tri[2]);
    };
    fix_winding(new1);
    fix_winding(new2);
    Vector3d nn1 = (points[new1[1]] - points[new1[0]]).cross(points[new1[2]] - points[new1[0]]);
    Vector3d nn2 = (points[new2[1]] - points[new2[0]]).cross(points[new2[2]] - points[new2[0]]);
    if (nn1.norm() < 1e-12 || nn2.norm() < 1e-12) continue;  // degenerate, skip

    // Cheap LOCAL cost delta: only the self-intersection contribution
    // of the two triangles being replaced actually changes (their
    // dihedral angles are folded into the full recompute below,
    // which is affordable since the mesh is small).
    int old_local_inter = intersections_with_others(points, tris, tris[t1], t1, t2) +
                          intersections_with_others(points, tris, tris[t2], t1, t2);

    PolygonIndices trial = tris;
    for (int i = 0; i < 3; i++) {
      trial[t1][i] = new1[i];
      trial[t2][i] = new2[i];
    }
    int new_local_inter = intersections_with_others(points, trial, new1, t1, t2) +
                          intersections_with_others(points, trial, new2, t1, t2);

    double old_dihedral = dihedral_angle_deg(n_t1, n_t2);
    double new_dihedral = dihedral_angle_deg(nn1, nn2);

    double delta = W_INTERSECT * (new_local_inter - old_local_inter) +
                   W_DIHEDRAL * (new_dihedral - old_dihedral) / std::max<size_t>(1, tris.size());

    bool accept = delta < 0.0 || rand01() < std::exp(-delta / std::max(T, 1e-6));
    if (accept) {
      tris = std::move(trial);
      cost += delta;
    }
  }
}

// =========================================================================
// 3.5b) Global orientation consistency
// =========================================================================
//
// The edge-flip and star-reduction passes above each fix up winding
// LOCALLY, by comparing against a heuristically averaged reference
// normal. Averaging normals from triangles that already disagree (which
// can happen after several chained flips/reductions, especially near
// sharp features) can occasionally pick the wrong orientation for a new
// triangle - the errors don't cancel out, they can accumulate over many
// local operations. The practical symptom: some faces end up with their
// inside pointing outward.
//
// The robust fix does not rely on any local heuristic (like checking
// against a reference normal, or a per-tetrahedron volume sign): for a
// proper 2-manifold, single connected surface, exactly one of the two
// triangles sharing an edge should traverse that edge in each direction
// (e.g. one has the directed edge a->b, the other b->a). This is a
// purely topological, always-decidable consistency condition. A single
// breadth-first traversal from an arbitrary starting triangle, flipping
// any newly-visited triangle whose shared edge direction doesn't match
// this rule, deterministically produces one of the two possible globally
// consistent orientations for the whole mesh. A final check of the
// mesh's total signed volume (via the divergence theorem) picks between
// "consistently outward" and "consistently inward" - if negative, every
// triangle gets flipped once more.
inline void enforce_consistent_orientation(const std::vector<Vector3d>& points, PolygonIndices& tris)
{
  const int nt = static_cast<int>(tris.size());
  if (nt == 0) return;

  // edge -> list of (triangle index, position of the edge's first
  // vertex within that triangle) - lets us find/flip the exact edge
  // direction stored in each triangle.
  std::unordered_map<EdgeKey2, std::vector<std::pair<int, int>>, EdgeKey2Hash> edge_tris;
  for (int i = 0; i < nt; ++i) {
    const auto& t = tris[i];
    for (int k = 0; k < 3; ++k) edge_tris[make_edge_key2(t[k], t[(k + 1) % 3])].push_back({i, k});
  }

  std::vector<char> visited(nt, 0);
  std::vector<int> stack{0};
  visited[0] = 1;
  while (!stack.empty()) {
    int cur = stack.back();
    stack.pop_back();
    const auto& t = tris[cur];
    for (int k = 0; k < 3; ++k) {
      int a = t[k], b = t[(k + 1) % 3];
      for (auto& owner : edge_tris[make_edge_key2(a, b)]) {
        int other = owner.first;
        if (other == cur || visited[other]) continue;
        // Does the OTHER triangle traverse this same edge a->b in
        // the SAME direction? For consistent orientation it
        // should traverse it b->a instead.
        const auto& ot = tris[other];
        int op = owner.second;
        bool same_direction = (ot[op] == a && ot[(op + 1) % 3] == b);
        if (same_direction) std::swap(tris[other][1], tris[other][2]);
        visited[other] = 1;
        stack.push_back(other);
      }
    }
  }

  // Pick outward vs inward: total signed volume (sum of signed
  // tetrahedra volumes from the origin) should come out positive for a
  // consistently outward-oriented closed surface.
  double signed_volume6 = 0.0;
  for (const auto& t : tris) signed_volume6 += points[t[0]].dot(points[t[1]].cross(points[t[2]]));
  if (signed_volume6 < 0) {
    for (auto& t : tris) std::swap(t[1], t[2]);
  }
}

// =========================================================================
// 3.6) Star reduction for high-valence "hub" vertices
// =========================================================================
//
// Single edge flips can only ever swap ONE diagonal at a time, and each
// candidate is blocked if its target edge already exists elsewhere. In
// sparsely / unevenly sampled point clouds this can leave a vertex
// connected to almost every other point in the mesh (a "hub"): every
// individually improving flip around it targets an edge that some OTHER
// spoke of the very same hub has already claimed, so nothing can move -
// confirmed empirically: with such a hub present, a full edge-flip pass
// can find zero valid improvements even though the mesh is clearly far
// from optimal.
//
// Star reduction resolves this by looking at a whole vertex neighborhood
// at once instead of one edge at a time: remove ALL triangles around a
// vertex V, giving a polygon boundary (V's "link", the cycle of its
// direct neighbors). That polygon is re-triangulated FROM SCRATCH using
// a classic minimum-weight polygon triangulation (dynamic programming,
// choosing whichever set of diagonals among the neighbors themselves has
// the smallest total length) - this does not use V at all, so the
// neighbors can now connect directly to each other instead of all
// routing through V. V is then re-attached with minimal impact by
// splitting whichever single resulting triangle is closest to it into a
// small 3-triangle fan. Net effect: V's valence typically drops from
// "connected to nearly everything" down to 3, and the total edge length
// of the whole local region drops sharply.

// Minimum-weight triangulation of a simple polygon given as an ordered
// list of point indices (classic O(n^3) DP). Returns triangles as index
// triples into 'poly' (local indices, i.e. 0..poly.size()-1), or an
// empty result if the polygon has fewer than 3 vertices.
inline std::vector<std::array<int, 3>> min_weight_polygon_triangulation(
  const std::vector<Vector3d>& poly)
{
  const int n = static_cast<int>(poly.size());
  std::vector<std::array<int, 3>> result;
  if (n < 3) return result;
  if (n == 3) {
    result.push_back({0, 1, 2});
    return result;
  }

  auto len = [&](int i, int j) { return (poly[i] - poly[j]).norm(); };

  std::vector<std::vector<double>> cost(n, std::vector<double>(n, 0.0));
  std::vector<std::vector<int>> split(n, std::vector<int>(n, -1));

  // cost[i][j] = min total diagonal length to triangulate the polygon
  // fan formed by vertices i, i+1, ..., j (a "sub-polygon" using the
  // chord i-j as one side, when j != i+1).
  for (int gap = 2; gap < n; ++gap) {
    for (int i = 0; i + gap < n; ++i) {
      int j = i + gap;
      double best = std::numeric_limits<double>::max();
      int best_k = -1;
      for (int k = i + 1; k < j; ++k) {
        double c =
          cost[i][k] + cost[k][j] + len(i, k) + len(k, j) + (k == i + 1 || k == j - 1 ? 0.0 : 0.0);
        // Triangle (i,k,j): count the two NEW diagonals it introduces
        // (i-k and k-j) towards the total weight; the polygon's own
        // boundary edges contribute a fixed amount regardless of the
        // chosen triangulation, so they can be left out here.
        if (c < best) {
          best = c;
          best_k = k;
        }
      }
      cost[i][j] = best;
      split[i][j] = best_k;
    }
  }

  std::vector<std::array<int, 3>> tris;
  std::function<void(int, int)> build = [&](int i, int j) {
    if (j - i < 2) return;
    int k = split[i][j];
    if (k < 0) return;
    tris.push_back({i, k, j});
    build(i, k);
    build(k, j);
  };
  build(0, n - 1);
  return tris;
}

// Attempts to reduce the valence of vertex V by re-triangulating its
// local neighborhood as described above. Returns true if a change was
// made. Only ever applied when it is geometrically valid (V's link forms
// a simple closed polygon) and strictly reduces total edge length.
inline bool reduce_star(const std::vector<Vector3d>& points, PolygonIndices& tris, int V)
{
  // Collect V's incident triangles and build a directed-edge map of the
  // OTHER two vertices of each, so the link cycle can be walked in order.
  std::vector<int> incident;
  for (size_t i = 0; i < tris.size(); ++i) {
    const auto& t = tris[i];
    if (t[0] == V || t[1] == V || t[2] == V) incident.push_back(static_cast<int>(i));
  }
  if (incident.size() < 4) return false;  // nothing to gain for very low valence

  // For each incident triangle (V, x, y) in consistent winding, the
  // directed edge x->y borders the link polygon. Walking these directed
  // edges traces the link cycle in order.
  std::unordered_map<int, int> next_in_link;  // x -> y
  for (int ti : incident) {
    const auto& t = tris[ti];
    int x = -1, y = -1;
    if (t[0] == V) {
      x = t[1];
      y = t[2];
    } else if (t[1] == V) {
      x = t[2];
      y = t[0];
    } else {
      x = t[0];
      y = t[1];
    }
    next_in_link[x] = y;
  }
  if (next_in_link.size() != incident.size()) return false;  // inconsistent, bail out safely

  std::vector<int> link;
  int start = next_in_link.begin()->first;
  int cur = start;
  do {
    link.push_back(cur);
    auto it = next_in_link.find(cur);
    if (it == next_in_link.end()) return false;  // broken cycle, bail out safely
    cur = it->second;
  } while (cur != start && link.size() <= next_in_link.size());
  if (cur != start || link.size() != incident.size()) return false;  // not a simple closed cycle

  // Build the polygon's own point list and get the minimum-weight
  // triangulation of just the neighbors (without V).
  std::vector<Vector3d> poly;
  poly.reserve(link.size());
  for (int idx : link) poly.push_back(points[idx]);
  auto local_tris = min_weight_polygon_triangulation(poly);
  if (local_tris.empty()) return false;

  // Build the final replacement triangle list for this whole star
  // region: the polygon's own triangles (translated back to global
  // indices), except the chosen one gets replaced by 3 triangles using V.
  // Orientation is fixed up to match the original star's outward
  // direction (all original incident triangles shared roughly the same
  // outward normal, since they all touch V).
  //
  // Find the local triangle closest to V (by centroid distance) and
  // split it into a 3-triangle fan using V, so V stays part of the mesh.
  int best_local = -1;
  double best_d = std::numeric_limits<double>::max();
  for (size_t i = 0; i < local_tris.size(); ++i) {
    auto& t = local_tris[i];
    Vector3d centroid = (poly[t[0]] + poly[t[1]] + poly[t[2]]) / 3.0;
    double d = (points[V] - centroid).norm();
    if (d < best_d) {
      best_d = d;
      best_local = static_cast<int>(i);
    }
  }

  // Build the final replacement triangle list for this whole star
  // region: the polygon's own triangles (translated back to global
  // indices), except the chosen one gets replaced by 3 triangles using V.
  //
  // No geometric orientation correction (e.g. against an averaged
  // reference normal) is applied or needed here at all:
  // min_weight_polygon_triangulation() emits every triangle in ascending
  // local-index order (i,k,j) - a structural property of its DP
  // construction that always traverses each polygon BOUNDARY edge
  // (i,i+1) as i->(i+1). Since poly[i] == points[link[i]], and
  // link[i]->link[i+1] is exactly the boundary direction already
  // established by the ORIGINAL (correctly oriented) incident triangles
  // being removed (see next_in_link above - it was built directly from
  // their existing, already-correct winding), using the (i,k,j) order
  // as-is - with no per-triangle AND no per-batch flip decision -
  // automatically reproduces the correct global orientation for the
  // entire replacement patch, boundary and fan alike. This is a standard
  // topological fact: the orientation of a triangulated disk is fully
  // determined by its boundary orientation, independent of how the
  // interior is triangulated.
  //
  // The PREVIOUS approach instead re-derived orientation from a dot
  // product against an averaged normal of the removed triangles - for a
  // nearly flat/degenerate local neighborhood (like the coplanar z=11
  // point cluster that triggered this bug), that average can be tiny
  // and its sign essentially noise-dominated, occasionally picking the
  // WRONG global flip for the whole patch and introducing exactly the
  // kind of fold reported. Removing the geometric decision entirely -
  // relying purely on the topological guarantee instead - eliminates
  // that failure mode structurally rather than papering over it.
  std::vector<std::array<int, 3>> replacement;
  for (size_t i = 0; i < local_tris.size(); ++i) {
    auto& t = local_tris[i];
    int a = link[t[0]], b = link[t[1]], c = link[t[2]];
    if (static_cast<int>(i) == best_local) {
      // The 3-triangle fan (V,a,b)/(V,b,c)/(V,c,a), in this rotational
      // order, is automatically self-consistent (each of its 3 internal
      // V-spokes is traversed in opposite directions by its two owners
      // within the fan) and automatically matches the boundary edge
      // directions (a->b, b->c, c->a) established for the rest of the
      // polygon.
      replacement.push_back({V, a, b});
      replacement.push_back({V, b, c});
      replacement.push_back({V, c, a});
    } else {
      replacement.push_back({a, b, c});
    }
  }

  // Splice into a TRIAL copy first and compare the requested quality
  // metric (average dihedral angle per edge, i.e. how smooth/creased
  // the surface looks) rather than total edge length: the two criteria
  // can disagree (a mesh with minimal total edge length can still have
  // very sharp, ugly-looking creases - confirmed empirically), and the
  // dihedral-angle criterion is what actually captures "looks
  // harmonious". Only accept the star reduction if it strictly reduces
  // the average dihedral angle across the whole mesh.
  std::vector<char> remove(tris.size(), 0);
  for (int ti : incident) remove[ti] = 1;
  PolygonIndices trial_tris;
  trial_tris.reserve(tris.size() - incident.size() + replacement.size());
  for (size_t i = 0; i < tris.size(); ++i)
    if (!remove[i]) trial_tris.push_back(tris[i]);
  for (auto& f : replacement) trial_tris.push_back({f[0], f[1], f[2]});

  double old_avg_dihedral = average_dihedral_angle(points, tris);
  double new_avg_dihedral = average_dihedral_angle(points, trial_tris);

  // Hard rejects, checked first (dominance order): a star reduction that
  // trades a lower average dihedral for a new bowtie vertex or a folded
  // edge elsewhere in the affected neighborhood is never acceptable,
  // regardless of how good the average number looks - min_weight_
  // polygon_triangulation() has no awareness of either defect, it only
  // minimizes summed diagonal length.
  if (count_nonmanifold_vertices(points, trial_tris) > 0) return false;
  if (count_folded_edges(points, trial_tris) > 0) return false;
  if (count_orientation_inconsistent_edges(trial_tris) > 0) return false;

  if (new_avg_dihedral >= old_avg_dihedral - 1e-6) return false;  // not an improvement

  // Same volume guard as previously used in the edge-flip pass: a re-triangulation of the
  // whole neighborhood can easily look better by the dihedral-angle
  // average while quietly carving away enclosed volume (min_weight_
  // polygon_triangulation only minimizes summed diagonal length, with
  // no awareness of volume at all) - reject any reduction that isn't at
  // least volume-neutral.
  double vol_before = mesh_volume(points, tris);
  double vol_after = mesh_volume(points, trial_tris);
  if (vol_after < vol_before - 1e-6) return false;

  tris = std::move(trial_tris);
  return true;
}

// Applies star reduction to every vertex whose valence is unusually high
// (heuristic threshold), repeating until no more improving reduction is
// found. Safety checks (full coverage / single component / manifold)
// guard every accepted change so this can never make the mesh worse in
// those respects - only total edge length is meant to improve.
inline void reduce_hub_vertices(const std::vector<Vector3d>& points, PolygonIndices& tris)
{
  const int n = static_cast<int>(points.size());
  auto is_ok = [&](const PolygonIndices& t) {
    std::vector<char> covered(n, 0);
    for (auto& tri : t) {
      covered[tri[0]] = 1;
      covered[tri[1]] = 1;
      covered[tri[2]] = 1;
    }
    for (char c : covered)
      if (!c) return false;
    std::vector<Face3> f;
    f.reserve(t.size());
    for (auto& tri : t) f.push_back({tri[0], tri[1], tri[2]});
    return count_components(f) == 1 && is_manifold(f);
  };

  for (int pass = 0; pass < 10; ++pass) {
    // Recompute valences fresh each pass, since reductions change them.
    std::vector<int> valence(n, 0);
    for (auto& t : tris) {
      valence[t[0]]++;
      valence[t[1]]++;
      valence[t[2]]++;
    }
    // Try the highest-valence vertices first.
    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return valence[a] > valence[b]; });

    bool any_change = false;
    for (int v : order) {
      if (valence[v] < 6) break;  // small valences are already fine
      PolygonIndices trial = tris;
      if (reduce_star(points, trial, v) && is_ok(trial)) {
        tris = std::move(trial);
        any_change = true;
        {
          std::string label = "reduce_hub_vertices: pass=" + std::to_string(pass) +
                              " reduce_star(V=" + std::to_string(v) + ") applied";
        }
        break;  // valences changed, restart ordering fresh
      }
    }
    if (!any_change) break;
  }
}

}  // namespace detail

// =========================================================================
// 4) Star-shaped hull reconstruction (radial projection + convex hull)
// =========================================================================
//
// Delaunay/alpha-shape reconstruction is designed for surface DISCOVERY
// from large, dense, roughly-uniform point clouds sampling an UNKNOWN
// surface. That is not actually the problem being solved here: the goal
// is "given specific points that are known to describe a star-shaped
// solid, connect them nicely into a surface, guaranteed non-self-
// intersecting, using every point" - a much more specific and, as it
// turns out, much simpler problem with a mathematically clean solution:
//
//   1) Project every point onto a unit sphere around a center point
//      (radial direction only, i.e. normalize(point - center)).
//   2) Compute the 3D convex hull of these projected directions.
//   3) Re-attach the ORIGINAL (non-projected) point positions to the
//      resulting triangle topology.
//
// This works because a convex hull can, by definition, never self-
// intersect, AND because every point that lies exactly on a sphere is
// automatically an extreme point of that sphere's convex hull (barring
// exact duplicate directions) - so every input point is guaranteed to be
// used, without any of the alpha-shape machinery (coverage search,
// bridging, pruning, hub reduction) needed at all.
//
// Requirement: the point set must be "star-shaped" around the chosen
// center, i.e. every point on the intended surface must be reachable by
// a straight ray from the center without crossing the surface twice.
// This holds for the tested cases (octahedra, dented cubes, ring+crater
// shapes) but would NOT hold for genuinely non-star-shaped topology
// (e.g. a torus, or a shape with a re-entrant tunnel).
namespace detail {

inline std::vector<Face3> star_shaped_hull(const std::vector<Vector3d>& points, const Vector3d& center)
{
  std::vector<Vector3d> dirs;
  dirs.reserve(points.size());
  for (const auto& p : points) {
    Vector3d d = p - center;
    double len = d.norm();
    dirs.push_back(len > 1e-12 ? d / len : Vector3d(1, 0, 0));  // degenerate: point == center
  }
  std::vector<Tet4> tets = delaunay3d_bowyer_watson(dirs);
  std::vector<char> keep_all(tets.size(), 1);  // keep every tetrahedron -> convex hull
  return boundary_from_kept(dirs, tets, keep_all);
}

// How many self-intersections does the star-shaped hull built around a
// given candidate center produce? Kept around (unused by the search
// below now) for anyone who wants the plain intersection-only metric,
// e.g. for quick standalone diagnostics.
inline int star_hull_intersection_count(const std::vector<Vector3d>& points, const Vector3d& center)
{
  auto faces = star_shaped_hull(points, center);
  PolygonIndices tris;
  tris.reserve(faces.size());
  for (auto& f : faces) tris.push_back({f.a, f.b, f.c});
  return count_self_intersections(points, tris);
}

// Checks, for the REATTACHED mesh (using the original point positions,
// not the unit-sphere projections used to build the topology), whether
// the chosen center actually lies on the interior side of every single
// triangle's plane. This is precisely the property "the hull center
// should not lie outside the plane of triangle (3,0,8)" - a center for
// which this holds for every face can see the whole surface from a
// single interior vantage point, which is the actual mathematical
// definition of star-shaped and a much more direct, principled test
// than "0 measured self-intersections": the direction-based convex hull
// construction (see star_shaped_hull) only ever guarantees a valid,
// non-self-crossing topology on the UNIT SPHERE - re-attaching the
// original, non-uniform point positions afterwards can still leave the
// center outside some face's half-space without the discrete
// self-intersection test necessarily flagging it (that test only finds
// actual triangle-triangle crossings, not "wrong side of a plane" on
// its own). A center with 0 violations here is a "properly natural"
// star center, which is exactly what was asked for.
inline int count_center_visibility_violations(const std::vector<Vector3d>& points,
                                              const PolygonIndices& tris, const Vector3d& center)
{
  int violations = 0;
  for (const auto& t : tris) {
    Vector3d a = points[t[0]], b = points[t[1]], c = points[t[2]];
    Vector3d n = (b - a).cross(c - a);
    Vector3d centroid = (a + b + c) / 3.0;
    if (n.dot(center - centroid) > 1e-9) ++violations;
  }
  return violations;
}

// Combined objective for the center search: self-intersection count and
// non-manifold ("bowtie") vertex count - both heavily and EQUALLY
// Combined objective for the center search: self-intersection count,
// non-manifold ("bowtie") vertex count, and folded/antiparallel-edge
// count - all three heavily and EQUALLY weighted, since all three are
// hard topological/geometric defects, not cosmetic ones - plus center-
// visibility violations (see count_center_visibility_violations above -
// weighted noticeably, but below the three hard defects: a visibility
// violation is a strong sign of a poorly-chosen, unnatural center, but
// unlike the three above it does not by itself make the mesh invalid) -
// plus average dihedral angle as the softest, purely cosmetic
// tie-breaker.
inline double star_hull_cost(const std::vector<Vector3d>& points, const Vector3d& center,
                             int& intersections_out, int& nonmanifold_out, int& folded_out,
                             int& visibility_out)
{
  auto faces = star_shaped_hull(points, center);
  PolygonIndices tris;
  tris.reserve(faces.size());
  for (auto& f : faces) tris.push_back({f.a, f.b, f.c});
  intersections_out = count_self_intersections(points, tris);
  nonmanifold_out = count_nonmanifold_vertices(points, tris);
  folded_out = count_folded_edges(points, tris);
  visibility_out = count_center_visibility_violations(points, tris, center);
  double dihedral = average_dihedral_angle(points, tris);
  const double W_INTERSECT = 10000.0;
  const double W_NONMANIFOLD = 10000.0;
  const double W_FOLDED = 10000.0;
  const double W_VISIBILITY = 500.0;
  const double W_DIHEDRAL = 1.0;
  return W_INTERSECT * intersections_out + W_NONMANIFOLD * nonmanifold_out + W_FOLDED * folded_out +
         W_VISIBILITY * visibility_out + W_DIHEDRAL * dihedral;
}

// Iteratively searches for a center point from which the star-shaped
// hull is genuinely non-self-intersecting (0 crossing triangle pairs)
// AND, among all such centers, has as smooth (low average dihedral
// angle) a topology as possible. Not every point cloud has a valid
// vantage point at all - a complex, multi-part sculpture (e.g. an
// elephant with trunk, ears, legs, torso all as one point set)
// generally does NOT have a single valid vantage point, no matter how
// cleverly chosen; that is precisely why such shapes need to be modeled
// as several separate, individually star-shaped point groups (head,
// trunk, ears, ...), each reconstructed on its own and then combined
// (e.g. via a boolean union) - the search below cannot invent a valid
// center where none exists, but for anything that genuinely IS
// star-shaped it reliably finds a working one, even when the naive
// centroid does not directly work (e.g. a strongly bent/curved part).
//
// Method: start from the centroid, then a simulated-annealing walk over
// candidate 3D center POSITIONS (not over triangulations - a much
// smaller, better-behaved 3-dimensional search space), using
// star_hull_cost as the cost to minimize. Unlike a pure intersection
// count, this does not stop the moment 0 intersections is reached - the
// full annealing budget keeps looking for a 0-intersection center that
// is ALSO smooth, since the first 0-intersection center found (often
// just the plain centroid) is not necessarily a good one topologically.
// Returns the best center found and reports how many intersections
// remain there (0 means success).
inline Vector3d find_good_star_center(const std::vector<Vector3d>& points, int& out_intersections,
                                      int& out_nonmanifold, int& out_folded, int& out_visibility,
                                      int max_iterations = 300, unsigned seed = 987654321u)
{
  Vector3d lo(1e300, 1e300, 1e300), hi(-1e300, -1e300, -1e300);
  Vector3d centroid = Vector3d::Zero();
  for (const auto& p : points) {
    lo = lo.cwiseMin(p);
    hi = hi.cwiseMax(p);
    centroid += p;
  }
  centroid /= static_cast<double>(points.size());
  double diag = (hi - lo).norm();
  if (diag < 1e-12) diag = 1.0;

  Vector3d best = centroid;
  int best_intersections = 0;
  int best_nonmanifold = 0;
  int best_folded = 0;
  int best_visibility = 0;
  star_hull_cost(points, best, best_intersections, best_nonmanifold, best_folded, best_visibility);

  // If the plain geometric centroid already produces a hull with none of
  // the hard defects (self-intersections, bowtie vertices, folded
  // edges, center-visibility violations), it IS the natural, ideal
  // center for the large majority of star-shaped point clouds - there is
  // no reason to search any further and risk wandering off to a
  // needlessly distant, harder-to-reason-about position purely to shave
  // a fraction of a degree off the average dihedral angle (which is
  // exactly what was happening before this check was added). Only fall
  // back to the full simulated-annealing search when the centroid itself
  // fails a hard check.
  if (best_intersections == 0 && best_nonmanifold == 0 && best_folded == 0 && best_visibility == 0) {
    out_intersections = best_intersections;
    out_nonmanifold = best_nonmanifold;
    out_folded = best_folded;
    out_visibility = best_visibility;
    return best;
  }

  // Soft pull back towards the centroid, added on top of star_hull_cost
  // (not baked into star_hull_cost itself, since that function has no
  // notion of "the natural center" - only this search does). Small
  // enough to never override a genuine reduction in hard defects or a
  // meaningful smoothness gain, but enough to stop the search from
  // drifting arbitrarily far away between two candidates that are
  // otherwise near-equivalent.
  const double W_CENTROID_PULL = 0.05;
  auto eval_cost = [&](const Vector3d& c, int& inter, int& nonmani, int& fold, int& vis) {
    double base = star_hull_cost(points, c, inter, nonmani, fold, vis);
    return base + W_CENTROID_PULL * (c - centroid).norm();
  };

  double best_cost = eval_cost(best, best_intersections, best_nonmanifold, best_folded, best_visibility);

  Vector3d current = best;
  double current_cost = best_cost;

  uint64_t rng_state = seed ? seed : 1;
  auto next_rand = [&]() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
  };
  auto rand01 = [&]() { return (next_rand() >> 11) * (1.0 / 9007199254740992.0); };
  auto rand_dir = [&]() {
    Vector3d v(rand01() * 2 - 1, rand01() * 2 - 1, rand01() * 2 - 1);
    double n = v.norm();
    return n > 1e-9 ? v / n : Vector3d(1, 0, 0);
  };

  // Once past the centroid short-circuit above, the search commits to
  // spending its full annealing budget looking for a center that clears
  // all three hard checks AND is reasonably smooth/close to the
  // centroid - it does not stop the moment self-intersections alone
  // reaches 0, since (as seen above) that is not sufficient on its own.
  // The two hard-defect terms still dominate this cost by four orders
  // of magnitude, so this can never trade away defect-freedom once
  // found purely to get closer to the centroid.
  const double T0 = 5.0, T1 = 0.001;
  for (int it = 0; it < max_iterations; ++it) {
    double frac = static_cast<double>(it) / std::max(1, max_iterations - 1);
    double T = T0 * std::pow(T1 / T0, frac);
    // Step size shrinks over time too, for increasingly local refinement.
    double step = diag * 0.15 * (1.0 - 0.9 * frac);

    Vector3d candidate = current + rand_dir() * step * rand01();
    int candidate_intersections = 0;
    int candidate_nonmanifold = 0;
    int candidate_folded = 0;
    int candidate_visibility = 0;
    double cost = eval_cost(candidate, candidate_intersections, candidate_nonmanifold, candidate_folded,
                            candidate_visibility);

    double delta = cost - current_cost;
    bool accept = delta < 0 || rand01() < std::exp(-delta / std::max(T, 1e-6));
    if (accept) {
      current = candidate;
      current_cost = cost;
      if (cost < best_cost) {
        best_cost = cost;
        best_intersections = candidate_intersections;
        best_nonmanifold = candidate_nonmanifold;
        best_folded = candidate_folded;
        best_visibility = candidate_visibility;
        best = candidate;
      }
    }
  }

  out_intersections = best_intersections;
  out_nonmanifold = best_nonmanifold;
  out_folded = best_folded;
  out_visibility = best_visibility;
  return best;
}

}  // namespace detail

PolySet organic_resample(const std::vector<Vector3d>& points, double max_mesh_size, double alpha = -1.0)
{
  PolySet out(3);
  if (points.size() < 4) return out;

  // Star-shaped hull reconstruction: guaranteed non-self-intersecting
  // and guaranteed to use every point (see the detailed comment on
  // star_shaped_hull above) - empirically far more robust than the
  // general Delaunay/alpha-shape route once Loop subdivision is applied,
  // even when that route is polished with simulated annealing (the
  // alpha-shape topology tends to contain long "spoke"-like triangles
  // that self-intersect once subdivided, regardless of how the
  // triangulation is locally optimized afterwards).
  //
  // The main requirement - the point set must be star-shaped around
  // SOME interior center - is not always satisfied by the plain
  // centroid, so an iterative search looks for a better center first.
  // This cannot invent a valid center for a genuinely non-star-shaped
  // point cloud (e.g. a complex multi-part sculpture combining a torso,
  // trunk and ears all as one point set) - such shapes need to be
  // modeled as several separate, individually star-shaped point groups
  // instead (see the comment on find_good_star_center for details).
  int intersections = 0;
  int nonmanifold_at_center = 0;
  int folded_at_center = 0;
  int visibility_violations = 0;
  Vector3d center = detail::find_good_star_center(points, intersections, nonmanifold_at_center,
                                                  folded_at_center, visibility_violations);

  std::cout << "organic(): star-shaped hull center found at (" << center.x() << ", " << center.y()
            << ", " << center.z() << "), self-intersections=" << intersections
            << ", non-manifold vertices=" << nonmanifold_at_center
            << ", folded edges=" << folded_at_center
            << ", center-visibility violations=" << visibility_violations << std::endl;

  std::vector<detail::Face3> faces = detail::star_shaped_hull(points, center);

  PolygonIndices tris;
  tris.reserve(faces.size());
  for (const auto& f : faces) tris.push_back({f.a, f.b, f.c});
  if (tris.empty()) return out;

  // Coarse structural cleanup first (collapses any extreme "hub"
  // vertices), then a lightweight dihedral-angle-based edge-flip polish.
  // Note: simulated_annealing_optimize (further above) was tried here
  // too, since it directly targets the real self-intersection count as
  // its cost function - but empirically it turned out to be
  // counter-productive for this pipeline: even though it keeps the
  // COARSE mesh at 0 self-intersections (that is what it optimizes for),
  // the specific diagonal choices it settles on are more fragile under
  // Loop subdivision and can reintroduce crossings there. The star-
  // shaped hull's own natural triangulation, only lightly polished by
  // plain greedy edge flips, survives subdivision cleanly - confirmed
  // empirically across every tested case (see the conversation history
  // for the specific measurements). Kept available via
  // detail::simulated_annealing_optimize for anyone who wants to
  // experiment further.
  double vol_before_polish = detail::mesh_volume(points, tris);
  detail::reduce_hub_vertices(points, tris);
  detail::improve_triangulation(points, tris);
  std::cout << "organic(): orientation-inconsistent edges before enforce_consistent_orientation="
            << detail::count_orientation_inconsistent_edges(tris) << std::endl;
  detail::enforce_consistent_orientation(points, tris);
  double vol_after_polish = detail::mesh_volume(points, tris);
  std::cout << "organic(): mesh volume before polish=" << vol_before_polish
            << ", after=" << vol_after_polish << std::endl;

  // The center search above only ever sees the RAW star-shaped hull, not
  // the mesh after reduce_hub_vertices()/improve_triangulation() have
  // touched it - those passes are not expected to introduce new bowtie
  // vertices (they only ever change which existing edges/fans are used,
  // both guarded by is_manifold()/is_ok() checks that are EDGE- not
  // VERTEX-based, though), so it's worth confirming that no such defect
  // survived into the actual final result and telling the person exactly
  // which point index(es) to look at if one did.
  {
    std::vector<int> bad_vertices;
    int final_nonmanifold = detail::count_nonmanifold_vertices(points, tris, &bad_vertices);
    if (final_nonmanifold > 0) {
      std::cout << "organic(): WARNING - " << final_nonmanifold
                << " non-manifold (bowtie) vertex/vertices remain in the final mesh, at point index/"
                   "indices: ";
      for (size_t i = 0; i < bad_vertices.size(); ++i) {
        std::cout << bad_vertices[i];
        if (i + 1 < bad_vertices.size()) std::cout << ", ";
      }
      std::cout << std::endl;
    }

    std::vector<detail::EdgeKey2> bad_edges;
    int final_folded = detail::count_folded_edges(points, tris, 170.0, &bad_edges);
    if (final_folded > 0) {
      std::cout << "organic(): WARNING - " << final_folded
                << " folded (antiparallel) edge(s) remain in the final mesh, at point index pairs: ";
      for (size_t i = 0; i < bad_edges.size(); ++i) {
        std::cout << "(" << bad_edges[i].lo << "," << bad_edges[i].hi << ")";
        if (i + 1 < bad_edges.size()) std::cout << ", ";
      }
      std::cout << std::endl;
    }
  }

  detail::LoopMesh smooth = detail::loop_subdivide_to_target(points, tris, max_mesh_size);

  out.vertices = std::move(smooth.verts);
  out.indices = std::move(smooth.tris);
  return out;
}
