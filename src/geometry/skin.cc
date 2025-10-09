#include "geometry/skin.h"

#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/Polygon2d.h"

/* Returns whether travel from p0 => p1 is a negative, zero, or positive distance
 * in the direction of the extusion, with respect to p0's plane.
 */
inline int check_extrusion_progression(const Vector3d& p0, const Vector3d& p1, const Vector3d& plane_abc,
                                       double plane_d, double equality_tolerance)
{
  // point is clearly above the plane?
  double plane_dist = plane_abc.dot(p1) + plane_d;
  if (plane_dist > equality_tolerance) return 1;
  // point lies on the plane, and is same as previous point?
  else if (plane_dist > -equality_tolerance && (p1 - p0).squaredNorm() < equality_tolerance) return 0;
  // point crossed the plane, or lies on the plane and isn't the same point.
  else return -1;
}

static std::unique_ptr<PolySet> expand_poly2d_to_ccw3d(std::shared_ptr<const Polygon2d> const& poly2d,
                                                       unsigned int convexity)
{
  PolySetBuilder builder;
  builder.setConvexity(convexity);

  // unpack all the 2D coordinates into 3D vectors with Z=0
  for (const auto& outline : poly2d->untransformedOutlines()) {
    builder.beginPolygon(outline.vertices.size());
    for (const auto& vtx : outline.vertices) builder.addVertex(Vector3d(vtx[0], vtx[1], 0));
    // Make sure winding order is CCW
    // if (polyset->indices.back().size() > 2) {
    //  Vector3d ab = polyset->indices.back()[1] - polyset->indices.back()[0];
    //  Vector3d bc = polyset->indices.back()[2] - polyset->indices.back()[1];
    //  if (ab.cross(bc).z() < 0) {
    //    // Reverse the winding
    //    std::reverse(polyset->indices.back().begin(), polyset->indices.back().end());
    //  }
    //}
  }
  std::unique_ptr<PolySet> res = builder.build();
  res->transform(poly2d->getTransform3d());
  return res;
}

// Check for no null slices
static bool sanityCheckNoNullSlices(const SkinNode& node,
                                    std::vector<std::shared_ptr<const Polygon2d>> const& slices,
                                    const Location& loc, std::string const& docpath)
{
  for (size_t i = 0; i != slices.size(); ++i) {
    if (slices[i] == nullptr) {
      LOG(message_group::Error, loc, docpath, "%1$s has a null slice at index %2$d", node.name(), i);
      return false;
    }
  }
  return true;
}

// Check for matching contours
static bool sanityCheckContours(const SkinNode& node,
                                std::vector<std::shared_ptr<const Polygon2d>> const& slices,
                                const Location& loc, std::string const& docpath)
{
  for (size_t i = 1; i < slices.size(); i++) {
    bool match = slices[i]->untransformedOutlines().size() == slices[0]->untransformedOutlines().size();
    if (!match) {
      LOG(message_group::Error, loc, docpath,
          "Each extrusion slice must have exactly the same number of contours\n");
      // Collect details to help debug
      LOG(message_group::Error, loc, docpath, " slice   0 - %1$2d outlines",
          slices[0]->untransformedOutlines().size());
      LOG(message_group::Error, loc, docpath, " slice %1$3d - %2$2d outlines", i,
          slices[i]->untransformedOutlines().size());
      return false;
    }
  }
  return true;
}

// Check for matching contours and vertices
static bool sanityCheckContoursAndVertices(const SkinNode& node,
                                           std::vector<std::shared_ptr<const Polygon2d>> const& slices,
                                           const Location& loc, std::string const& docpath)
{
  for (size_t i = 1; i < slices.size(); i++) {
    bool match = slices[i]->untransformedOutlines().size() == slices[0]->untransformedOutlines().size();
    for (size_t p = 0; match && p < slices[i]->untransformedOutlines().size(); p++)
      match = slices[i]->untransformedOutlines()[p].vertices.size() ==
              slices[0]->untransformedOutlines()[p].vertices.size();
    if (!match) {
      LOG(message_group::Error, loc, docpath,
          "Each extrusion slice must have exactly the same vertex count,\n"
          "(note that polygon sanitization may remove duplicate vertices or co-linear points)");
      // Collect details to help debug
      std::stringstream desc_0, desc_i;
      for (const auto& o : slices[0]->untransformedOutlines())
        desc_0 << " " << o.vertices.size() << "vtx";
      for (const auto& o : slices[i]->untransformedOutlines())
        desc_i << " " << o.vertices.size() << "vtx";
      LOG(message_group::Error, loc, docpath, " slice   0 - %1$2d outlines: %2$s",
          slices[0]->untransformedOutlines().size(), desc_0.str().c_str());
      LOG(message_group::Error, loc, docpath, " slice %1$3d - %2$2d outlines: %3$s", i,
          slices[i]->untransformedOutlines().size(), desc_i.str().c_str());
      return false;
    }
  }
  return true;
}

static void outputSingleQuad(PolySetBuilder& builder, Vector3d const& prev0, Vector3d const& prev1,
                             Vector3d const& cur0, Vector3d const& cur1)
{
  // Like with linear_interpolate, triangulate on the shorter
  double d1 = std::abs((prev0 - cur1).norm());
  double d2 = std::abs((prev1 - cur0).norm());
  bool splitfirst = (d1 >= d2) || (std::abs(d2 - d1) < 1e-4);

  if (splitfirst) {
    builder.beginPolygon(3);
    builder.addVertex(cur0);
    builder.addVertex(prev0);
    builder.addVertex(prev1);

    builder.beginPolygon(3);
    builder.addVertex(prev1);
    builder.addVertex(cur1);
    builder.addVertex(cur0);
  } else {
    builder.beginPolygon(3);
    builder.addVertex(cur1);
    builder.addVertex(cur0);
    builder.addVertex(prev0);

    builder.beginPolygon(3);
    builder.addVertex(prev0);
    builder.addVertex(prev1);
    builder.addVertex(cur1);
  }
}

// Build a quad with two triangles between each pair of adjacent vertices
static void outputQuad(PolySetBuilder& builder, Vector3d const& prev0, Vector3d const& prev1,
                       Vector3d const& cur0, Vector3d const& cur1, bool v0_progression, bool progression)
{
  if (v0_progression && progression) {
    outputSingleQuad(builder, prev0, prev1, cur0, cur1);
  } else {
    if (v0_progression) {
      builder.beginPolygon(3);
      builder.addVertex(cur0);
      builder.addVertex(prev0);
      builder.addVertex(prev1);
    }
    if (progression) {
      builder.beginPolygon(3);
      builder.addVertex(prev1);
      builder.addVertex(cur1);
      builder.addVertex(cur0);
    }
  }
}

struct AlignmentPoint {
  size_t vertex_index;
  Vector2d intersect_point;
  double distance_from_centre;
  double distance_round_polygon{-1};
};

static double fix_angle(double angle)
{
  angle = fmod(angle, 360);
  if (angle > 180) angle -= 360;
  if (angle < -180) angle += 360;
  return angle;
}

// A set of polygon vertices can start from any location in its 2d definition.
// e.g. for one polygon vertex 0 could be top right, another bottom left
// Find the further point where a line projected from the centre at a specified angle hits the poly and
// use that to select a vertex
static std::vector<std::vector<AlignmentPoint>> findAlignmentPoints(
  std::vector<std::shared_ptr<const Polygon2d>> const& slicesin, bool has_align_angle, int align_angle)
{
  std::vector<std::vector<AlignmentPoint>> alignmentPoints;
  alignmentPoints.resize(slicesin.size());

  align_angle = fix_angle(align_angle);

  int outlines_count = slicesin[0]->untransformedOutlines().size();
  for (auto& per_slice : alignmentPoints) per_slice.resize(outlines_count);

  for (int o_i = 0, o_end = outlines_count; o_i != o_end; ++o_i) {
    // Try to start from the further to one corner
    for (int s_i = 0, s_end = slicesin.size(); s_i != s_end; ++s_i) {
      auto const& slice = slicesin[s_i];
      auto const& vertices = slice->untransformedOutlines()[o_i].vertices;
      double xmin = std::numeric_limits<double>::infinity();
      double xmax = -std::numeric_limits<double>::infinity();
      double ymin = std::numeric_limits<double>::infinity();
      double ymax = -std::numeric_limits<double>::infinity();
      for (auto& vertex : vertices) {
        xmin = std::min(xmin, vertex[0]);
        xmax = std::max(xmax, vertex[0]);
        ymin = std::min(ymin, vertex[1]);
        ymax = std::max(ymax, vertex[1]);
      }
      Vector2d min_point(xmin, ymin);
      Vector2d max_point(xmax, ymax);
      Vector2d centre((max_point + min_point) / 2);

      Vector2d centre2d(centre[0], centre[1]);

      // Find the vertex that is furthest in align_angle direction in the outer contour
      // Start by computing the angle of each vertex
      std::vector<double> angles;
      for (auto const& vertex : vertices) {
        auto relative_vertex = vertex - centre2d;
        double angle = atan2(relative_vertex[1], relative_vertex[0]) / (M_PI * 2 / 360);
        angles.push_back(angle);
      }
      if (!has_align_angle) {
        align_angle = fix_angle(*angles.begin());
        has_align_angle = true;
      }

      // Then we only care about the pairs which straddle the desired angle
      AlignmentPoint point;
      point.vertex_index = 0;
      point.distance_from_centre = -1;
      point.intersect_point = Vector2d(0, 0);
      double prev_angle = *angles.rbegin();
      int v_prev_i = vertices.size() - 1;
      for (int v_i = 0, v_end = vertices.size(); v_i != v_end; ++v_i) {
        auto vc = vertices[v_i] - centre2d;
        auto vp = vertices[v_prev_i] - centre2d;
        auto centre2d_rebased = centre2d - centre2d;
        double angle = angles[v_i];

        double angle_delta = fix_angle(angle - prev_angle);

        if ((angle_delta >= 0 && (angle >= align_angle and prev_angle <= align_angle)) ||
            (angle_delta < 0 && (angle <= align_angle and prev_angle >= align_angle))) {
          auto line1 = Eigen::Hyperplane<double, 2>::Through(vc, vp);
          double align_angle_radians = double(align_angle) * 2.0 * M_PI / 360;
          double linelen = 1e6 * (max_point[0] - min_point[0]);
          Vector2d centre2dadj(centre2d_rebased[0] + linelen,
                               centre2d_rebased[1] + tan(align_angle_radians) * linelen);
          auto line2 = Eigen::Hyperplane<double, 2>::Through(centre2d_rebased, centre2dadj);

          auto intersect_point = line1.intersection(line2);

          // Distance
          double distance_from_centre = sqrt(pow(intersect_point[1] - centre2d_rebased[1], 2.0) +
                                             pow(intersect_point[0] - centre2d_rebased[0], 2.0));

          if (distance_from_centre > point.distance_from_centre) {
            point.distance_from_centre = distance_from_centre;
            point.intersect_point = intersect_point + centre2d;
            point.vertex_index = v_prev_i;
          }
        }
        v_prev_i = v_i;
        prev_angle = angle;
      }

      alignmentPoints[s_i][o_i] = std::move(point);
    }
  }

  return alignmentPoints;
}

// Make sure each slice has the same number of vertices in the same relative location
// Alignment points will be corrected
static std::vector<std::shared_ptr<const Polygon2d>> interpolateVertices(
  std::vector<std::shared_ptr<const Polygon2d>> const& slicesin,
  std::vector<std::vector<AlignmentPoint>>& alignmentPoints)
{
  std::vector<std::shared_ptr<Polygon2d>> slicesadj;
  for (size_t i = 0; i < slicesin.size(); i++) slicesadj.push_back(std::make_shared<Polygon2d>());

  // Calculate the distance round each contour and the fraction of each edge
  // Also adds a new vertex at the alignmentPoint
  int outlines_count = slicesin[0]->untransformedOutlines().size();
  for (size_t o_i = 0, o_end = outlines_count; o_i != o_end; ++o_i) {
    std::set<double> all_distance_fractions;
    std::vector<double> slice_distances;
    double max_total_distance = 0;
    for (size_t sl_i = 0, sl_end = slicesin.size(); sl_i != sl_end; ++sl_i) {
      auto& alignmentPoint = alignmentPoints[sl_i][o_i];

      auto& vertices = slicesin[sl_i]->untransformedOutlines()[o_i].vertices;
      double total_distance = 0;
      std::vector<double> dist;
      dist.push_back(0);
      for (size_t vl_i = 0, vl_end = vertices.size(); vl_i != vl_end; ++vl_i) {
        bool last = (vl_i + 1) == vertices.size();
        size_t vl_next_i = last ? 0 : (vl_i + 1);
        auto diff = vertices[vl_next_i] - vertices[vl_i];

        double distance = sqrt(pow(diff[0], 2) + pow(diff[1], 2));

        if (vl_i == alignmentPoint.vertex_index) {
          auto diff_align = alignmentPoint.intersect_point - vertices[vl_i];
          double distance_align = sqrt(pow(diff_align[0], 2) + pow(diff_align[1], 2));
          alignmentPoint.distance_round_polygon = total_distance + distance_align;
          dist.push_back(alignmentPoint.distance_round_polygon);
        }

        total_distance += distance;
        if (!last) dist.push_back(total_distance);
      }
      slice_distances.push_back(total_distance);
      max_total_distance = std::max(max_total_distance, total_distance);
      for (double distance_fraction : dist) {
        distance_fraction /= total_distance;
        all_distance_fractions.insert(distance_fraction);
      }
    }

    // Simplify fractions
    std::set<double> all_distance_fractions_unadjusted = std::move(all_distance_fractions);
    double distance_fraction_prev = *all_distance_fractions_unadjusted.rbegin() - 1;
    for (double distance_fraction : all_distance_fractions_unadjusted) {
      double change = distance_fraction - distance_fraction_prev;
      change *= max_total_distance;
      if (change > 1e-4) {
        all_distance_fractions.insert(distance_fraction);
        distance_fraction_prev = distance_fraction;
      }
    }

    // Rewrite the contours interpolating with all_distance_fractions
    for (int sl_i = 0, sl_end = slicesin.size(); sl_i != sl_end; ++sl_i) {
      Polygon2d const& polyin = *slicesin[sl_i];
      Polygon2d& polyadj = *slicesadj[sl_i];

      auto& alignmentPoint = alignmentPoints[sl_i][o_i];

      auto& vertices = polyin.untransformedOutlines()[o_i].vertices;

      Outline2d outlineadj;

      int vl_i = 0;
      int vl_next_i = 1;
      double distance = 0;
      auto diff = vertices[vl_next_i] - vertices[vl_i];
      double distance_next = sqrt(pow(diff[0], 2) + pow(diff[1], 2));
      alignmentPoint.vertex_index = -1;
      double alignment_point_distance = std::numeric_limits<double>::infinity();

      for (double distance_fraction : all_distance_fractions) {
        double vertex_distance = distance_fraction * slice_distances[sl_i];
        if (vertex_distance > distance_next) {
          // Next point
          vl_i++;
          vl_next_i++;
          if (vl_next_i == vertices.size()) vl_next_i = 0;
          auto diff = vertices[vl_next_i] - vertices[vl_i];
          distance = distance_next;
          distance_next += sqrt(pow(diff[0], 2) + pow(diff[1], 2));
        }

        auto const& v0 = vertices[vl_i];
        auto const& v1 = vertices[vl_next_i];
        auto v0_adj = v0 + ((vertex_distance - distance) * (v1 - v0)) / (distance_next - distance);
        outlineadj.vertices.push_back(v0_adj);

        if (std::abs(vertex_distance - alignmentPoint.distance_round_polygon) <
            alignment_point_distance) {
          alignmentPoint.vertex_index = outlineadj.vertices.size() - 1;
          alignment_point_distance = std::abs(vertex_distance - alignmentPoint.distance_round_polygon);
        }
      }
      polyadj.addOutline(std::move(outlineadj));
    }
  }

  for (int sl_i = 0, sl_end = slicesin.size(); sl_i != sl_end; ++sl_i) {
    Polygon2d const& polyin = *slicesin[sl_i];
    auto polyadj = slicesadj[sl_i];
    polyadj->transform3d(polyin.getTransform3d());
  }

  std::vector<std::shared_ptr<const Polygon2d>> slicesadjconst;  // TODO: why is this not possible?
  for (auto const& slice : slicesadj) slicesadjconst.push_back(slice);
  return slicesadjconst;
}

// Make the first vertex the alignmentpoint
static std::vector<std::shared_ptr<const Polygon2d>> spinPolygons(
  std::vector<std::shared_ptr<const Polygon2d>> const& slicesin,
  std::vector<std::vector<AlignmentPoint>>& alignmentPoints)
{
  std::vector<std::shared_ptr<Polygon2d>> slicesadj;
  for (auto const& slice : slicesin) slicesadj.push_back(std::make_shared<Polygon2d>());

  int outlines_count = slicesin[0]->untransformedOutlines().size();
  for (int o_i = 0, o_end = outlines_count; o_i != o_end; ++o_i) {
    for (int sl_i = 0, sl_end = slicesin.size(); sl_i != sl_end; ++sl_i) {
      Polygon2d const& polyin = *slicesin[sl_i];
      Polygon2d& polyadj = *slicesadj[sl_i];

      auto const& alignmentPoint = alignmentPoints[sl_i][o_i];

      auto const& vertices = polyin.untransformedOutlines()[o_i].vertices;
      Outline2d outlineadj;

      for (int vl_i = 0, vl_end = vertices.size(); vl_i != vl_end; ++vl_i) {
        int vl_adj = vl_i + alignmentPoint.vertex_index;
        outlineadj.vertices.push_back(vertices[vl_adj % vertices.size()]);
      }

      polyadj.addOutline(std::move(outlineadj));
    }
  }

  for (int sl_i = 0, sl_end = slicesin.size(); sl_i != sl_end; ++sl_i) {
    Polygon2d const& polyin = *slicesin[sl_i];
    auto polyadj = slicesadj[sl_i];
    polyadj->transform3d(polyin.getTransform3d());
  }

  std::vector<std::shared_ptr<const Polygon2d>> slicesadjconst;  // TODO: why is this not possible?
  for (auto const& slice : slicesadj) slicesadjconst.push_back(slice);
  return slicesadjconst;
}

// When there is not very planar it can be modelled better with more segments, allow this as an option
// Done here rather than in outputSingleQuad since that would lead to T junctions
static std::vector<std::shared_ptr<const Polygon2d>> segmentVertices(
  std::vector<std::shared_ptr<const Polygon2d>> const& slicesin, bool has_segments,
  unsigned int segments)
{
  if (has_segments && segments > 0) {
    double sides = 0;
    for (auto const& outlinein : slicesin[0]->untransformedOutlines())
      sides += outlinein.vertices.size();
    unsigned int segments_per_side = std::ceil(double(segments) / sides);

    std::vector<std::shared_ptr<const Polygon2d>> slicesadj;
    for (auto const& slice : slicesin) {
      Polygon2d const& polyin = *slice;
      auto polyadj = std::make_shared<Polygon2d>();
      std::vector<Outline2d> const& outlinesin = polyin.untransformedOutlines();
      for (auto const& outlinein : outlinesin) {
        Outline2d outlineadj;
        for (size_t i = 1; i <= outlinein.vertices.size(); ++i) {
          auto const& v0 = outlinein.vertices[i - 1];
          auto const& v1 = outlinein.vertices[i != outlinein.vertices.size() ? i : 0];
          for (size_t i = 0; i != segments_per_side; ++i) {
            auto v0_adj = v0 + (i * (v1 - v0)) / segments_per_side;
            outlineadj.vertices.push_back(v0_adj);
          }
        }

        polyadj->addOutline(std::move(outlineadj));
      }
      polyadj->transform3d(polyin.getTransform3d());
      slicesadj.push_back(polyadj);
    }
    return slicesadj;
  }
  return slicesin;
}

// Debug support
#if 0
static void dumpPolygons(std::string where, std::vector<std::shared_ptr<const Polygon2d>> slicesin)
{
  int sl_i = 0;
  for (auto const& slice : slicesin) {
    Polygon2d const& polygon = *slice;
    auto const& outlines = polygon.untransformedOutlines();
    int pth_i = 0;
    std::vector<int> path_lens;
    std::stringstream points;
    std::stringstream paths;
    bool first = true;
    for (auto const& outline : outlines) {
      std::stringstream path;
      for (auto const& vertex : outline.vertices) {
        points << ", [" << vertex[0] << "," << vertex[1] << "]";
        path << ", " << pth_i++;
      }
      path << "]";
      if (!first) paths << ", ";
      first = false;
      paths << "[" << path.str().substr(1);
      path_lens.push_back(pth_i);
    }
    std::stringstream polygonstr;
    polygonstr << "polygon(points=[" << points.str().substr(1) << "] , paths=[" << paths.str() << "]);";
    std::cerr << "# Slice " << sl_i << " of " << where;
    std::cerr << " has " << path_lens.size() << " contours with lengths [";
    for (int p : path_lens) {
      std::cerr << p << ",";
    }
    std::cerr << "]" << std::endl;
    std::cerr << polygonstr.str() << std::endl;
    sl_i++;
  }
}

static void dumpAlignmentPoints(std::vector<std::vector<AlignmentPoint>> const& alignmentPoints,
                                int align_angle)
{
  std::cerr << "Aligned to angle: " << align_angle << std::endl;
  for (int sl_i = 0, sl_end = alignmentPoints.size(); sl_i != sl_end; ++sl_i) {
    auto const& per_slice = alignmentPoints[sl_i];
    for (int ol_i = 0, ol_end = per_slice.size(); ol_i != ol_end; ++ol_i) {
      auto const& alignmentPoint = per_slice[ol_i];
      std::cerr << "Slice " << sl_i << " outline " << ol_i << " on vertex "
                << alignmentPoint.vertex_index << " point " << alignmentPoint.intersect_point[0] << ","
                << alignmentPoint.intersect_point[1] << " distance from centre "
                << alignmentPoint.distance_from_centre << std::endl;
    }
  }
}

#endif
/*!
  input: List of 2D objects arranged in 3D, each with identical outline count and vertex count
  output: 3D PolySet
 */
std::shared_ptr<const Geometry> skinPolygonSequence(
  const SkinNode& node, std::vector<std::shared_ptr<const Polygon2d>> slicesin, const Location& loc,
  std::string const& docpath)
{
  size_t i, p, v;
  const double CLOSE_ENOUGH = 0.00000000000000001;  // tolerance for identical coordinates

  // Verify there is something to work with
  if (slicesin.size() < 2) {
    LOG(message_group::Error, loc, docpath, "%1$s requires at least two slices", node.name());
    return nullptr;
  }

  // Check for no null slices
  if (!sanityCheckNoNullSlices(node, slicesin, loc, docpath)) return nullptr;

  // Verify that every slice has the same number of contours with the same number of vertices
  if (!sanityCheckContours(node, slicesin, loc, docpath)) return nullptr;

  // If contours match but number of vertices differs, attempt to align
  // dumpPolygons("input",slicesin);
  auto alignmentPoints = findAlignmentPoints(slicesin, node.has_align_angle, node.align_angle);
  // dumpAlignmentPoints(alignmentPoints,node.align_angle);
  if (node.interpolate) slicesin = interpolateVertices(slicesin, alignmentPoints);
  // dumpPolygons("interpolate",slicesin);
  slicesin = spinPolygons(slicesin, alignmentPoints);
  // dumpPolygons("spun",slicesin);

  // Verify that every slice has the same number of contours with the same number of vertices
  if (!sanityCheckContoursAndVertices(node, slicesin, loc, docpath)) return nullptr;

  // Add more vertices to slices, to segment more
  auto slices = segmentVertices(slicesin, node.has_segments, node.segments);
  // dumpPolygons("segments",slices);

  // Start extruding slices.  Come back to "end caps" at the end.
  int reversed = 0;
  std::unique_ptr<PolySet> tmp0, tmp1, tmp2;
  PolySetBuilder result;
  result.setConvexity(node.convexity);

  // Unroll first iteration so we have a "prev" to work with, and so we can use it again at the end
  tmp0 = expand_poly2d_to_ccw3d(slices[0], node.convexity);

  std::unique_ptr<PolySet> *cur = &tmp1, *prev = &tmp0;
  int progression = 0;
  for (i = 1; i < slices.size(); i++, prev = cur, cur = (cur == &tmp1 ? &tmp2 : &tmp1)) {
    const Transform3d& cur_mat = slices[i]->getTransform3d();
    const Transform3d& prev_mat = slices[i - 1]->getTransform3d();
    // Build new polygon set in 3D from 2D outlines
    *cur = expand_poly2d_to_ccw3d(slices[i], node.convexity);
    // Plane equations for these matrices
    Vector3d cur_origin(cur_mat * Vector3d(0, 0, 0));
    Vector3d cur_abc(cur_mat * Vector3d(0, 0, 1) - cur_origin);
    double cur_d = -(cur_abc.dot(cur_origin));
    Vector3d prev_origin(prev_mat * Vector3d(0, 0, 0));
    Vector3d prev_abc(prev_mat * Vector3d(0, 0, 1) - prev_origin);
    double prev_d = -(prev_abc.dot(prev_origin));

    // Decide whether to reverse the list of slices.  Each slice should be located within
    // +Z of previous, but it's easy to get that backward, and annoying to the user to have
    // to fix it.  This could also be a result of fixing the winding order of the polygons.
    if (i == 1 && !reversed) {
      // Take a guess based on the first point that isn't on this plane
      // (a point from slice 0 can appear on the plane of slice 1 if they share an axis)
      int direction = 0;
      for (p = 0; !direction && p < (*cur)->indices.size(); p++)
        for (v = 0; !direction && v < (*cur)->indices[p].size(); v++)
          direction = check_extrusion_progression((*prev)->vertices[(*prev)->indices[p][v]],
                                                  (*cur)->vertices[(*cur)->indices[p][v]], prev_abc,
                                                  prev_d, CLOSE_ENOUGH);
      // If negative direction, reverse the list and restart the loop
      if (direction < 0) {
        std::reverse(slices.begin(), slices.end());
        i = 0;
        reversed = 1;
        // Need to re-calculate the starting points for slice 0
        cur = &tmp0;
        *cur = expand_poly2d_to_ccw3d(slices[0], node.convexity);
        continue;
      }
    }

    // If final slice looks mostly identical to first slice, then connect it to the first slice
    if (i == slices.size() - 1) {
      bool closed_loop = true;
      for (p = 0; closed_loop && p < (*cur)->indices.size(); p++) {
        for (v = 0; closed_loop && v < (*cur)->indices[p].size(); v++) {
          Vector3d const& tmp0_v = tmp0->vertices[tmp0->indices[p][v]];
          Vector3d const& cur_v = (*cur)->vertices[(*cur)->indices[p][v]];
          closed_loop = fabs(tmp0_v[0] - cur_v[0]) < CLOSE_ENOUGH &&
                        fabs(tmp0_v[1] - cur_v[1]) < CLOSE_ENOUGH &&
                        fabs(tmp0_v[2] - cur_v[2]) < CLOSE_ENOUGH;
        }
      }
      if (closed_loop)  // use exact original coordinates
        cur = &tmp0;
      else {  // else need to append end-cap polygons
        // Always progress in +Z direction, so start needs reversed, and end does not.
        auto start = slices[0]->tessellate(true);
        for (auto& p : start->indices) std::reverse(p.begin(), p.end());
        result.appendPolySet(*start);

        auto end = slices[i]->tessellate(true);
        result.appendPolySet(*end);
      }
    }

    // For each pair of adjacent vertices on each of the current and previous
    // polygons, build a quad between them using two triangles.  However, check if the
    // slices share a vertex like will happen if extruding around an axis, and in those
    // cases either make one triangle or exclude the polygon entirely.
    for (p = 0; p < (*cur)->indices.size() && progression >= 0; p++) {
      size_t v0 = (*cur)->indices[p].size() - 1;
      Vector3d const& outer_cur0 = (*cur)->vertices[(*cur)->indices[p][v0]];
      Vector3d const& outer_prev0 = (*prev)->vertices[(*prev)->indices[p][v0]];
      // previous vertex must be -Z of current plane
      progression = -check_extrusion_progression(outer_cur0, outer_prev0, cur_abc, cur_d, CLOSE_ENOUGH);
      if (progression < 0) break;
      // next vertex must be +Z of previous plane
      progression = check_extrusion_progression(outer_prev0, outer_cur0, prev_abc, prev_d, CLOSE_ENOUGH);
      int v0_progression = progression;
      for (size_t v1 = 0; v1 < (*cur)->indices[p].size() && progression >= 0; v0 = v1, ++v1) {
        Vector3d const& cur0 = (*cur)->vertices[(*cur)->indices[p][v0]];
        Vector3d const& prev0 = (*prev)->vertices[(*prev)->indices[p][v0]];
        Vector3d const& cur1 = (*cur)->vertices[(*cur)->indices[p][v1]];
        Vector3d const& prev1 = (*prev)->vertices[(*prev)->indices[p][v1]];

        // previous vertex must be -Z of current plane
        progression = -check_extrusion_progression(cur1, prev1, cur_abc, cur_d, CLOSE_ENOUGH);
        if (progression < 0) break;
        // next vertex must be +Z of previous plane
        progression = check_extrusion_progression(prev1, cur1, prev_abc, prev_d, CLOSE_ENOUGH);

        outputQuad(result, prev0, prev1, cur0, cur1, v0_progression > 0, progression > 0);
        v0_progression = progression;
      }
    }
    if (progression < 0) break;
  }
  if (progression < 0) {
    LOG(message_group::Error, loc, docpath,
        "An extrusion slice must not intersect the plane of its neighbors"
        " (collision at slice %1$d)",
        (reversed ? slices.size() - 1 - i : i));
    return nullptr;
  }
  return result.build();
}
