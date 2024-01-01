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

#include "PolySet.h"
#include "PolySetUtils.h"
#include "printutils.h"

#include "LegacyCGALRenderer.h"
#include "CGALRenderUtils.h"
#ifdef ENABLE_CGAL
#include "CGALHybridPolyhedron.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#endif

//#include "Preferences.h"

LegacyCGALRenderer::LegacyCGALRenderer(const std::shared_ptr<const class Geometry>& geom)
{
  this->addGeometry(geom);
  PRINTD("LegacyCGALRenderer::LegacyCGALRenderer() -> createPolyhedrons()");
#ifdef ENABLE_CGAL
  if (!this->nefPolyhedrons.empty() && this->polyhedrons.empty()) createPolyhedrons();
#endif
}

void LegacyCGALRenderer::addGeometry(const std::shared_ptr<const Geometry>& geom)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      this->addGeometry(item.second);
    }
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    assert(ps->getDimension() == 3);
    // We need to tessellate here, in case the generated PolySet contains concave polygons
    // See tests/data/scad/3D/features/polyhedron-concave-test.scad
    this->polysets.push_back(PolySetUtils::tessellate_faces(*ps));
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    this->polysets.push_back(std::shared_ptr<const PolySet>(poly->tessellate()));
#ifdef ENABLE_CGAL
  } else if (const auto new_N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    assert(new_N->getDimension() == 3);
    if (!new_N->isEmpty()) {
      this->nefPolyhedrons.push_back(new_N);
    }
  } else if (const auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    // TODO(ochafik): Implement rendering of CGAL_HybridMesh (CGAL::Surface_mesh) instead.
    this->polysets.push_back(hybrid->toPolySet());
#endif
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    this->polysets.push_back(mani->toPolySet());
#endif
  } else {
    assert(false && "unsupported geom in LegacyCGALRenderer");
  }
}

LegacyCGALRenderer::~LegacyCGALRenderer()
{
}

#ifdef ENABLE_CGAL
void LegacyCGALRenderer::createPolyhedrons()
{
  PRINTD("createPolyhedrons");
  this->polyhedrons.clear();
  for (const auto& N : this->nefPolyhedrons) {
    auto p = new CGAL_OGL_Polyhedron(*this->colorscheme);
    CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(*N->p3, p);
    // CGAL_NEF3_MARKED_FACET_COLOR <- CGAL_FACE_BACK_COLOR
    // CGAL_NEF3_UNMARKED_FACET_COLOR <- CGAL_FACE_FRONT_COLOR
    p->init();
    this->polyhedrons.push_back(std::shared_ptr<CGAL_OGL_Polyhedron>(p));
  }
  PRINTD("createPolyhedrons() end");
}
#endif

// Overridden from Renderer
void LegacyCGALRenderer::setColorScheme(const ColorScheme& cs)
{
  PRINTD("setColorScheme");
  Renderer::setColorScheme(cs);
  colormap[ColorMode::CGAL_FACE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_FACE_2D_COLOR);
  colormap[ColorMode::CGAL_EDGE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_2D_COLOR);
#ifdef ENABLE_CGAL
  this->polyhedrons.clear(); // Mark as dirty
#endif
  PRINTD("setColorScheme done");
}

void LegacyCGALRenderer::draw(bool showfaces, bool showedges, const shaderinfo_t * /*shaderinfo*/) const
{
  PRINTD("draw()");

  for (const auto& polyset : this->polysets) {
    PRINTD("draw() polyset");
    if (polyset->getDimension() == 2) {
      // Draw 2D polygons
      glDisable(GL_LIGHTING);
      setColor(ColorMode::CGAL_FACE_2D_COLOR);

      for (const auto& polygon : polyset->indices) {
	glBegin(GL_POLYGON);
	for (const auto& ind : polygon) {
	  Vector3d p=polyset->vertices[ind];
	  glVertex3d(p[0], p[1], 0);
	}
	glEnd();
      }

      // Draw 2D edges
      glDisable(GL_DEPTH_TEST);

      glLineWidth(2);
      setColor(ColorMode::CGAL_EDGE_2D_COLOR);
      this->render_edges(*polyset, CSGMODE_NONE);
      glEnable(GL_DEPTH_TEST);
    } else {
      // Draw 3D polygons
      setColor(ColorMode::MATERIAL);
      this->render_surface(*polyset, CSGMODE_NORMAL, Transform3d::Identity(), nullptr);
    }
  }

#ifdef ENABLE_CGAL
  for (const auto& p : this->getPolyhedrons()) {
    if (showfaces) p->set_style(SNC_BOUNDARY);
    else p->set_style(SNC_SKELETON);
    p->draw(showfaces && showedges);
  }
#endif

  PRINTD("draw() end");
}

BoundingBox LegacyCGALRenderer::getBoundingBox() const
{
  BoundingBox bbox;

#ifdef ENABLE_CGAL
  for (const auto& p : this->getPolyhedrons()) {
    CGAL::Bbox_3 cgalbbox = p->bbox();
    bbox.extend(BoundingBox(
                  Vector3d(cgalbbox.xmin(), cgalbbox.ymin(), cgalbbox.zmin()),
                  Vector3d(cgalbbox.xmax(), cgalbbox.ymax(), cgalbbox.zmax())));
  }
#endif
  for (const auto& ps : this->polysets) {
    bbox.extend(ps->getBoundingBox());
  }
  return bbox;
}

std::vector<SelectedObject> LegacyCGALRenderer::findModelObject(Vector3d near_pt, Vector3d far_pt,int mouse_x, int mouse_y, double tolerance) {
  std::vector<SelectedObject> results;
  double dist_near;
  double dist_nearest=NAN;
  Vector3d pt1_nearest;
  Vector3d pt2_nearest;
  for (const std::shared_ptr<const PolySet>& ps : this->polysets) {
    for(const Vector3d &pt: ps->vertices) {
      double dist_pt= calculateLinePointDistance(near_pt, far_pt, pt, dist_near);
      if(dist_pt < tolerance  ) {
        if(isnan(dist_nearest) || dist_near < dist_nearest)
        {
          dist_nearest=dist_near;
          pt1_nearest=pt;
        }	  
      }
    }
  }
  if(!isnan(dist_nearest)) {
    SelectedObject obj;
    obj.type = SELECTION_POINT;
    obj.p1=pt1_nearest;
    results.push_back(obj);
    return results;
  }
  for (const std::shared_ptr<const PolySet>& ps : this->polysets) {
    for(const auto &pol : ps->indices) {
	int n = pol.size();
        for(int i=0;i < n;i++ )
	{
	  int ind1=pol[i];
	  int ind2=pol[(i+1)%n];
	  double dist_lat;
          double dist_norm= fabs(calculateLineLineDistance(ps->vertices[ind1], ps->vertices[ind2], near_pt, far_pt,dist_lat));
          if(dist_lat >= 0 && dist_lat <= 1 && dist_norm < tolerance  ) {
	      dist_nearest=dist_lat;
	      pt1_nearest=ps->vertices[ind1];
	      pt2_nearest=ps->vertices[ind2];
	  }
        }	  
      }
   }

  if(!isnan(dist_nearest)) {
    SelectedObject obj;
    obj.type = SELECTION_LINE;
    obj.p1=pt1_nearest;
    obj.p2=pt2_nearest;
    results.push_back(obj);
    return results;
  }
  return results;
}
