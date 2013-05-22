/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
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

#ifdef _MSC_VER 
// Boost conflicts with MPFR under MSVC (google it)
#include <mpfr.h>
#endif

// dxfdata.h must come first for Eigen SIMD alignment issues
#include "dxfdata.h"
#include "polyset.h"

#include "CGALRenderer.h"
#include "CGAL_renderer.h"
#include "dxftess.h"
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"

//#include "Preferences.h"

CGALRenderer::CGALRenderer(const CGAL_Nef_polyhedron &root) : root(root)
{
	if (this->root.isNull()) {
		this->polyhedron = NULL;
		this->polyset = NULL;
	}
	else if (root.dim == 2) {
		DxfData *dd = root.convertToDxfData();
		this->polyhedron = NULL;
		this->polyset = new PolySet();
		this->polyset->is2d = true;
		dxf_tesselate(this->polyset, *dd, 0, Vector2d(1,1), true, false, 0);
		delete dd;
	}
	else if (root.dim == 3) {
		this->polyset = NULL;
		this->polyhedron = new Polyhedron();
    // FIXME: Make independent of Preferences
		// this->polyhedron->setColor(Polyhedron::CGAL_NEF3_MARKED_FACET_COLOR,
		// 													 Preferences::inst()->color(Preferences::CGAL_FACE_BACK_COLOR).red(),
		// 													 Preferences::inst()->color(Preferences::CGAL_FACE_BACK_COLOR).green(),
		// 													 Preferences::inst()->color(Preferences::CGAL_FACE_BACK_COLOR).blue());
		// this->polyhedron->setColor(Polyhedron::CGAL_NEF3_UNMARKED_FACET_COLOR,
		// 													 Preferences::inst()->color(Preferences::CGAL_FACE_FRONT_COLOR).red(),
		// 													 Preferences::inst()->color(Preferences::CGAL_FACE_FRONT_COLOR).green(),
		// 													 Preferences::inst()->color(Preferences::CGAL_FACE_FRONT_COLOR).blue());
		
		CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(*this->root.p3, this->polyhedron);
		this->polyhedron->init();
	}
}

CGALRenderer::~CGALRenderer()
{
	delete this->polyset;
	delete this->polyhedron;
}

void CGALRenderer::draw(bool showfaces, bool showedges) const
{
	if (this->root.isNull()) return;
	if (this->root.dim == 2) {
		// Draw 2D polygons
		glDisable(GL_LIGHTING);
// FIXME:		const QColor &col = Preferences::inst()->color(Preferences::CGAL_FACE_2D_COLOR);
		glColor3f(0.0f, 0.75f, 0.60f);
		
		for (size_t i=0; i < this->polyset->polygons.size(); i++) {
			glBegin(GL_POLYGON);
			for (size_t j=0; j < this->polyset->polygons[i].size(); j++) {
				const Vector3d &p = this->polyset->polygons[i][j];
				glVertex3d(p[0], p[1], -0.1);
			}
			glEnd();
		}
		
		typedef CGAL_Nef_polyhedron2::Explorer Explorer;
		typedef Explorer::Face_const_iterator fci_t;
		typedef Explorer::Halfedge_around_face_const_circulator heafcc_t;
		typedef Explorer::Point Point;
		Explorer E = this->root.p2->explorer();
		
		// Draw 2D edges
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glLineWidth(2);
// FIXME:		const QColor &col2 = Preferences::inst()->color(Preferences::CGAL_EDGE_2D_COLOR);
		glColor3f(1.0f, 0.0f, 0.0f);
		
		// Extract the boundary, including inner boundaries of the polygons
		for (fci_t fit = E.faces_begin(), facesend = E.faces_end(); fit != facesend; ++fit) {
			bool fset = false;
			double fx = 0.0, fy = 0.0;
			heafcc_t fcirc(E.halfedge(fit)), fend(fcirc);
			CGAL_For_all(fcirc, fend) {
				if(E.is_standard(E.target(fcirc))) {
					Point p = E.point(E.target(fcirc));
					double x = to_double(p.x()), y = to_double(p.y());
					if (!fset) {
						glBegin(GL_LINE_STRIP);
						fx = x, fy = y;
						fset = true;
					}
					glVertex3d(x, y, -0.1);
				}
			}
			if (fset) {
				glVertex3d(fx, fy, -0.1);
				glEnd();
			}
		}
		
		glEnable(GL_DEPTH_TEST);
	}
	else if (this->root.dim == 3) {
		if (showfaces) this->polyhedron->set_style(SNC_BOUNDARY);
		else this->polyhedron->set_style(SNC_SKELETON);
		
		this->polyhedron->draw(showfaces && showedges);
  }
}
