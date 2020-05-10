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

#include "ThrownTogetherRenderer.h"
#include "polyset.h"
#include "printutils.h"

#include "system-gl.h"


//
// VBORenderer ThrownTogetherRenderer
//
#ifdef ENABLE_EXPERIMENTAL
ThrownTogetherRenderer::ThrownTogetherRenderer(shared_ptr<CSGProducts> root_products,
																							 shared_ptr<CSGProducts> highlight_products,
																							 shared_ptr<CSGProducts> background_products)
	: product_vertex_sets(new std::vector<VertexSets *>()),
    root_products(root_products), highlight_products(highlight_products), background_products(background_products)
{
}

ThrownTogetherRenderer::~ThrownTogetherRenderer()
{
  if (product_vertex_sets) {
    for (auto &vertex_sets : *product_vertex_sets) {
			glDeleteBuffers(1,&vertex_sets->first);
			for (auto &vertex_set : *vertex_sets->second) {
					delete vertex_set;
			}
			vertex_sets->second->clear();
			delete vertex_sets;
	  }
	  product_vertex_sets->clear();
	  delete product_vertex_sets;
  }
}

void ThrownTogetherRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	PRINTD("Thrown draw");
  if (!getProductVertexSets().size()) {
    if (this->root_products)
  		renderCSGProducts(*this->root_products, false, false, showedges);
  	if (this->background_products)
  	 	renderCSGProducts(*this->background_products, false, true, showedges);
  	if (this->highlight_products)
  	 	renderCSGProducts(*this->highlight_products, true, false, showedges);
	}

	drawCSGProducts(showedges);
}

Renderer::ColorMode ThrownTogetherRenderer::getColorMode(const CSGNode::Flag &flags, bool highlight_mode,
																							           bool background_mode, bool fberror, OpenSCADOperator type) const
{
	ColorMode colormode = ColorMode::NONE;

	if (highlight_mode) {
		colormode = ColorMode::HIGHLIGHT;
	} else if (background_mode) {
		if (flags & CSGNode::FLAG_HIGHLIGHT) {
			colormode = ColorMode::HIGHLIGHT;
		}
		else {
			colormode = ColorMode::BACKGROUND;
		}
	} else if (fberror) {
	} else if (type == OpenSCADOperator::DIFFERENCE) {
		if (flags & CSGNode::FLAG_HIGHLIGHT) {
			colormode = ColorMode::HIGHLIGHT;
		}
		else {
			colormode = ColorMode::CUTOUT;
		}
	} else {
		if (flags & CSGNode::FLAG_HIGHLIGHT) {
			colormode = ColorMode::HIGHLIGHT;
		}
		else {
			colormode = ColorMode::MATERIAL;
		}
	}
  return colormode;
}

void ThrownTogetherRenderer::createChainObject(std::vector<VertexSet *> &vertex_sets, std::vector<Vertex> &render_buffer,
                                               const class CSGChainObject &csgobj, bool highlight_mode,
                                               bool background_mode, bool fberror, OpenSCADOperator type) const
{
  VertexSet *prev = 0;
	VertexSet *vertex_set = 0;
  Color4f color;

  if (csgobj.leaf->geom) {
    if (this->geomVisitMark[std::make_pair(csgobj.leaf->geom.get(), &csgobj.leaf->matrix)]++ > 0) return;

    prev = (vertex_sets.empty() ? 0 : vertex_sets.back());
    vertex_set = new VertexSet({false, type, csgobj.leaf->geom->getConvexity(), 0, 0, false, false});
    GLintptr prev_start_offset = 0;
    GLsizei prev_draw_size = 0;
    if (prev) {
      prev_start_offset = prev->start_offset;
      prev_draw_size = prev->draw_size;
    }

    Color4f &leaf_color = csgobj.leaf->color;
    csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, type);
    const ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, fberror, type);
    getShaderColor(colormode, leaf_color, color);

    if (highlight_mode || background_mode) {
      vertex_set->draw_cull_front = false;
      vertex_set->draw_cull_back = false;
      const_cast<ThrownTogetherRenderer *>(this)->create_surface(csgobj.leaf->geom,
          render_buffer, *vertex_set, prev_start_offset, prev_draw_size,
          csgmode, csgobj.leaf->matrix, color);
    } else { // root mode
      vertex_set->draw_cull_front = false;
      vertex_set->draw_cull_back = true;

      const_cast<ThrownTogetherRenderer *>(this)->create_surface(csgobj.leaf->geom,
          render_buffer, *vertex_set, prev_start_offset, prev_draw_size,
          csgmode, csgobj.leaf->matrix, color);
      vertex_sets.push_back(vertex_set);

      prev = (vertex_sets.empty() ? 0 : vertex_sets.back());
      vertex_set = new VertexSet({false, type, csgobj.leaf->geom->getConvexity(), 0, 0, false, false});
      if (prev) {
        prev_start_offset = prev->start_offset;
        prev_draw_size = prev->draw_size;
      }

      color[0] = 1.0; color[1] = 0.0; color[2] = 1.0; // override leaf color on front/back error
      vertex_set->draw_cull_front = true;
      vertex_set->draw_cull_back = false;
      const_cast<ThrownTogetherRenderer *>(this)->create_surface(csgobj.leaf->geom,
          render_buffer, *vertex_set, prev_start_offset, prev_draw_size,
          csgmode, csgobj.leaf->matrix, color);
    }
    vertex_sets.push_back(vertex_set);
  }
}

void ThrownTogetherRenderer::renderCSGProducts(const CSGProducts &products, bool highlight_mode,
																							 bool background_mode, bool /*showedges*/,
																							 bool /*fberror*/) const
{
	PRINTD("Thrown renderCSGProducts");
	this->geomVisitMark.clear();

  std::vector<Vertex> *render_buffer = new std::vector<Vertex>();
	std::vector<VertexSet *> *vertex_sets = 0;
	GLuint vbo;

  if (render_buffer) {
    for(const auto &product : products.products) {
      if (product.intersections.size() || product.subtractions.size()) {
			  vertex_sets = new std::vector<VertexSet *>();
        if (vertex_sets) {
          for(const auto &csgobj : product.intersections) {
            createChainObject(*vertex_sets, *render_buffer, csgobj, highlight_mode, background_mode, false, OpenSCADOperator::INTERSECTION);
          }
          for(const auto &csgobj : product.subtractions) {
            createChainObject(*vertex_sets, *render_buffer, csgobj, highlight_mode, background_mode, false, OpenSCADOperator::DIFFERENCE);
          }
          if (render_buffer->size()) {
				    glGenBuffers(1, &vbo);

				    product_vertex_sets->push_back(new VertexSets(vbo,vertex_sets));

				    glBindBuffer(GL_ARRAY_BUFFER, vbo);
				    glBufferData(GL_ARRAY_BUFFER, render_buffer->size()*sizeof(Vertex), render_buffer->data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
				    render_buffer->clear();
          }
        }
		  }
    }
	}
	delete render_buffer;
}

void ThrownTogetherRenderer::drawCSGProducts(bool use_edge_shader) const
{
  glDepthFunc(GL_LEQUAL);

	for(const auto &vertex_sets : *product_vertex_sets) {
		if (vertex_sets) {
			if (use_edge_shader) glUseProgram(getVBOShaderSettings()[0]);

			glBindBuffer(GL_ARRAY_BUFFER, vertex_sets->first);
			for(const auto &vertex_set : *vertex_sets->second) {
				if (!vertex_set->draw_cull_front && !vertex_set->draw_cull_back) {
					draw_surface(*vertex_set, true, use_edge_shader);
				}
				if (vertex_set->draw_cull_front) {
					glEnable(GL_CULL_FACE); // cull face
					glCullFace(GL_FRONT);
					draw_surface(*vertex_set, true, use_edge_shader);
					glDisable(GL_CULL_FACE);
				}
				if (vertex_set->draw_cull_back) {
					glEnable(GL_CULL_FACE); // cull face
					glCullFace(GL_BACK);
					draw_surface(*vertex_set, true, use_edge_shader);
					glDisable(GL_CULL_FACE);
				}
			}

			glBindBuffer(GL_ARRAY_BUFFER,0);

			if (use_edge_shader) glUseProgram(0);
		}
	}
}

#else
//
// Original Renderer ThrownTogetherRenderer code
//
ThrownTogetherRenderer::ThrownTogetherRenderer(shared_ptr<CSGProducts> root_products,
																							 shared_ptr<CSGProducts> highlight_products,
																							 shared_ptr<CSGProducts> background_products)
	: root_products(root_products), highlight_products(highlight_products), background_products(background_products)
{
}

ThrownTogetherRenderer::~ThrownTogetherRenderer()
{
}

void ThrownTogetherRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	PRINTD("Thrown draw");
 	if (this->root_products) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		renderCSGProducts(*this->root_products, false, false, showedges, false);
		glCullFace(GL_FRONT);
		glColor3ub(255, 0, 255);
		renderCSGProducts(*this->root_products, false, false, showedges, true);
		glDisable(GL_CULL_FACE);
	}
	if (this->background_products)
	 	renderCSGProducts(*this->background_products, false, true, showedges, false);
	if (this->highlight_products)
	 	renderCSGProducts(*this->highlight_products, true, false, showedges, false);
}

void ThrownTogetherRenderer::renderChainObject(const CSGChainObject &csgobj, bool highlight_mode,
																							 bool background_mode, bool showedges, bool fberror, OpenSCADOperator type) const
{
	if (this->geomVisitMark[std::make_pair(csgobj.leaf->geom.get(), &csgobj.leaf->matrix)]++ > 0) return;
	const Color4f &c = csgobj.leaf->color;
	csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, type);
	ColorMode colormode = ColorMode::NONE;
	ColorMode edge_colormode = ColorMode::NONE;
	
	if (highlight_mode) {
		colormode = ColorMode::HIGHLIGHT;
		edge_colormode = ColorMode::HIGHLIGHT_EDGES;
	} else if (background_mode) {
		if (csgobj.flags & CSGNode::FLAG_HIGHLIGHT) {
			colormode = ColorMode::HIGHLIGHT;
		}
		else {
			colormode = ColorMode::BACKGROUND;
		}
		edge_colormode = ColorMode::BACKGROUND_EDGES;
	} else if (fberror) {
	} else if (type == OpenSCADOperator::DIFFERENCE) {
		if (csgobj.flags & CSGNode::FLAG_HIGHLIGHT) {
			colormode = ColorMode::HIGHLIGHT;
		}
		else {
			colormode = ColorMode::CUTOUT;
		}
		edge_colormode = ColorMode::CUTOUT_EDGES;
	} else {
		if (csgobj.flags & CSGNode::FLAG_HIGHLIGHT) {
			colormode = ColorMode::HIGHLIGHT;
		}
		else {
			colormode = ColorMode::MATERIAL;
		}
		edge_colormode = ColorMode::MATERIAL_EDGES;
	}
	
	const Transform3d &m = csgobj.leaf->matrix;
	setColor(colormode, c.data());
	glPushMatrix();
	glMultMatrixd(m.data());
	render_surface(csgobj.leaf->geom, csgmode, m);
	if (showedges) {
		// FIXME? glColor4f((c[0]+1)/2, (c[1]+1)/2, (c[2]+1)/2, 1.0);
		setColor(edge_colormode);
		render_edges(csgobj.leaf->geom, csgmode);
	}
	glPopMatrix();
	
}

void ThrownTogetherRenderer::renderCSGProducts(const CSGProducts &products, bool highlight_mode,
																							 bool background_mode, bool showedges, 
																							 bool fberror) const
{
	PRINTD("Thrown renderCSGProducts");
	glDepthFunc(GL_LEQUAL);
	this->geomVisitMark.clear();

	for(const auto &product : products.products) {
		for(const auto &csgobj : product.intersections) {
			renderChainObject(csgobj, highlight_mode, background_mode, showedges, fberror, OpenSCADOperator::INTERSECTION);
		}
		for(const auto &csgobj : product.subtractions) {
			renderChainObject(csgobj, highlight_mode, background_mode, showedges, fberror, OpenSCADOperator::DIFFERENCE);
		}
	}
}
#endif // ENABLE_EXPERIMENTAL

BoundingBox ThrownTogetherRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (this->root_products) bbox = this->root_products->getBoundingBox();
	if (this->highlight_products) bbox.extend(this->highlight_products->getBoundingBox());
	if (this->background_products) bbox.extend(this->background_products->getBoundingBox());
	return bbox;
}
