/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define INCLUDE_ABSTRACT_NODE_DETAILS

#include "openscad.h"

struct tess_vdata {
	GLdouble v[3];
};

struct tess_triangle {
	GLdouble *p[3];
	tess_triangle() { p[0] = NULL; p[1] = NULL; p[2] = NULL; }
	tess_triangle(double *p1, double *p2, double *p3) { p[0] = p1; p[1] = p2; p[2] = p3; }
};

static GLenum tess_type;
static int tess_count;
static QVector<tess_triangle> tess_tri;
static GLdouble *tess_p1, *tess_p2;

static void tess_vertex(void *vertex_data)
{
	GLdouble *p = (double*)vertex_data;
#if 0
	printf("  %d: %f %f %f\n", tess_count, p[0], p[1], p[2]);
#endif
	if (tess_type == GL_TRIANGLE_FAN) {
		if (tess_count == 0) {
			tess_p1 = p;
		}
		if (tess_count == 1) {
			tess_p2 = p;
		}
		if (tess_count > 1) {
			tess_tri.append(tess_triangle(tess_p1, tess_p2, p));
			tess_p2 = p;
		}
	}
	if (tess_type == GL_TRIANGLE_STRIP) {
		if (tess_count == 0) {
			tess_p1 = p;
		}
		if (tess_count == 1) {
			tess_p2 = p;
		}
		if (tess_count > 1) {
			if (tess_count % 2 == 1) {
				tess_tri.append(tess_triangle(tess_p2, tess_p1, p));
			} else {
				tess_tri.append(tess_triangle(tess_p1, tess_p2, p));
			}
			tess_p1 = tess_p2;
			tess_p2 = p;
		}
	}
	if (tess_type == GL_TRIANGLES) {
		if (tess_count == 0) {
			tess_p1 = p;
		}
		if (tess_count == 1) {
			tess_p2 = p;
		}
		if (tess_count == 2) {
			tess_tri.append(tess_triangle(tess_p1, tess_p2, p));
			tess_count = -1;
		}
	}
	tess_count++;
}

static void tess_begin(GLenum type)
{
#if 0
	if (type == GL_TRIANGLE_FAN) {
		printf("GL_TRIANGLE_FAN:\n");
	}
	if (type == GL_TRIANGLE_STRIP) {
		printf("GL_TRIANGLE_STRIP:\n");
	}
	if (type == GL_TRIANGLES) {
		printf("GL_TRIANGLES:\n");
	}
#endif
	tess_count = 0;
	tess_type = type;
}

static void tess_end(void)
{
	/* nothing to be done here */
}

static void tess_error(GLenum errno)
{
	PRINTF("GLU tesselation error %d!", errno);
}

static bool point_on_line(double *p1, double *p2, double *p3)
{
	if (fabs(p1[0] - p2[0]) < 0.0001 && fabs(p1[1] - p2[1]) < 0.0001)
		return false;

	if (fabs(p3[0] - p2[0]) < 0.0001 && fabs(p3[1] - p2[1]) < 0.0001)
		return false;

	double v1[2] = { p2[0] - p1[0], p2[1] - p1[1] };
	double v2[2] = { p3[0] - p1[0], p3[1] - p1[1] };

	if (fabs(atan2(v1[0], v1[1]) - atan2(v2[0], v2[1])) < 0.0001 &&
			sqrt(v1[0]*v1[0] + v1[1]*v1[1]) < sqrt(v2[0]*v2[0] + v2[1]*v2[1])) {
#if 0
		printf("Point on line: %f/%f %f/%f %f/%f\n", p1[0], p1[1], p2[0], p2[1], p3[0], p3[1]);
#endif
		return true;
	}

	return false;
}

void dxf_tesselate(PolySet *ps, DxfData *dxf, bool up, double h)
{
	GLUtesselator *tobj = gluNewTess();

	gluTessCallback(tobj, GLU_TESS_VERTEX, (GLvoid(*)())&tess_vertex);
	gluTessCallback(tobj, GLU_TESS_BEGIN, (GLvoid(*)())&tess_begin);
	gluTessCallback(tobj, GLU_TESS_END, (GLvoid(*)())&tess_end);
	gluTessCallback(tobj, GLU_TESS_ERROR, (GLvoid(*)())&tess_error);

	tess_tri.clear();
	QList<tess_vdata> vl;

	gluTessBeginPolygon(tobj, NULL);

	for (int i = 0; i < dxf->paths.count(); i++) {
		if (!dxf->paths[i].is_closed)
			continue;
		gluTessBeginContour(tobj);
		if (up != dxf->paths[i].is_inner) {
			for (int j = 1; j < dxf->paths[i].points.count(); j++) {
				vl.append(tess_vdata());
				vl.last().v[0] = dxf->paths[i].points[j]->x;
				vl.last().v[1] = dxf->paths[i].points[j]->y;
				vl.last().v[2] = h;
				gluTessVertex(tobj, vl.last().v, vl.last().v);
			}
		} else {
			for (int j = dxf->paths[i].points.count() - 1; j > 0; j--) {
				vl.append(tess_vdata());
				vl.last().v[0] = dxf->paths[i].points[j]->x;
				vl.last().v[1] = dxf->paths[i].points[j]->y;
				vl.last().v[2] = h;
				gluTessVertex(tobj, vl.last().v, vl.last().v);
			}
		}
		gluTessEndContour(tobj);
	}

	gluTessEndPolygon(tobj);
	gluDeleteTess(tobj);

#if 0
	for (int i = 0; i < tess_tri.count(); i++) {
		printf("~~~\n");
		printf("  %f %f %f\n", tess_tri[i].p[0][0], tess_tri[i].p[0][1], tess_tri[i].p[0][2]);
		printf("  %f %f %f\n", tess_tri[i].p[1][0], tess_tri[i].p[1][1], tess_tri[i].p[1][2]);
		printf("  %f %f %f\n", tess_tri[i].p[2][0], tess_tri[i].p[2][1], tess_tri[i].p[2][2]);
	}
#endif

	// GLU tessing sometimes generates degenerated triangles. We must find and remove
	// them so we can use the triangle array with CGAL..
	for (int i = 0; i < tess_tri.count(); i++) {
		if (point_on_line(tess_tri[i].p[0], tess_tri[i].p[1], tess_tri[i].p[2]) ||
				point_on_line(tess_tri[i].p[1], tess_tri[i].p[2], tess_tri[i].p[0]) ||
				point_on_line(tess_tri[i].p[2], tess_tri[i].p[0], tess_tri[i].p[1])) {
			tess_tri.remove(i--);
		}
	}

	// GLU tessing might merge points into edges. This is ok for GL displaying but
	// creates invalid polyeders for CGAL. So we split this tirangles up again in order
	// to create polyeders that are also accepted by CGAL..
	bool added_triangles = true;
	while (added_triangles)
	{
		added_triangles = false;
		for (int i = 0; i < tess_tri.count(); i++)
		for (int j = 0; j < tess_tri.count(); j++)
		for (int k = 0; k < 3; k++)
		for (int l = 0; l < 3; l++) {
			if (point_on_line(tess_tri[i].p[k], tess_tri[j].p[l], tess_tri[i].p[(k+1)%3])) {
				tess_tri.append(tess_triangle(tess_tri[j].p[l],
						tess_tri[i].p[(k+1)%3], tess_tri[i].p[(k+2)%3]));
				tess_tri[i].p[(k+1)%3] = tess_tri[j].p[l];
				added_triangles = true;
			}
		}
	}

	for (int i = 0; i < tess_tri.count(); i++) {
#if 0
		printf("---\n");
		printf("  %f %f %f\n", tess_tri[i].p[0][0], tess_tri[i].p[0][1], tess_tri[i].p[0][2]);
		printf("  %f %f %f\n", tess_tri[i].p[1][0], tess_tri[i].p[1][1], tess_tri[i].p[1][2]);
		printf("  %f %f %f\n", tess_tri[i].p[2][0], tess_tri[i].p[2][1], tess_tri[i].p[2][2]);
#endif
		ps->append_poly();
		ps->insert_vertex(tess_tri[i].p[0][0], tess_tri[i].p[0][1], tess_tri[i].p[0][2]);
		ps->insert_vertex(tess_tri[i].p[1][0], tess_tri[i].p[1][1], tess_tri[i].p[1][2]);
		ps->insert_vertex(tess_tri[i].p[2][0], tess_tri[i].p[2][1], tess_tri[i].p[2][2]);
	}

	tess_tri.clear();
}

