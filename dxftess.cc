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

#ifdef WIN32
#  define STDCALL __stdcall
#else
#  define STDCALL
#endif

#define INCLUDE_ABSTRACT_NODE_DETAILS

#include "openscad.h"

#undef DEBUG_TRIANGLE_SPLITTING

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

static void STDCALL tess_vertex(void *vertex_data)
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

static void STDCALL tess_begin(GLenum type)
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

static void STDCALL tess_end(void)
{
	/* nothing to be done here */
}

static void STDCALL tess_error(GLenum errno)
{
	PRINTF("GLU tesselation error %d!", errno);
}

static bool point_on_line(double *p1, double *p2, double *p3)
{
	if (fabs(p1[0] - p2[0]) < 0.00001 && fabs(p1[1] - p2[1]) < 0.00001)
		return false;

	if (fabs(p3[0] - p2[0]) < 0.00001 && fabs(p3[1] - p2[1]) < 0.00001)
		return false;

	double v1[2] = { p2[0] - p1[0], p2[1] - p1[1] };
	double v2[2] = { p3[0] - p1[0], p3[1] - p1[1] };

	if (sqrt(v1[0]*v1[0] + v1[1]*v1[1]) > sqrt(v2[0]*v2[0] + v2[1]*v2[1]))
		return false;

	if (fabs(v1[0]) > fabs(v1[1])) {
		// y = x * dy/dx
		if (v2[0] == 0 || ((v1[0] > 0) != (v2[0] > 0)))
			return false;
		double v1_dy_dx = v1[1] / v1[0];
		double v2_dy_dx = v2[1] / v2[0];
		if (fabs(v1_dy_dx - v2_dy_dx) > 1e-15)
			return false;
	} else {
		// x = y * dx/dy
		if (v2[1] == 0 || ((v1[1] > 0) != (v2[1] > 0)))
			return false;
		double v1_dy_dx = v1[0] / v1[1];
		double v2_dy_dx = v2[0] / v2[1];
		if (fabs(v1_dy_dx - v2_dy_dx) > 1e-15)
			return false;
	}

#if 0
	printf("Point on line: %f/%f %f/%f %f/%f\n", p1[0], p1[1], p2[0], p2[1], p3[0], p3[1]);
#endif
	return true;
}

void dxf_tesselate(PolySet *ps, DxfData *dxf, double rot, bool up, double h)
{
	GLUtesselator *tobj = gluNewTess();

	gluTessCallback(tobj, GLU_TESS_VERTEX, (void(STDCALL *)())&tess_vertex);
	gluTessCallback(tobj, GLU_TESS_BEGIN, (void(STDCALL *)())&tess_begin);
	gluTessCallback(tobj, GLU_TESS_END, (void(STDCALL *)())&tess_end);
	gluTessCallback(tobj, GLU_TESS_ERROR, (void(STDCALL *)())&tess_error);

	tess_tri.clear();
	QList<tess_vdata> vl;

	gluTessBeginPolygon(tobj, NULL);

	gluTessProperty(tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
	if (up) {
		gluTessNormal(tobj, 0, 0, -1);
	} else {
		gluTessNormal(tobj, 0, 0, +1);
	}

	Grid3d< QPair<int,int> > point_to_path;

	for (int i = 0; i < dxf->paths.count(); i++) {
		if (!dxf->paths[i].is_closed)
			continue;
		gluTessBeginContour(tobj);
		for (int j = 1; j < dxf->paths[i].points.count(); j++) {
			point_to_path.data(dxf->paths[i].points[j]->x,
					dxf->paths[i].points[j]->y,
					h) = QPair<int,int>(i, j);
			vl.append(tess_vdata());
			vl.last().v[0] = dxf->paths[i].points[j]->x;
			vl.last().v[1] = dxf->paths[i].points[j]->y;
			vl.last().v[2] = h;
			gluTessVertex(tobj, vl.last().v, vl.last().v);
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

	// GLU tessing creates T-junctions. This is ok for GL displaying but creates
	// invalid polyeders for CGAL. So we split this tirangles up again in order
	// to create polyeders that are also accepted by CGAL..
	// All triangle edges are sorted by their atan2 and only edges with a simmilar atan2
	// value are compared. This speeds up this code block dramatically (compared to the
	// n^2 compares that are neccessary in the trivial implementation).
#if 1
	bool added_triangles = true;
	typedef QPair<int,int> QPair_ii;
	QHash<int, QPair_ii> tri_by_atan2;
	for (int i = 0; i < tess_tri.count(); i++)
	for (int j = 0; j < 3; j++) {
		int ai = (int)round(atan2(fabs(tess_tri[i].p[(j+1)%3][0] - tess_tri[i].p[j][0]),
				fabs(tess_tri[i].p[(j+1)%3][1] - tess_tri[i].p[j][1])) / 0.001);
		tri_by_atan2.insertMulti(ai, QPair<int,int>(i, j));
	}
	while (added_triangles)
	{
		added_triangles = false;
#ifdef DEBUG_TRIANGLE_SPLITTING
		printf("*** Triangle splitting (%d) ***\n", tess_tri.count()+1);
#endif
		for (int i = 0; i < tess_tri.count(); i++)
		for (int k = 0; k < 3; k++)
		{
			QHash<QPair_ii, QPair_ii> possible_neigh;
			int ai = (int)floor(atan2(fabs(tess_tri[i].p[(k+1)%3][0] - tess_tri[i].p[k][0]),
					fabs(tess_tri[i].p[(k+1)%3][1] - tess_tri[i].p[k][1])) / 0.001 - 0.5);
			for (int j = 0; j < 2; j++) {
				foreach (QPair_ii jl, tri_by_atan2.values(ai+j))
					if (i != jl.first)
						possible_neigh[jl] = jl;
			}
#ifdef DEBUG_TRIANGLE_SPLITTING
			printf("%d/%d: %d\n", i, k, possible_neigh.count());
#endif
			foreach (QPair_ii jl, possible_neigh) {
				int j = jl.first;
				for (int l = jl.second; l != (jl.second + 2) % 3; l = (l + 1) % 3)
				if (point_on_line(tess_tri[i].p[k], tess_tri[j].p[l], tess_tri[i].p[(k+1)%3])) {
#ifdef DEBUG_TRIANGLE_SPLITTING
					printf("%% %f %f %f %f %f %f [%d %d]\n",
							tess_tri[i].p[k][0], tess_tri[i].p[k][1],
							tess_tri[j].p[l][0], tess_tri[j].p[l][1],
							tess_tri[i].p[(k+1)%3][0], tess_tri[i].p[(k+1)%3][1],
							i, j);
#endif
					tess_tri.append(tess_triangle(tess_tri[j].p[l],
							tess_tri[i].p[(k+1)%3], tess_tri[i].p[(k+2)%3]));
					for (int m = 0; m < 2; m++) {
						int ai = (int)round(atan2(fabs(tess_tri.last().p[(m+1)%3][0] - tess_tri.last().p[m][0]),
								fabs(tess_tri.last().p[(m+1)%3][1] - tess_tri.last().p[m][1])) / 0.001 );
						tri_by_atan2.insertMulti(ai, QPair<int,int>(tess_tri.count()-1, m));
					}
					tess_tri[i].p[(k+1)%3] = tess_tri[j].p[l];
					for (int m = 0; m < 2; m++) {
						int ai = (int)round(atan2(fabs(tess_tri[i].p[(m+1)%3][0] - tess_tri[i].p[m][0]),
								fabs(tess_tri[i].p[(m+1)%3][1] - tess_tri[i].p[m][1])) / 0.001 );
						tri_by_atan2.insertMulti(ai, QPair<int,int>(i, m));
					}
					added_triangles = true;
				}
			}
		}
	}
#endif

	for (int i = 0; i < tess_tri.count(); i++)
	{
#if 0
		printf("---\n");
		printf("  %f %f %f\n", tess_tri[i].p[0][0], tess_tri[i].p[0][1], tess_tri[i].p[0][2]);
		printf("  %f %f %f\n", tess_tri[i].p[1][0], tess_tri[i].p[1][1], tess_tri[i].p[1][2]);
		printf("  %f %f %f\n", tess_tri[i].p[2][0], tess_tri[i].p[2][1], tess_tri[i].p[2][2]);
#endif
		double x, y;
		ps->append_poly();

		x = tess_tri[i].p[0][0] *  cos(rot*M_PI/180) + tess_tri[i].p[0][1] * sin(rot*M_PI/180);
		y = tess_tri[i].p[0][0] * -sin(rot*M_PI/180) + tess_tri[i].p[0][1] * cos(rot*M_PI/180);
		ps->insert_vertex(x, y, tess_tri[i].p[0][2]);

		x = tess_tri[i].p[1][0] *  cos(rot*M_PI/180) + tess_tri[i].p[1][1] * sin(rot*M_PI/180);
		y = tess_tri[i].p[1][0] * -sin(rot*M_PI/180) + tess_tri[i].p[1][1] * cos(rot*M_PI/180);
		ps->insert_vertex(x, y, tess_tri[i].p[1][2]);

		x = tess_tri[i].p[2][0] *  cos(rot*M_PI/180) + tess_tri[i].p[2][1] * sin(rot*M_PI/180);
		y = tess_tri[i].p[2][0] * -sin(rot*M_PI/180) + tess_tri[i].p[2][1] * cos(rot*M_PI/180);
		ps->insert_vertex(x, y, tess_tri[i].p[2][2]);

		int i0 = point_to_path.data(tess_tri[i].p[0][0], tess_tri[i].p[0][1], tess_tri[i].p[0][2]).first;
		int j0 = point_to_path.data(tess_tri[i].p[0][0], tess_tri[i].p[0][1], tess_tri[i].p[0][2]).second;

		int i1 = point_to_path.data(tess_tri[i].p[1][0], tess_tri[i].p[1][1], tess_tri[i].p[1][2]).first;
		int j1 = point_to_path.data(tess_tri[i].p[1][0], tess_tri[i].p[1][1], tess_tri[i].p[1][2]).second;

		int i2 = point_to_path.data(tess_tri[i].p[2][0], tess_tri[i].p[2][1], tess_tri[i].p[2][2]).first;
		int j2 = point_to_path.data(tess_tri[i].p[2][0], tess_tri[i].p[2][1], tess_tri[i].p[2][2]).second;

		if (i0 == i1 && j0 == 1 && j1 == 2)
			dxf->paths[i0].is_inner = !up;
		if (i0 == i1 && j0 == 2 && j1 == 1)
			dxf->paths[i0].is_inner = up;

		if (i1 == i2 && j1 == 1 && j2 == 2)
			dxf->paths[i1].is_inner = !up;
		if (i1 == i2 && j1 == 2 && j2 == 1)
			dxf->paths[i1].is_inner = up;

		if (i2 == i0 && j2 == 1 && j0 == 2)
			dxf->paths[i2].is_inner = !up;
		if (i2 == i0 && j2 == 2 && j0 == 1)
			dxf->paths[i2].is_inner = up;
	}

	tess_tri.clear();
}

