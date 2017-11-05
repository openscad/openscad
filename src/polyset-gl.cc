#include "polyset.h"
#include "polyset-utils.h"
#include "linalg.h"
#include "printutils.h"
#include "grid.h"
#include <Eigen/LU>
// all GL functions grouped together here


#ifdef ENABLE_OPENCSG
static void draw_triangle(GLint *shaderinfo, const Vector3d &p0, const Vector3d &p1, const Vector3d &p2,
													double e0f, double e1f, double e2f, double z, bool mirror)
{
	glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
	glVertexAttrib3d(shaderinfo[4], p1[0], p1[1], p1[2] + z);
	glVertexAttrib3d(shaderinfo[5], p2[0], p2[1], p2[2] + z);
	glVertexAttrib3d(shaderinfo[6], 0.0, 1.0, 0.0);
	glVertex3d(p0[0], p0[1], p0[2] + z);
	if (!mirror) {
		glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
		glVertexAttrib3d(shaderinfo[4], p0[0], p0[1], p0[2] + z);
		glVertexAttrib3d(shaderinfo[5], p2[0], p2[1], p2[2] + z);
		glVertexAttrib3d(shaderinfo[6], 0.0, 0.0, 1.0);
		glVertex3d(p1[0], p1[1], p1[2] + z);
	}
	glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
	glVertexAttrib3d(shaderinfo[4], p0[0], p0[1], p0[2] + z);
	glVertexAttrib3d(shaderinfo[5], p1[0], p1[1], p1[2] + z);
	glVertexAttrib3d(shaderinfo[6], 1.0, 0.0, 0.0);
	glVertex3d(p2[0], p2[1], p2[2] + z);
	if (mirror) {
		glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
		glVertexAttrib3d(shaderinfo[4], p0[0], p0[1], p0[2] + z);
		glVertexAttrib3d(shaderinfo[5], p2[0], p2[1], p2[2] + z);
		glVertexAttrib3d(shaderinfo[6], 0.0, 0.0, 1.0);
		glVertex3d(p1[0], p1[1], p1[2] + z);
	}
}
#endif // ifdef ENABLE_OPENCSG

static void draw_tri(const Vector3d &p0, const Vector3d &p1, const Vector3d &p2, double z, bool mirror)
{
	glVertex3d(p0[0], p0[1], p0[2] + z);
	if (!mirror) glVertex3d(p1[0], p1[1], p1[2] + z);
	glVertex3d(p2[0], p2[1], p2[2] + z);
	if (mirror) glVertex3d(p1[0], p1[1], p1[2] + z);
}

#ifndef NULLGL
static void gl_draw_triangle(GLint *shaderinfo, const Vector3d &p0, const Vector3d &p1, const Vector3d &p2, bool e0, bool e1, bool e2, double z, bool mirrored)
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
		double e0f = e0 ? 2.0 : -1.0;
		double e1f = e1 ? 2.0 : -1.0;
		double e2f = e2 ? 2.0 : -1.0;
		draw_triangle(shaderinfo, p0, p1, p2, e0f, e1f, e2f, z, mirrored);
	}
	else
#endif
	{
		draw_tri(p0, p1, p2, z, mirrored);
	}
}

void PolySet::render_surface(Renderer::csgmode_e csgmode, const Transform3d &m, GLint *shaderinfo) const
{
	PRINTD("Polyset render");
	bool mirrored = m.matrix().determinant() < 0;
#ifdef ENABLE_OPENCSG
	if (shaderinfo) {
		glUniform1f(shaderinfo[7], shaderinfo[9]);
		glUniform1f(shaderinfo[8], shaderinfo[10]);
	}
#endif /* ENABLE_OPENCSG */
	if (this->dim == 2) {
		// Render 2D objects 1mm thick, but differences slightly larger
		double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0);
		glBegin(GL_TRIANGLES);

		// Render top+bottom
		for (double z = -zbase / 2; z < zbase; z += zbase) {
			for (size_t i = 0; i < polygons.size(); i++) {
				const Polygon *poly = &polygons[i];
				if (poly->size() == 3) {
					if (z < 0) {
						gl_draw_triangle(shaderinfo, poly->at(0), poly->at(2), poly->at(1), true, true, true, z, mirrored);
					}
					else {
						gl_draw_triangle(shaderinfo, poly->at(0), poly->at(1), poly->at(2), true, true, true, z, mirrored);
					}
				}
				else if (poly->size() == 4) {
					if (z < 0) {
						gl_draw_triangle(shaderinfo, poly->at(0), poly->at(3), poly->at(1), true, false, true, z, mirrored);
						gl_draw_triangle(shaderinfo, poly->at(2), poly->at(1), poly->at(3), true, false, true, z, mirrored);
					}
					else {
						gl_draw_triangle(shaderinfo, poly->at(0), poly->at(1), poly->at(3), true, false, true, z, mirrored);
						gl_draw_triangle(shaderinfo, poly->at(2), poly->at(3), poly->at(1), true, false, true, z, mirrored);
					}
				}
				else {
					Vector3d center = Vector3d::Zero();
					for (size_t j = 0; j < poly->size(); j++) {
						center[0] += poly->at(j)[0];
						center[1] += poly->at(j)[1];
					}
					center[0] /= poly->size();
					center[1] /= poly->size();
					for (size_t j = 1; j <= poly->size(); j++) {
						if (z < 0) {
							gl_draw_triangle(shaderinfo, center, poly->at(j % poly->size()), poly->at(j - 1),
															 false, true, false, z, mirrored);
						}
						else {
							gl_draw_triangle(shaderinfo, center, poly->at(j - 1), poly->at(j % poly->size()),
															 false, true, false, z, mirrored);
						}
					}
				}
			}
		}

		// Render sides
		if (polygon.outlines().size() > 0) {
			for (const Outline2d &o : polygon.outlines()) {
				for (size_t j = 1; j <= o.vertices.size(); j++) {
					Vector3d p1(o.vertices[j - 1][0], o.vertices[j - 1][1], -zbase / 2);
					Vector3d p2(o.vertices[j - 1][0], o.vertices[j - 1][1], zbase / 2);
					Vector3d p3(o.vertices[j % o.vertices.size()][0], o.vertices[j % o.vertices.size()][1], -zbase / 2);
					Vector3d p4(o.vertices[j % o.vertices.size()][0], o.vertices[j % o.vertices.size()][1], zbase / 2);
					gl_draw_triangle(shaderinfo, p2, p1, p3, true, true, false, 0, mirrored);
					gl_draw_triangle(shaderinfo, p2, p3, p4, false, true, true, 0, mirrored);
				}
			}
		}
		else {
			// If we don't have borders, use the polygons as borders.
			// FIXME: When is this used?
			const Polygons *borders_p = &polygons;
			for (size_t i = 0; i < borders_p->size(); i++) {
				const Polygon *poly = &borders_p->at(i);
				for (size_t j = 1; j <= poly->size(); j++) {
					Vector3d p1 = poly->at(j - 1), p2 = poly->at(j - 1);
					Vector3d p3 = poly->at(j % poly->size()), p4 = poly->at(j % poly->size());
					p1[2] -= zbase / 2, p2[2] += zbase / 2;
					p3[2] -= zbase / 2, p4[2] += zbase / 2;
					gl_draw_triangle(shaderinfo, p2, p1, p3, true, true, false, 0, mirrored);
					gl_draw_triangle(shaderinfo, p2, p3, p4, false, true, true, 0, mirrored);
				}
			}
		}
		glEnd();
	}
	else if (this->dim == 3) {
		for (size_t i = 0; i < polygons.size(); i++) {
			const Polygon *poly = &polygons[i];
			glBegin(GL_TRIANGLES);
			if (poly->size() == 3) {
				gl_draw_triangle(shaderinfo, poly->at(0), poly->at(1), poly->at(2), true, true, true, 0, mirrored);
			}
			else if (poly->size() == 4) {
				gl_draw_triangle(shaderinfo, poly->at(0), poly->at(1), poly->at(3), true, false, true, 0, mirrored);
				gl_draw_triangle(shaderinfo, poly->at(2), poly->at(3), poly->at(1), true, false, true, 0, mirrored);
			}
			else {
				Vector3d center = Vector3d::Zero();
				for (size_t j = 0; j < poly->size(); j++) {
					center[0] += poly->at(j)[0];
					center[1] += poly->at(j)[1];
					center[2] += poly->at(j)[2];
				}
				center[0] /= poly->size();
				center[1] /= poly->size();
				center[2] /= poly->size();
				for (size_t j = 1; j <= poly->size(); j++) {
					gl_draw_triangle(shaderinfo, center, poly->at(j - 1), poly->at(j % poly->size()), false, true, false, 0, mirrored);
				}
			}
			glEnd();
		}
	}
	else {
		assert(false && "Cannot render object with no dimension");
	}
}

/*! This is used in throwntogether and CGAL mode

   csgmode is set to CSGMODE_NONE in CGAL mode. In this mode a pure 2D rendering is performed.

   For some reason, this is not used to render edges in Preview mode
 */
void PolySet::render_edges(Renderer::csgmode_e csgmode) const
{
	glDisable(GL_LIGHTING);
	if (this->dim == 2) {
		if (csgmode == Renderer::CSGMODE_NONE) {
			// Render only outlines
			for (const Outline2d &o : polygon.outlines()) {
				glBegin(GL_LINE_LOOP);
				for (const Vector2d &v : o.vertices) {
					glVertex3d(v[0], v[1], 0);
				}
				glEnd();
			}
		}
		else {
			// Render 2D objects 1mm thick, but differences slightly larger
			double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0);

			for (const Outline2d &o : polygon.outlines()) {
				// Render top+bottom outlines
				for (double z = -zbase / 2; z < zbase; z += zbase) {
					glBegin(GL_LINE_LOOP);
					for (const Vector2d &v : o.vertices) {
						glVertex3d(v[0], v[1], z);
					}
					glEnd();
				}
				// Render sides
				glBegin(GL_LINES);
				for (const Vector2d &v : o.vertices) {
					glVertex3d(v[0], v[1], -zbase / 2);
					glVertex3d(v[0], v[1], +zbase / 2);
				}
				glEnd();
			}
		}
	}
	else if (dim == 3) {
		for (size_t i = 0; i < polygons.size(); i++) {
			const Polygon *poly = &polygons[i];
			glBegin(GL_LINE_LOOP);
			for (size_t j = 0; j < poly->size(); j++) {
				const Vector3d &p = poly->at(j);
				glVertex3d(p[0], p[1], p[2]);
			}
			glEnd();
		}
	}
	else {
		assert(false && "Cannot render object with no dimension");
	}
	glEnable(GL_LIGHTING);
}


#else //NULLGL
static void gl_draw_triangle(GLint *shaderinfo, const Vector3d &p0, const Vector3d &p1, const Vector3d &p2, bool e0, bool e1, bool e2, double z, bool mirrored) {}
void PolySet::render_surface(Renderer::csgmode_e csgmode, const Transform3d &m, GLint *shaderinfo) const {}
void PolySet::render_edges(Renderer::csgmode_e csgmode) const {}
#endif //NULLGL

