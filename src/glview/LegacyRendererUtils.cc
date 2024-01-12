#include "LegacyRendererUtils.h"
#include "PolySet.h"
#include "Polygon2d.h"
#include "ColorMap.h"
#include "printutils.h"
#include "PlatformUtils.h"
#include "system-gl.h"

#include <Eigen/LU>
#include <fstream>

#ifdef ENABLE_OPENCSG
static void draw_triangle(const Renderer::shaderinfo_t *shaderinfo, const Vector3d& p0, const Vector3d& p1, const Vector3d& p2,
                          bool e0, bool e1, bool e2, double z, bool mirror)
{
  Renderer::shader_type_t type =
    (shaderinfo) ? shaderinfo->type : Renderer::NONE;

  // e0,e1,e2 are used to disable some edges from display.
  // Edges are numbered to correspond with the vertex opposite of them.
  // The edge shader draws edges when the minimum component of barycentric coords is near 0
  // Disabled edges have their corresponding components set to 1.0 when they would otherwise be 0.0.
  double d0 = e0 ? 0.0 : 1.0;
  double d1 = e1 ? 0.0 : 1.0;
  double d2 = e2 ? 0.0 : 1.0;

  switch (type) {
  case Renderer::EDGE_RENDERING:
    if (mirror) {
      glVertexAttrib3f(shaderinfo->data.csg_rendering.barycentric, 1.0, d1, d2);
      glVertex3f(p0[0], p0[1], p0[2] + z);
      glVertexAttrib3f(shaderinfo->data.csg_rendering.barycentric, d0, d1, 1.0);
      glVertex3f(p2[0], p2[1], p2[2] + z);
      glVertexAttrib3f(shaderinfo->data.csg_rendering.barycentric, d0, 1.0, d2);
      glVertex3f(p1[0], p1[1], p1[2] + z);
    } else {
      glVertexAttrib3f(shaderinfo->data.csg_rendering.barycentric, 1.0, d1, d2);
      glVertex3f(p0[0], p0[1], p0[2] + z);
      glVertexAttrib3f(shaderinfo->data.csg_rendering.barycentric, d0, 1.0, d2);
      glVertex3f(p1[0], p1[1], p1[2] + z);
      glVertexAttrib3f(shaderinfo->data.csg_rendering.barycentric, d0, d1, 1.0);
      glVertex3f(p2[0], p2[1], p2[2] + z);
    }
    break;
  default:
  case Renderer::SELECT_RENDERING:
    glVertex3d(p0[0], p0[1], p0[2] + z);
    if (!mirror) {
      glVertex3d(p1[0], p1[1], p1[2] + z);
    }
    glVertex3d(p2[0], p2[1], p2[2] + z);
    if (mirror) {
      glVertex3d(p1[0], p1[1], p1[2] + z);
    }
  }
}
#endif // ifdef ENABLE_OPENCSG

static void draw_tri(const Vector3d& p0, const Vector3d& p1, const Vector3d& p2, double z, bool mirror)
{
  glVertex3d(p0[0], p0[1], p0[2] + z);
  if (!mirror) glVertex3d(p1[0], p1[1], p1[2] + z);
  glVertex3d(p2[0], p2[1], p2[2] + z);
  if (mirror) glVertex3d(p1[0], p1[1], p1[2] + z);
}

static void gl_draw_triangle(const Renderer::shaderinfo_t *shaderinfo, const Vector3d& p0, const Vector3d& p1, const Vector3d& p2, bool e0, bool e1, bool e2, double z, bool mirrored)
{
  double ax = p1[0] - p0[0], bx = p1[0] - p2[0];
  double ay = p1[1] - p0[1], by = p1[1] - p2[1];
  double az = p1[2] - p0[2], bz = p1[2] - p2[2];
  double nx = ay * bz - az * by;
  double ny = az * bx - ax * bz;
  double nz = ax * by - ay * bx;
  double nl = sqrt(nx * nx + ny * ny + nz * nz);
  glNormal3d(nx / nl, ny / nl, nz / nl);
#ifdef ENABLE_OPENCSG
  if (shaderinfo) {
    draw_triangle(shaderinfo, p0, p1, p2, e0, e1, e2, z, mirrored);
  } else
#endif
  {
    draw_tri(p0, p1, p2, z, mirrored);
  }
}

void render_surface(const PolySet& ps, Renderer::csgmode_e csgmode, const Transform3d& m, const Renderer::shaderinfo_t *shaderinfo)
{
  PRINTD("render_surface");
  bool mirrored = m.matrix().determinant() < 0;

  if (ps.getDimension() == 2) {
    // Render 2D objects 1mm thick, but differences slightly larger
    double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0);
    glBegin(GL_TRIANGLES);

    // Render top+bottom
    for (double z : {-zbase / 2, zbase / 2}) {
      for (const auto& poly : ps.indices) {
        if (poly.size() == 3) {
          if (z < 0) {
            gl_draw_triangle(shaderinfo, ps.vertices[poly.at(0)], ps.vertices[poly.at(2)], ps.vertices[poly.at(1)], true, true, true, z, mirrored);
          } else {
            gl_draw_triangle(shaderinfo, ps.vertices[poly.at(0)], ps.vertices[poly.at(1)], ps.vertices[poly.at(2)], true, true, true, z, mirrored);
          }
        } else if (poly.size() == 4) {
          if (z < 0) {
            gl_draw_triangle(shaderinfo, ps.vertices[poly.at(0)], ps.vertices[poly.at(3)], ps.vertices[poly.at(1)], false, true, true, z, mirrored);
            gl_draw_triangle(shaderinfo, ps.vertices[poly.at(2)], ps.vertices[poly.at(1)], ps.vertices[poly.at(3)], false, true, true, z, mirrored);
          } else {
            gl_draw_triangle(shaderinfo, ps.vertices[poly.at(0)], ps.vertices[poly.at(1)], ps.vertices[poly.at(3)], false, true, true, z, mirrored);
            gl_draw_triangle(shaderinfo, ps.vertices[poly.at(2)], ps.vertices[poly.at(3)], ps.vertices[poly.at(1)], false, true, true, z, mirrored);
          }
        } else {
          Vector3d center = Vector3d::Zero();
          for (const auto& point : poly) {
            center[0] += ps.vertices[point][0];
            center[1] += ps.vertices[point][1];
          }
          center /= poly.size();
          for (size_t j = 1; j <= poly.size(); ++j) {
            if (z < 0) {
              gl_draw_triangle(shaderinfo, center, ps.vertices[poly.at(j % poly.size())], ps.vertices[poly.at(j - 1)],
                               true, false, false, z, mirrored);
            } else {
              gl_draw_triangle(shaderinfo, center, ps.vertices[poly.at(j - 1)], ps.vertices[poly.at(j % poly.size())],
                               true, false, false, z, mirrored);
            }
          }
        }
      }
    }

    // Render sides
    if (ps.getPolygon().outlines().size() > 0) {
      for (const Outline2d& o : ps.getPolygon().outlines()) {
        for (size_t j = 1; j <= o.vertices.size(); ++j) {
          Vector3d p1(o.vertices[j - 1][0], o.vertices[j - 1][1], -zbase / 2);
          Vector3d p2(o.vertices[j - 1][0], o.vertices[j - 1][1], zbase / 2);
          Vector3d p3(o.vertices[j % o.vertices.size()][0], o.vertices[j % o.vertices.size()][1], -zbase / 2);
          Vector3d p4(o.vertices[j % o.vertices.size()][0], o.vertices[j % o.vertices.size()][1], zbase / 2);
          gl_draw_triangle(shaderinfo, p2, p1, p3, true, false, true, 0, mirrored);
          gl_draw_triangle(shaderinfo, p2, p3, p4, true, true, false, 0, mirrored);
        }
      }
    } else {
      // If we don't have borders, use the polygons as borders.
      // FIXME: When is this used?
      const std::vector<IndexedFace> *borders_p = &ps.indices;
      for (const auto& poly : *borders_p) {
        for (size_t j = 1; j <= poly.size(); ++j) {
          Vector3d p1 = ps.vertices[poly.at(j - 1)], p2 = ps.vertices[poly.at(j - 1)];
          Vector3d p3 = ps.vertices[poly.at(j % poly.size())], p4 = ps.vertices[poly.at(j % poly.size())];
          p1[2] -= zbase / 2, p2[2] += zbase / 2;
          p3[2] -= zbase / 2, p4[2] += zbase / 2;
          gl_draw_triangle(shaderinfo, p2, p1, p3, true, false, true, 0, mirrored);
          gl_draw_triangle(shaderinfo, p2, p3, p4, true, true, false, 0, mirrored);
        }
      }
    }
    glEnd();
  } else if (ps.getDimension() == 3) {
    for (const auto& poly : ps.indices) {
      glBegin(GL_TRIANGLES);
      if (poly.size() == 3) {
        gl_draw_triangle(shaderinfo, ps.vertices[poly.at(0)], ps.vertices[poly.at(1)], ps.vertices[poly.at(2)], true, true, true, 0, mirrored);
      } else if (poly.size() == 4) {
        gl_draw_triangle(shaderinfo, ps.vertices[poly.at(0)], ps.vertices[poly.at(1)], ps.vertices[poly.at(3)], false, true, true, 0, mirrored);
        gl_draw_triangle(shaderinfo, ps.vertices[poly.at(2)], ps.vertices[poly.at(3)], ps.vertices[poly.at(1)], false, true, true, 0, mirrored);
      } else {
        Vector3d center = Vector3d::Zero();
        for (const auto& point : poly) {
          center += ps.vertices[point];
        }
        center /= poly.size();
        for (size_t j = 1; j <= poly.size(); ++j) {
          gl_draw_triangle(shaderinfo, center, ps.vertices[poly.at(j - 1)], ps.vertices[poly.at(j % poly.size())], true, false, false, 0, mirrored);
        }
      }
      glEnd();
    }
  } else {
    assert(false && "Cannot render object with no dimension");
  }
}

/*! This is used in throwntogether and CGAL mode

   csgmode is set to CSGMODE_NONE in CGAL mode. In this mode a pure 2D rendering is performed.

   For some reason, this is not used to render edges in Preview mode
 */
void render_edges(const PolySet& ps, Renderer::csgmode_e csgmode)
{
  glDisable(GL_LIGHTING);
  if (ps.getDimension() == 2) {
    if (csgmode == Renderer::CSGMODE_NONE) {
      // Render only outlines
      for (const Outline2d& o : ps.getPolygon().outlines()) {
        glBegin(GL_LINE_LOOP);
        for (const Vector2d& v : o.vertices) {
          glVertex3d(v[0], v[1], 0);
        }
        glEnd();
      }
    } else {
      // Render 2D objects 1mm thick, but differences slightly larger
      double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0);

      for (const Outline2d& o : ps.getPolygon().outlines()) {
        // Render top+bottom outlines
        for (double z : { -zbase / 2, zbase / 2}) {
          glBegin(GL_LINE_LOOP);
          for (const Vector2d& v : o.vertices) {
            glVertex3d(v[0], v[1], z);
          }
          glEnd();
        }
        // Render sides
        glBegin(GL_LINES);
        for (const Vector2d& v : o.vertices) {
          glVertex3d(v[0], v[1], -zbase / 2);
          glVertex3d(v[0], v[1], +zbase / 2);
        }
        glEnd();
      }
    }
  } else if (ps.getDimension() == 3) {
    for (const auto& polygon : ps.indices) {
      const IndexedFace *poly = &polygon;
      glBegin(GL_LINE_LOOP);
      for (const auto& ind : *poly) {
	Vector3d p=ps.vertices[ind];
        glVertex3d(p[0], p[1], p[2]);
      }
      glEnd();
    }
  } else {
    assert(false && "Cannot render object with no dimension");
  }
  glEnable(GL_LIGHTING);
}
