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

#include "system-gl.h"
#include "OpenCSGRenderer.h"
#include "polyset.h"
#include "csgterm.h"
#include "stl-utils.h"
#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

class OpenCSGPrim : public OpenCSG::Primitive
{
public:
	OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity) :
			OpenCSG::Primitive(operation, convexity) { }
	shared_ptr<const Geometry> geom;
	Transform3d m;
	Renderer::csgmode_e csgmode;
	virtual void render() {
		glPushMatrix();
		glMultMatrixd(m.data());
		Renderer::render_surface(geom, csgmode, m);
		glPopMatrix();
	}
};

OpenCSGRenderer::OpenCSGRenderer(CSGChain *root_chain, CSGChain *highlights_chain,
																 CSGChain *background_chain, GLint *shaderinfo)
	:   root_chain(root_chain),
            highlights_chain(highlights_chain), 
            background_chain(background_chain),
            clippingPlane(0),
            shaderinfo(shaderinfo)
{
}

void OpenCSGRenderer::draw(
    bool         /*showfaces*/,
    bool         showedges,
    const double *_clippingPlane
) const
{
        clippingPlane = _clippingPlane;

	GLint *shaderinfo = this->shaderinfo;
	if (!shaderinfo[0]) shaderinfo = NULL;
	if (this->root_chain) {
		renderCSGChain(this->root_chain, showedges ? shaderinfo : NULL, false, false);
	}
	if (this->background_chain) {
		renderCSGChain(this->background_chain, showedges ? shaderinfo : NULL, false, true);
	}
	if (this->highlights_chain) {
		renderCSGChain(this->highlights_chain, showedges ? shaderinfo : NULL, true, false);
	}
}

static Geometry *makeClipCube() {

    double x1 = -1.0;
    double y1 = -1.0;
    double z1 = -1.0;

    double x2 =  1.0;
    double y2 =  1.0;
    double z2 =  1.0;

    PolySet *p = new PolySet(3, true);

    // top
    p->append_poly();
    p->append_vertex(x1, y1, z2);
    p->append_vertex(x2, y1, z2);
    p->append_vertex(x2, y2, z2);
    p->append_vertex(x1, y2, z2);

    // bottom
    p->append_poly();
    p->append_vertex(x1, y2, z1);
    p->append_vertex(x2, y2, z1);
    p->append_vertex(x2, y1, z1);
    p->append_vertex(x1, y1, z1);

    // side1
    p->append_poly();
    p->append_vertex(x1, y1, z1);
    p->append_vertex(x2, y1, z1);
    p->append_vertex(x2, y1, z2);
    p->append_vertex(x1, y1, z2);

    // side2
    p->append_poly();
    p->append_vertex(x2, y1, z1);
    p->append_vertex(x2, y2, z1);
    p->append_vertex(x2, y2, z2);
    p->append_vertex(x2, y1, z2);

    // side3
    p->append_poly();
    p->append_vertex(x2, y2, z1);
    p->append_vertex(x1, y2, z1);
    p->append_vertex(x1, y2, z2);
    p->append_vertex(x2, y2, z2);

    // side4
    p->append_poly();
    p->append_vertex(x1, y2, z1);
    p->append_vertex(x1, y1, z1);
    p->append_vertex(x1, y1, z2);
    p->append_vertex(x1, y2, z2);

    return p;
}

static boost::shared_ptr<const Geometry> kClipCubeGeometry(makeClipCube());

void OpenCSGRenderer::renderCSGChain(CSGChain *chain, GLint *shaderinfo, 
																		 bool highlight, bool background) const
{
	std::vector<OpenCSG::Primitive*> primitives;
	size_t j = 0;
	for (size_t i = 0;; i++) {
		bool last = i == chain->objects.size();
		const CSGChainObject &i_obj = last ? chain->objects[i-1] : chain->objects[i];
		if (last || i_obj.type == CSGTerm::TYPE_UNION) {

                        OpenCSGPrim *clipCube = 0;
                        if(0<primitives.size() && 0!=clippingPlane) {

                            Transform3d s = Transform3d::Identity();
                            s.scale(Vector3d(50.0, 100.0 , 30.0));

                            double clipZ = clippingPlane[3];
                            Transform3d t = Transform3d::Identity();
                            t.translate(Vector3d(clipZ-50.0, 0.0 , 0.0));

                            clipCube = new OpenCSGPrim(OpenCSG::Subtraction, 2);
                            clipCube->m = (t * s);
                            clipCube->geom = kClipCubeGeometry;
                            clipCube->csgmode = csgmode_e(CSGMODE_NORMAL | CSGMODE_DIFFERENCE);
                            primitives.push_back(clipCube);
                        }

			if (j+1!=i || 0!=clippingPlane) {
                            OpenCSG::render(primitives);
                            glDepthFunc(GL_EQUAL);
			}

			if (shaderinfo) {
                            glUseProgram(shaderinfo[0]);
                        }

			for (; j < i; j++) {
				const CSGChainObject &j_obj = chain->objects[j];
				const Color4f &c = j_obj.color;
				glPushMatrix();
				glMultMatrixd(j_obj.matrix.data());
				csgmode_e csgmode = csgmode_e(
					(highlight ? 
					 CSGMODE_HIGHLIGHT :
					 (background ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)) |
					(j_obj.type == CSGTerm::TYPE_DIFFERENCE ? CSGMODE_DIFFERENCE : 0));

				ColorMode colormode = COLORMODE_NONE;
				if (background) {
					if (j_obj.flag & CSGTerm::FLAG_HIGHLIGHT) {
						colormode = COLORMODE_HIGHLIGHT;
					}
					else {
						colormode = COLORMODE_BACKGROUND;
					}
				} else if (j_obj.type == CSGTerm::TYPE_DIFFERENCE) {
					if (j_obj.flag & CSGTerm::FLAG_HIGHLIGHT) {
						colormode = COLORMODE_HIGHLIGHT;
					}
					else {
						colormode = COLORMODE_CUTOUT;
					}
				} else {
					if (j_obj.flag & CSGTerm::FLAG_HIGHLIGHT) {
						colormode = COLORMODE_HIGHLIGHT;
					 }
					else {
						colormode = COLORMODE_MATERIAL;
					}
				}

				setColor(colormode, c.data(), shaderinfo);

				render_surface(j_obj.geom, csgmode, j_obj.matrix, shaderinfo);
				glPopMatrix();
			}

                        if(0!=clipCube) {
                            glDepthFunc(GL_EQUAL);
                            glPushMatrix();
                                Color4f clip(0x7F, 0x7F, 0xFF, 0xFF);
                                setColor(COLORMODE_CUTOUT, clip.data(), shaderinfo);
                                glMultMatrixd(clipCube->m.data());
                                render_surface(
                                    clipCube->geom,
                                    clipCube->csgmode,
                                    clipCube->m,
                                    shaderinfo
                                );
                            glPopMatrix();
                        }

			if (shaderinfo) glUseProgram(0);
			for (unsigned int k = 0; k < primitives.size(); k++) {
				delete primitives[k];
			}
			glDepthFunc(GL_LEQUAL);
			primitives.clear();
		}

		if (last) break;

		if (i_obj.geom) {
			OpenCSGPrim *prim = new OpenCSGPrim(i_obj.type == CSGTerm::TYPE_DIFFERENCE ?
																					OpenCSG::Subtraction : OpenCSG::Intersection, i_obj.geom->getConvexity());
			
			prim->geom = i_obj.geom;
			prim->m = i_obj.matrix;
			prim->csgmode = csgmode_e(
				(highlight ? 
				 CSGMODE_HIGHLIGHT :
				 (background ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)) |
				(i_obj.type == CSGTerm::TYPE_DIFFERENCE ? CSGMODE_DIFFERENCE : 0));

			primitives.push_back(prim);
		}
	}
	std::for_each(primitives.begin(), primitives.end(), del_fun<OpenCSG::Primitive>());
}

BoundingBox OpenCSGRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (this->root_chain) bbox = this->root_chain->getBoundingBox();
	if (this->background_chain) bbox.extend(this->background_chain->getBoundingBox());

	return bbox;
}
