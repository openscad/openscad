#include "core/CurveDiscretizer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>
#include <ostream>
#include <queue>
#include <utility>
#include <vector>

#include "Feature.h"
#include "core/AST.h"  // for Location
#include "core/Parameters.h"
#include "geometry/Grid.h"
#include "geometry/Polygon2d.h"
#include "utils/calc.h"
#include "utils/degree_trig.h"
#include "utils/printutils.h"

#define F_MINIMUM 0.01

CurveDiscretizer::CurveDiscretizer(const Parameters& parameters, const Location& loc)
{
  fn = parameters["$fn"].toDouble();
  if (Feature::ExperimentalDiscretizationByError.is_enabled()) {
    fe = parameters["$fe"].toDouble();
  } else {
    fe = 0.0;
  }
  fs = parameters["$fs"].toDouble();
  fa = parameters["$fa"].toDouble();

  if (fn < 0.0) {
    LOG(message_group::Warning, loc, parameters.documentRoot(), "$fn negative - setting to 0");
    fn = 0.0;
  }
  if (Feature::ExperimentalDiscretizationByError.is_enabled() && fe < 0.0) {
    LOG(message_group::Warning, loc, parameters.documentRoot(), "$fe negative - setting to 0");
    fe = 0.0;
  }
  if (fs < F_MINIMUM) {
    LOG(message_group::Warning, loc, parameters.documentRoot(), "$fs too small - clamping to %1$f",
        F_MINIMUM);
    fs = F_MINIMUM;
  }
  if (fa < F_MINIMUM) {
    LOG(message_group::Warning, loc, parameters.documentRoot(), "$fa too small - clamping to %1$f",
        F_MINIMUM);
    fa = F_MINIMUM;
  }
}

CurveDiscretizer::CurveDiscretizer(const Parameters& parameters)
{
  fn = std::max(parameters["$fn"].toDouble(), 0.0);
  fe = std::max(parameters["$fe"].toDouble(), 0.0);
  fs = std::max(parameters["$fs"].toDouble(), F_MINIMUM);
  fa = std::max(parameters["$fa"].toDouble(), F_MINIMUM);
}

CurveDiscretizer::CurveDiscretizer(double segmentsPerCircle)
{
  fn = segmentsPerCircle;
  fe = 0;
  fs = 0;
  fa = 0;

  if (fn < 5.0) {
    fn = 5.0;
  }
}

CurveDiscretizer::CurveDiscretizer(std::function<std::optional<double>(const char *)> valueLookup)
{
  // These defaults were what the Python code was using.
  // Don't know why it differs from OpenSCAD language.
  fn = std::max(valueLookup("fn").value_or(0.0), 0.0);
  fe = std::max(valueLookup("fe").value_or(0.0), 0.0);
  fa = std::max(valueLookup("fa").value_or(12.0), F_MINIMUM);
  fs = std::max(valueLookup("fs").value_or(2.0), F_MINIMUM);
}

double segments_given_fa(double r, double fa)
{
  return 360.0 / fa;
}

double segments_given_fs(double r, double fs)
{
  return r * 2 * M_PI / fs;
}

/*!
   Returns the number of subdivision of a whole circle, given radius and
   the three special variables $fn, $fs and $fa
 */
std::optional<int> CurveDiscretizer::getCircularSegmentCount(double r, double angle_degrees) const
{
  // FIXME: It would be better to refuse to create an object. Let's do more strict error handling
  // in future versions of OpenSCAD
  if (r < GRID_FINE || std::isinf(fn) || std::isnan(fn) || std::isinf(angle_degrees) ||
      std::isnan(angle_degrees))
    return {};

  double result;
  if (fn > 0.0) {
    // If $fn is supplied, the other parameters are not used.
    // We continue to separately call `ceil()` before angle calculations to preserve backward
    // compatibility
    result = std::ceil(fn >= 3 ? fn : 3) * std::fabs(angle_degrees) / 360.0;
  } else {
    if (std::isinf(fe) || std::isnan(fe)) return {};
    // $fe measures "the most allowed error from the platonic circle to the discretized circle".
    // Which is the distance along a radius from the circumference to the midpoint of an edge
    // of the inscribed regular polygon we create when discretizing a circle.
    // $fe respects the minimums for $fa and $fs, but ignores their dynamic values.
    // aka specifying $fe means $fa/$fs are not used.
    if (fe >= GRID_FINE) {
      double max_segments =
        std::max(std::min(segments_given_fa(r, F_MINIMUM), segments_given_fs(r, F_MINIMUM)), 5.0);

      // Apothem = line from the center to the midpoint of an inscribed regular polygon
      // Apothem = `r-fe`
      // Circumradius (r) = apothem * sec(Pi/n)
      // Which we can rework to r*cos(Pi/n) = r-fe
      // then invcos(1-(fe/r)) = Pi/n
      // then n = Pi/invcos(1-(fe/r))
      // Which looks like the kind of formula you need to sanitize your inputs for.

      double ratio = fe / r;

      // We want 5 to be our minimum number of segments, so we can combine the
      // min segments and if ratio >= 1 into one check for the value of
      // ratio which creates n==5.0000:
      if (ratio >= 0.1909830056) {
        result = 5.0;
      } else {
        result = M_PI / std::acos(1 - ratio);
        // NaN is given for domain error for acos, but our input must be between 0 and 1
        // because we checked fe>0 and r>0, NaN-ness, and their ratio<1.
        // So we do not need to check for NaN.
        result = std::min(max_segments, result);
        // result gets ceil() applied to it below, so we don't need to do it here.
      }
    } else {
      result = std::ceil(std::max(std::min(segments_given_fa(r, fa), segments_given_fs(r, fs)), 5.0));
    }
    result *= std::fabs(angle_degrees) / 360.0;
  }
  return std::max(1, static_cast<int>(std::ceil(result)));
}


std::optional<int> CurveDiscretizer::getCircularSegmentCountAlt(double r, double angle_degrees) const
{
  // getCircularSegmentCount creates absolutely too less segments.
  // My understanding is that fn, fa, and fs requirement needs to me met at the same time
  // lets add an extra function to keep all their tests clean

  if (r < GRID_FINE || std::isinf(fn) || std::isnan(fn) || std::isinf(angle_degrees) ||
      std::isnan(angle_degrees))
    return {};

  double  result =  std::max(fn, std::max(360.0 / fa, r * 2 * M_PI*angle_degrees/360.0 / fs));
  return std::max(1, static_cast<int>(std::ceil(result)));
}

/*
   https://mathworld.wolfram.com/Helix.html
   For a helix defined as:         F(t) = [r*cost(t), r*sin(t), c*t]  for t in [0,T)
   The helical arc length is          L = T * sqrt(r^2 + c^2)
   Where its pitch is             pitch = 2*PI*c
   Pitch is also height per turn: pitch = height / (twist/360)
   Solving for c gives                c = height / (twist*PI/180)
   Where (twist*PI/180) is just twist in radians, aka "T"
 */
static double helix_arc_length(double r_sqr, double height, double twist_degrees)
{
  const double T = twist_degrees * M_DEG2RAD;
  const double c = height / T;
  return T * sqrt(r_sqr + c * c);
}

inline int helix_slices_given_fe(double fe, double r_sqr, double twist_degrees, int min_slices)
{
  // What does $fe mean in this context?
  // The return value will be the number of pieces stacked vertically on each other.
  // Say we're taking a centered square, twist=180, h=10.
  // If we had ten slices, each slice would be 1 unit tall, and its top vertices would be rotated 18
  // degrees from the bottom vertices. That also means there's a square of 4 vertices at z=0, 1, 2...10
  // The stacked vertices are always precisely on the arc, but the line between them is a straight edge.
  // So we could calculate the error from that straight edge to the helix arc.
  // The larger the radius of the furthest point, the larger that error will be.
  // This function could be changed, but all it currently knows about the vertices is the distance of the
  // furthest vertex from the axis of rotation. No edge could stick out further than that vertex, because
  // then it would need to terminate in a vertex, which would instead be the furthest vertex. So it seems
  // reasonable that edges between this vertex in each level
  //  will have more error from the helix path than any other point you could pick on the shape.
  // So we should have all the information we need to make the best decision.

  // First, bounds checking. The domain for our equations requires fe<r.
  const double r = sqrt(r_sqr);
  if (fe >= r) return min_slices;

  // Again we'll pretend we already know the number of slices, n.
  // For one step:
  // * Rotation around the central axis will be θ=twist_degrees/n degrees, no more than 120.
  // * dz=height/n
  // * Assuming x0=r and y0=0, then x1=r*cos(θ), y1=r*sin(θ)
  // The point with the largest error will be at the midpoint of the line segment from
  // (x0,0,0)->(x1,y1,dz) And we know the true arc point by substituting in θ/2 using the same equations
  // we used for x1&y1. Midpoint (mp) will be ((x0-x1)/2 + x1, y1/2, dz/2)
  //                   aka (x0/2+x1/2, y1/2, dz/2)
  // True arc midpoint (am) is (r*cos(θ/2), r*sin(θ/2), dz/2)
  // $fe will then be the distance between those two points.

  // Initially I approached this as a difference of two vectors symbolically, which is very complicated:
  // Of course, now we must work backward to make `n` the independent variable.
  // fe = (am-mp).norm() = sqrt((am.x-mp.x)^2+(am.y-mp.y)^2+(am.z-mp.z)^2)
  //   am.x-mp.x = r*cos(θ/2)-(r/2+r*cos(θ)/2)
  //   am.x-mp.x = r*(cos(θ/2)-(1+cos(θ))/2)
  //   am.y-mp.y = r*sin(θ/2) - r/2*sin(θ)
  //   am.y-mp.y = r*(sin(θ/2) - sin(θ)/2)
  //   am.z-mp.z = dz/2 - dz/2 = 0
  // fe = sqrt((r*(cos(θ/2)-(1+cos(θ))/2))^2 + (r*(sin(θ/2) - sin(θ)/2))^2)
  // The next steps to solve for θ are a bit complicated.
  // The following was used in Maxima; some of it may be redundant and many dead-ends excised:
  // assume(θ > 0, θ <= %pi/2);
  // eq: fe = sqrt( (r*(cos(θ/2) - (1+cos(θ))/2))^2 + (r*(sin(θ/2) - sin(θ)/2))^2 )
  // eq_sq: fe^2 = rhs(eq)^2;
  // eq_reduce: trigreduce(eq_sq);
  // eq_simp: fe^2 = trigsimp(rhs(eq_reduce));
  // expand(eq_simp);
  // eq_simp2: subst(2*cos(θ/2)^2-1, cos(θ), eq_simp);
  // eq_simp3: subst(c,cos(θ/2),eq_simp2);
  // eq_simp4: expand(eq_simp3);
  // s1: solve(eq_simp4, c);
  // eq2: subst(cos(θ/2),c,s1);
  // s2_1: solve(eq2[1], θ);
  // Which finally gives θ=2*%pi-2*acos(fe/r-1)

  // Easier is to measure the distance from the origin at height dz/2.
  // The arc distance is always r.
  // The midpoint distance is sqrt(mp.x^2+mp.y^2). Switching to Maxima syntax:
  // mp_d: sqrt((r/2+r*cos(θ)/2)^2+(r*sin(θ)/2)^2);
  // mp_dr: radcan(mp_d);
  // mp_ds: trigsimp(mp_dr);
  // mp_dr: (sqrt(sin(θ)^2+cos(θ)^2+2*cos(θ)+1)*r)/2
  // mp_ds: (sqrt(2*cos(θ)+2)*r)/2
  // err1: r-mp_d;
  // assume(r>0);
  // s3: solve(fe=err1,r);
  // s4: trigsimp(s3[1]);
  // s5: solve(s4,θ);
  // s5_s: trigsimp(s5[1]);
  // Which yields θ=acos((r^2-4*fe*r+2*fe^2)/r^2)
  // Fortunately that graphs to the same thing as θ=2(pi-acos(fe/r-1)) over the domain of interest (r>fe,
  // 0<theta<120deg). So use the simpler equation.

  double theta = 2 * (M_PI - acos(fe / r - 1));
  int fe_slices = static_cast<int>(std::ceil(twist_degrees * M_DEG2RAD / theta));
  // min_slices already handles the requirement to not exceed 120 degrees per step.
  return std::max(fe_slices, min_slices);
}

std::optional<int> CurveDiscretizer::getHelixSlices(double r_sqr, double height,
                                                    double twist_degrees) const
{
  twist_degrees = std::fabs(twist_degrees);
  // 180 twist per slice is worst case, guaranteed non-manifold.
  // Make sure we have at least 3 slices per 360 twist
  const int min_slices = std::max(static_cast<int>(std::ceil(twist_degrees / 120.0)), 1);
  if (sqrt(r_sqr) < GRID_FINE || std::isinf(fn) || std::isnan(fn) || std::isnan(height) ||
      std::isnan(twist_degrees))
    return {};
  if (fn > 0.0) {
    const int fn_slices = static_cast<int>(std::ceil(twist_degrees / 360.0 * fn));
    return std::max(fn_slices, min_slices);
  }
  if (fe > 0.0) {
    return helix_slices_given_fe(fe, r_sqr, twist_degrees, min_slices);
  }

  const int fa_slices = static_cast<int>(std::ceil(twist_degrees / fa));
  const int fs_slices = static_cast<int>(std::ceil(helix_arc_length(r_sqr, height, twist_degrees) / fs));
  return std::max(std::min(fa_slices, fs_slices), min_slices);
}

/*
   For linear_extrude with twist and uniform scale (scale_x == scale_y),
   to calculate the limit imposed by special variable $fs, we find the
   total length along the path that a vertex would follow.
   The XY-projection of this path is a section of the Archimedes Spiral.
   https://mathworld.wolfram.com/ArchimedesSpiral.html
   Using the formula for its arc length, then pythagorean theorem with height
   should tell us the total distance a vertex covers.
 */
static double archimedes_length(double a, double theta)
{
  return 0.5 * a * (theta * sqrt(1 + theta * theta) + asinh(theta));
}

std::optional<int> CurveDiscretizer::getConicalHelixSlices(double r_sqr, double height,
                                                           double twist_degrees, double scale) const
{
  twist_degrees = fabs(twist_degrees);
  const double r = sqrt(r_sqr);
  const int min_slices = std::max(static_cast<int>(ceil(twist_degrees / 120.0)), 1);
  if (r < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return {};
  if (fn > 0.0) {
    const int fn_slices = static_cast<int>(ceil(twist_degrees * fn / 360));
    return std::max(fn_slices, min_slices);
  }
  /*
     Spiral length equation assumes starting from theta=0
     Our twist+scale only covers a section of this length (unless scale=0).
     Find the start and end angles that our twist+scale correspond to.
     Use similar triangles to visualize cross-section of single vertex,
     with scale extended to 0 (origin).

     (scale < 1)        (scale > 1)
                        ______t_  1.5x (Z=h)
     0x                 |    | /
   |\                  |____|/
   | \                 |    / 1x  (Z=0)
   |  \                |   /
   |___\ 0.66x (Z=h)   |  /            t is angle of our arc section (twist, in rads)
   |   |\              | /             E is angle_end (total triangle base length)
   |___|_\  1x (Z=0)   |/ 0x           S is angle_start
         t

     E = t*1/(1-0.66)=3t E = t*1.5/(1.5-1)  = 3t
     B = E - t            B = E - t
   */
  const double rads = twist_degrees * M_DEG2RAD;
  double angle_end = 0;
  if (scale > 1) {
    angle_end = rads * scale / (scale - 1);
  } else if (scale < 1) {
    angle_end = rads / (1 - scale);
  } else {
    assert(false && "Don't calculate conical slices on non-scaled extrude!");
  }
  const double angle_start = angle_end - rads;
  const double a = r / angle_end;  // spiral scale coefficient
  const double spiral_length = archimedes_length(a, angle_end) - archimedes_length(a, angle_start);
  // Treat (flat spiral_length,extrusion height) as (base,height) of a right triangle to get diagonal
  // length.
  const double total_length = sqrt(spiral_length * spiral_length + height * height);

  const int fs_slices = static_cast<int>(ceil(total_length / fs));
  const int fa_slices = static_cast<int>(ceil(twist_degrees / fa));
  return std::max(std::min(fa_slices, fs_slices), min_slices);
}

std::optional<int> CurveDiscretizer::getDiagonalSlices(double delta_sqr, double height) const
{
  constexpr int min_slices = 1;
  if (sqrt(delta_sqr) < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return {};
  if (fn > 0.0) {
    const int fn_slices = static_cast<int>(fn);
    return std::max(fn_slices, min_slices);
  }
  const int fs_slices = static_cast<int>(ceil(sqrt(delta_sqr + height * height) / fs));
  return std::max(fs_slices, min_slices);
}

// Insert vertices for segments interpolated between v0 and v1.
// The last vertex (t==1) is not added here to avoid duplicate vertices,
// since it will be the first vertex of the *next* edge.
void add_segmented_edge(Outline2d& o, const Vector2d& v0, const Vector2d& v1, unsigned int edge_segments)
{
  for (unsigned int j = 0; j < edge_segments; ++j) {
    double t = static_cast<double>(j) / edge_segments;
    o.vertices.push_back((1 - t) * v0 + t * v1);
  }
}

// While total outline segments < fn, increment segment_count for edge with largest
// (max_edge_length / segment_count).
Outline2d splitOutlineByFn(const Outline2d& o, const double twist, const double scale_x,
                           const double scale_y, const double fn, unsigned int slices)
{
  struct segment_tracker {
    size_t edge_index;
    double max_edgelen;
    unsigned int segment_count{1u};
    segment_tracker(size_t i, double len) : edge_index(i), max_edgelen(len) {}
    // metric for comparison: average between (max segment length, and max segment length after split)
    [[nodiscard]] double metric() const { return max_edgelen / (segment_count + 0.5); }
    bool operator<(const segment_tracker& rhs) const { return this->metric() < rhs.metric(); }
    [[nodiscard]] bool close_match(const segment_tracker& other) const
    {
      // Edges are grouped when metrics match by at least 99.9%
      constexpr double APPROX_EQ_RATIO = 0.999;
      double l1 = this->metric(), l2 = other.metric();
      return std::min(l1, l2) / std::max(l1, l2) >= APPROX_EQ_RATIO;
    }
  };

  const auto num_vertices = o.vertices.size();
  std::vector<unsigned int> segment_counts(num_vertices, 1);
  std::priority_queue<segment_tracker, std::vector<segment_tracker>> q;

  Vector2d v0 = o.vertices[0];
  // non-uniform scaling requires iterating over each slice transform
  // to find maximum length of a given edge.
  if (scale_x != scale_y) {
    for (size_t i = 1; i <= num_vertices; ++i) {
      Vector2d v1 = o.vertices[i % num_vertices];
      double max_edgelen = 0.0;  // max length for single edge over all transformed slices
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
  } else {  // uniform scaling
    double max_scale = std::max(scale_x, 1.0);
    for (size_t i = 1; i <= num_vertices; ++i) {
      Vector2d v1 = o.vertices[i % num_vertices];
      double max_edgelen = (v1 - v0).norm() * max_scale;
      q.emplace(i - 1, max_edgelen);
      v0 = v1;
    }
  }

  std::vector<segment_tracker> tmp_q;
  // Process priority_queue until number of segments is reached.
  size_t seg_total = num_vertices;
  while (seg_total < fn) {
    auto current = q.top();

    // Group similar length segmented edges to keep result roughly symmetrical.
    while (!q.empty() && (tmp_q.empty() || q.top().close_match(tmp_q.front()))) {
      tmp_q.push_back(q.top());
      q.pop();
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
  for (size_t i = 1; i <= num_vertices; ++i) {
    Vector2d v1 = o.vertices[i % num_vertices];
    add_segmented_edge(o2, v0, v1, segment_counts[i - 1]);
    v0 = v1;
  }

  assert(o2.vertices.size() <= fn);
  return o2;
}

// For each edge in original outline, find its max length over all slice transforms,
// and divide into segments no longer than fs.
Outline2d splitOutlineByFs(const Outline2d& o, const double twist, const double scale_x,
                           const double scale_y, const double fs, unsigned int slices)
{
  const auto num_vertices = o.vertices.size();

  Vector2d v0 = o.vertices[0];
  Outline2d o2;
  o2.positive = o.positive;

  // non-uniform scaling requires iterating over each slice transform
  // to find maximum length of a given edge.
  if (scale_x != scale_y) {
    for (size_t i = 1; i <= num_vertices; ++i) {
      Vector2d v1 = o.vertices[i % num_vertices];
      double max_edgelen = 0.0;  // max length for single edge over all transformed slices
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
  } else {  // uniform scaling
    double max_scale = std::max(scale_x, 1.0);
    for (size_t i = 1; i <= num_vertices; ++i) {
      Vector2d v1 = o.vertices[i % num_vertices];
      unsigned int edge_segments =
        static_cast<unsigned int>(std::ceil((v1 - v0).norm() * max_scale / fs));
      add_segmented_edge(o2, v0, v1, edge_segments);
      v0 = v1;
    }
  }
  return o2;
}

Outline2d CurveDiscretizer::splitOutline(const Outline2d& o, double twist, double scale_x,
                                         double scale_y, unsigned int slices,
                                         unsigned int segments) const
{
  if (segments > 0 || fn > 0.0) {
    unsigned int min_vertices = segments > 0 ? segments : static_cast<unsigned int>(std::max(fn, 3.0));
    if (o.vertices.size() >= min_vertices) {
      return o;
    } else {
      return splitOutlineByFn(o, twist, scale_x, scale_y, min_vertices, slices);
    }
  }
  // $fs and $fa based segmentation
  auto fa_segs = static_cast<unsigned int>(std::ceil(360.0 / fa));
  if (o.vertices.size() >= fa_segs) {
    return o;
  } else {
    // try splitting by $fs, then check if $fa results in less segments
    auto fsOutline = splitOutlineByFs(o, twist, scale_x, scale_y, fs, slices);
    if (fsOutline.vertices.size() >= fa_segs) {
      return splitOutlineByFn(o, twist, scale_x, scale_y, fa_segs, slices);
    } else {
      return std::move(fsOutline);
    }
  }
}

std::ostream& operator<<(std::ostream& stream, const CurveDiscretizer& f)
{
  if (Feature::ExperimentalDiscretizationByError.is_enabled()) {
    stream << "$fn = " << f.fn << ", $fe = " << f.fe << ", $fa = " << f.fa << ", $fs = " << f.fs;
  } else {
    stream << "$fn = " << f.fn << ", $fa = " << f.fa << ", $fs = " << f.fs;
  }
  return stream;
}

RoofDiscretizer::RoofDiscretizer(const CurveDiscretizer& d, double scale)
  : max_angle_deviation(M_PI / 180.0 * (d.fn > 0.0 ? 360.0 / d.fn : d.fa) / 2.0),
    max_segment_sqr_length(d.fn > 0.0 ? 0.0 : d.fs * d.fs * scale * scale)
{
}

bool RoofDiscretizer::overMaxAngle(double radians) const
{
  return radians > max_angle_deviation;
}

bool RoofDiscretizer::overMaxSegmentSqrLength(double segment_sqr_length) const
{
  return segment_sqr_length > max_segment_sqr_length;
}
