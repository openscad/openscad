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

#include "polyset.h"

#include "CGALRenderer.h"
#include "CGAL_renderer.h"
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"

//#include "Preferences.h"

#include <boost/foreach.hpp>

CGALRenderer::CGALRenderer(shared_ptr<const Geometry> geom)
{
	this->addGeometry(geom);
}

void CGALRenderer::addGeometry(const shared_ptr<const Geometry> &geom)
{
	if (shared_ptr<const GeometryList> geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
		BOOST_FOREACH(const Geometry::GeometryItem &item, geomlist->getChildren()) {
			this->addGeometry(item.second);
		}
	}
	else if (const shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom)) {
		this->polysets.push_back(ps);
	}
	else if (shared_ptr<const Polygon2d> poly = dynamic_pointer_cast<const Polygon2d>(geom)) {
		this->polysets.push_back(shared_ptr<const PolySet>(poly->tessellate()));
	}
	else if (shared_ptr<const CGAL_Nef_polyhedron> new_N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		assert(new_N->getDimension() == 3);
		if (!new_N->isEmpty()) {
			Polyhedron *p = new Polyhedron();
			// FIXME: Make independent of Preferences
			// this->polyhedron->setColor(Polyhedron::CGAL_NEF3_MARKED_FACET_COLOR,
			// 													 Preferences::inst()->color(Preferences::CGAL_FACE_BACK_COLOR).red(),
			// 													 Preferences::inst()->color(Preferences::CGAL_FACE_BACK_COLOR).green(),
			// 													 Preferences::inst()->color(Preferences::CGAL_FACE_BACK_COLOR).blue());
			// this->polyhedron->setColor(Polyhedron::CGAL_NEF3_UNMARKED_FACET_COLOR,
			// 													 Preferences::inst()->color(Preferences::CGAL_FACE_FRONT_COLOR).red(),
			// 													 Preferences::inst()->color(Preferences::CGAL_FACE_FRONT_COLOR).green(),
			// 													 Preferences::inst()->color(Preferences::CGAL_FACE_FRONT_COLOR).blue());
			
			CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(*new_N->p3, p);
			p->init();
			this->polyhedrons.push_back(shared_ptr<Polyhedron>(p));
		}
	}
}

CGALRenderer::~CGALRenderer()
{
}

void CGALRenderer::draw(bool showfaces, bool showedges) const
{
	BOOST_FOREACH(const shared_ptr<const PolySet> &polyset, this->polysets) {
		if (polyset->getDimension() == 2) {
			// Draw 2D polygons
			glDisable(GL_LIGHTING);
// FIXME:		const QColor &col = Preferences::inst()->color(Preferences::CGAL_FACE_2D_COLOR);
			glColor3f(0.0f, 0.75f, 0.60f);

			for (size_t i=0; i < polyset->polygons.size(); i++) {
				glBegin(GL_POLYGON);
				for (size_t j=0; j < polyset->polygons[i].size(); j++) {
					const Vector3d &p = polyset->polygons[i][j];
					glVertex3d(p[0], p[1], -0.1);
				}
				glEnd();
			}
		
			// Draw 2D edges
			glDisable(GL_DEPTH_TEST);

			glLineWidth(2);
// FIXME:		const QColor &col2 = Preferences::inst()->color(Preferences::CGAL_EDGE_2D_COLOR);
			glColor3f(1.0f, 0.0f, 0.0f);
			polyset->render_edges(CSGMODE_NONE);
			glEnable(GL_DEPTH_TEST);
		}
		else {
			// Draw 3D polygons
			const Color4f c(-1,-1,-1,-1);	
			setColor(COLORMODE_MATERIAL, c.data(), NULL);
			polyset->render_surface(CSGMODE_NORMAL, Transform3d::Identity(), NULL);
		}
	}

	BOOST_FOREACH(const shared_ptr<Polyhedron> &p, this->polyhedrons) {
		if (showfaces) p->set_style(SNC_BOUNDARY);
		else p->set_style(SNC_SKELETON);
		p->draw(showfaces && showedges);
  }
}

BoundingBox CGALRenderer::getBoundingBox() const
{
	BoundingBox bbox;

	BOOST_FOREACH(const shared_ptr<class Polyhedron> &p, this->polyhedrons) {
		CGAL::Bbox_3 cgalbbox = p->bbox();
		bbox.extend(BoundingBox(
									Vector3d(cgalbbox.xmin(), cgalbbox.ymin(), cgalbbox.zmin()),
									Vector3d(cgalbbox.xmax(), cgalbbox.ymax(), cgalbbox.zmax())));
	}
	BOOST_FOREACH(const shared_ptr<const class PolySet> &ps, this->polysets) {
		bbox.extend(ps->getBoundingBox());
	}

	return bbox;
}
