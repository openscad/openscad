#include "geometry/extrude.h"

#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"

/* Returns whether travel from p0 => p1 is a negative, zero, or positive distance
 * in the direction of the extusion, with respect to p0's plane.
 */
inline int check_extrusion_progression(const Vector3d &p0, const Vector3d &p1, const Vector3d &plane_abc, double plane_d, double equality_tolerance) {
  // point is clearly above the plane?
  double plane_dist= plane_abc.dot(p1) + plane_d;
  if (plane_dist > equality_tolerance)
    return 1;
  // point lies on the plane, and is same as previous point?
  else if (plane_dist > -equality_tolerance && (p1 - p0).squaredNorm() < equality_tolerance)
    return 0;
  // point crossed the plane, or lies on the plane and isn't the same point.
  else
    return -1;
}

static std::unique_ptr<PolySet> expand_poly2d_to_ccw3d(std::shared_ptr<const Polygon2d> const & poly2d, unsigned int convexity) {
  PolySetBuilder builder;
  builder.setConvexity(convexity);

  // unpack all the 2D coordinates into 3D vectors with Z=0
  for (const auto &outline : poly2d->untransformedOutlines()) {
    builder.beginPolygon(outline.vertices.size());
    for (const auto &vtx : outline.vertices)
      builder.addVertex(Vector3d(vtx[0], vtx[1], 0));
    // Make sure winding order is CCW
    //if (polyset->indices.back().size() > 2) {
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
bool sanityCheckNoNullSlices(const ExtrudeNode &node, std::vector<std::shared_ptr<const Polygon2d>> const & slices, const Location &loc, std::string const & docpath)
{
  for (int i=0; i!=slices.size(); ++i)
  {
    if (slices[i]==nullptr)
    {
      LOG(message_group::Error, loc, docpath, "%1$s has a null slice at index %2$d", node.name(), i);
      return false;
    }
  }
  return true;
}

// Check for matching contours and vertices
bool sanityCheckContoursAndVertices(const ExtrudeNode &node, std::vector<std::shared_ptr<const Polygon2d>> const & slices, const Location &loc, std::string const & docpath)
{
  for (int i= 1; i < slices.size(); i++) {
    bool match = slices[i]->untransformedOutlines().size() == slices[0]->untransformedOutlines().size();
    for (int p = 0; match && p < slices[i]->untransformedOutlines().size(); p++)
      match = slices[i]->untransformedOutlines()[p].vertices.size() == slices[0]->untransformedOutlines()[p].vertices.size();
    if (!match) {
      LOG(message_group::Error, loc, docpath, "Each extrusion slice must have exactly the same vertex count,\n"
        "(note that polygon sanitization may remove duplicate vertices or co-linear points)");
      // Collect details to help debug
      std::stringstream desc_0, desc_i;
      for (const auto &o : slices[0]->untransformedOutlines()) desc_0 << " " << o.vertices.size() << "vtx";
      for (const auto &o : slices[i]->untransformedOutlines()) desc_i << " " << o.vertices.size() << "vtx";
      LOG(message_group::Error, loc, docpath, " slice   0 - %1$2d outlines: %2$s", slices[0]->untransformedOutlines().size() , desc_0.str().c_str());
      LOG(message_group::Error, loc, docpath, " slice %1$3d - %2$2d outlines: %3$s", i , slices[i]->untransformedOutlines().size() , desc_i.str().c_str());
      return false;
    }
  }
  return true;
}

/*!
  input: List of 2D objects arranged in 3D, each with identical outline count and vertex count
  output: 3D PolySet
 */
std::shared_ptr<const Geometry> extrudePolygonSequence(const ExtrudeNode &node, std::vector<std::shared_ptr<const Polygon2d>> slices, const Location &loc, std::string const & docpath)
{
  size_t i, p, v;
  const double CLOSE_ENOUGH = 0.00000000000000001; // tolerance for identical coordinates

  // Verify there is something to work with
  if (slices.size() < 2) {
    LOG(message_group::Error, loc, docpath, "%1$s requires at least two slices",node.name());
    return nullptr;
  }

  // Check for no null slices
  if (!sanityCheckNoNullSlices(node, slices, loc, docpath))
    return nullptr;
  
  // Verify that every slice has the same number of contours with the same number of vertices
  if (!sanityCheckContoursAndVertices(node, slices, loc, docpath))
    return nullptr;

  // Start extruding slices.  Come back to "end caps" at the end.
  int reversed= 0;
  std::unique_ptr<PolySet> tmp0, tmp1, tmp2;
  PolySetBuilder result;
  result.setConvexity(node.convexity);

  // Unroll first iteration so we have a "prev" to work with, and so we can use it again at the end
  tmp0 = expand_poly2d_to_ccw3d(slices[0], node.convexity);
  
  std::unique_ptr<PolySet> * cur = &tmp1, * prev = &tmp0;
  int progression= 0;
  for (i = 1; i < slices.size(); i++, prev = cur, cur = (cur == &tmp1? &tmp2 : &tmp1)) {
    const Transform3d &cur_mat = slices[i]->getTransform3d();
    const Transform3d &prev_mat = slices[i-1]->getTransform3d();
    // Build new polygon set in 3D from 2D outlines
    *cur = expand_poly2d_to_ccw3d(slices[i], node.convexity);
    // Plane equations for these matrices
    Vector3d cur_origin(cur_mat * Vector3d(0,0,0));
    Vector3d cur_abc(cur_mat * Vector3d(0,0,1) - cur_origin);
    double cur_d = - (cur_abc.dot(cur_origin));
    Vector3d prev_origin(prev_mat * Vector3d(0,0,0));
    Vector3d prev_abc(prev_mat * Vector3d(0,0,1) - prev_origin);
    double prev_d = - (prev_abc.dot(prev_origin));

    
    // Decide whether to reverse the list of slices.  Each slice should be located within
    // +Z of previous, but it's easy to get that backward, and annoying to the user to have
    // to fix it.  This could also be a result of fixing the winding order of the polygons.
    if (i == 1 && !reversed) {
      // Take a guess based on the first point that isn't on this plane
      // (a point from slice 0 can appear on the plane of slice 1 if they share an axis)
      int direction = 0;
      for (p = 0; !direction && p < (*cur)->indices.size(); p++)
        for (v = 0; !direction && v < (*cur)->indices[p].size(); v++)
          direction = check_extrusion_progression(
            (*prev)->vertices[(*prev)->indices[p][v]],
            (*cur)->vertices[(*cur)->indices[p][v]],
            prev_abc, prev_d, CLOSE_ENOUGH
          );
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
    if (i == slices.size()-1) {
      bool closed_loop = true;
      for (p = 0; closed_loop && p < (*cur)->indices.size(); p++) {
        for (v = 0; closed_loop && v < (*cur)->indices[p].size(); v++) {
	  Vector3d const & tmp0_v = tmp0->vertices[tmp0->indices[p][v]];
	  Vector3d const & cur_v = (*cur)->vertices[(*cur)->indices[p][v]];
          closed_loop = fabs(tmp0_v[0] - cur_v[0]) < CLOSE_ENOUGH
                     && fabs(tmp0_v[1] - cur_v[1]) < CLOSE_ENOUGH
                     && fabs(tmp0_v[2] - cur_v[2]) < CLOSE_ENOUGH;
        }
      }
      if (closed_loop) // use exact original coordinates
        cur = &tmp0;
      else { // else need to append end-cap polygons
        // Always progress in +Z direction, so start needs reversed, and end does not.
        auto start = slices[0]->tessellate(true);
        for(auto &p : start->indices) std::reverse(p.begin(), p.end());
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
      size_t v0 = (*cur)->indices[p].size()-1;
      Vector3d const & outer_cur0 = (*cur)->vertices[(*cur)->indices[p][v0]];
      Vector3d const & outer_prev0 = (*prev)->vertices[(*prev)->indices[p][v0]];
      // previous vertex must be -Z of current plane
      progression= -check_extrusion_progression(outer_cur0,outer_prev0, cur_abc, cur_d, CLOSE_ENOUGH);
      if (progression < 0) break;
      // next vertex must be +Z of previous plane
      progression = check_extrusion_progression(outer_prev0,outer_cur0, prev_abc, prev_d, CLOSE_ENOUGH);
      int v0_progression= progression;
      for (size_t v1 = 0; v1 < (*cur)->indices[p].size() && progression >= 0; v0 = v1, ++v1) {
        Vector3d const & cur0 = (*cur)->vertices[(*cur)->indices[p][v0]];
        Vector3d const & prev0 = (*prev)->vertices[(*prev)->indices[p][v0]];
        Vector3d const & cur1 = (*cur)->vertices[(*cur)->indices[p][v1]];
        Vector3d const & prev1 = (*prev)->vertices[(*prev)->indices[p][v1]];

        // previous vertex must be -Z of current plane
        progression= -check_extrusion_progression(cur1,prev1, cur_abc, cur_d, CLOSE_ENOUGH);
        if (progression < 0) break;
        // next vertex must be +Z of previous plane
        progression = check_extrusion_progression(prev1,cur1, prev_abc, prev_d, CLOSE_ENOUGH);
        
	if (v0_progression > 0 && progression > 0)
	{
          // Like with linear_interpolate, triangulate on the shorter
	  double d1 = std::abs((prev0-cur1).norm());
	  double d2 = std::abs((prev1-cur0).norm());
          bool splitfirst = (d1>=d2) || (std::abs(d2-d1)<1e-4); 

	  if (splitfirst)
	  {
            result.beginPolygon(3);
            result.addVertex(cur0);
            result.addVertex(prev0);
            result.addVertex(prev1);

            result.beginPolygon(3);
            result.addVertex(prev1);
            result.addVertex(cur1);
            result.addVertex(cur0);
	  }
	  else
	  {
            result.beginPolygon(3);
            result.addVertex(cur1);
            result.addVertex(cur0);
            result.addVertex(prev0);

            result.beginPolygon(3);
            result.addVertex(prev0);
            result.addVertex(prev1);
            result.addVertex(cur1);
	  }
	}
	else 
	{
          if (v0_progression > 0) {
            result.beginPolygon(3);
            result.addVertex(cur0);
            result.addVertex(prev0);
            result.addVertex(prev1);
          }
          if (progression > 0) {
            result.beginPolygon(3);
            result.addVertex(prev1);
            result.addVertex(cur1);
            result.addVertex(cur0);
          }
	}
        v0_progression = progression;
      }
    }
    if (progression < 0) break;
  }
  if (progression < 0) {
    LOG(message_group::Error, loc, docpath, "An extrusion slice must not intersect the plane of its neighbors"
               " (collision at slice %1$d)", (reversed? slices.size()-1-i : i));
    return nullptr;
  }
  return result.build();
}

