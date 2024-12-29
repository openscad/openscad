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

#include "glview/preview/LegacyThrownTogetherRenderer.h"

#include <memory>
#include <utility>
#include "Feature.h"
#include "geometry/PolySet.h"
#include "core/enums.h"
#include "utils/printutils.h"
#include "glview/LegacyRendererUtils.h"

#include "glview/system-gl.h"

LegacyThrownTogetherRenderer::LegacyThrownTogetherRenderer(std::shared_ptr<CSGProducts> root_products,
                                               std::shared_ptr<CSGProducts> highlight_products,
                                               std::shared_ptr<CSGProducts> background_products)
  : root_products(std::move(root_products)), highlight_products(std::move(highlight_products)), background_products(std::move(background_products))
{
}

void LegacyThrownTogetherRenderer::draw(bool /*showfaces*/, bool showedges, const Renderer::shaderinfo_t *shaderinfo) const
{
  PRINTD("draw()");
  if (!shaderinfo && showedges) {
    shaderinfo = &getShader();
  }
  if (shaderinfo && shaderinfo->progid) {
    glUseProgram(shaderinfo->progid);
  }

  if (this->root_products) {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    renderCSGProducts(this->root_products, showedges, shaderinfo, false, false, false);
    glCullFace(GL_FRONT);
    glColor3ub(255, 0, 255);
    renderCSGProducts(this->root_products, showedges, shaderinfo, false, false, true);
    glDisable(GL_CULL_FACE);
  }
  if (this->background_products) renderCSGProducts(this->background_products, showedges, shaderinfo, false, true, false);
  if (this->highlight_products) renderCSGProducts(this->highlight_products, showedges, shaderinfo, true, false, false);
  if (shaderinfo && shaderinfo->progid) {
    glUseProgram(0);
  }
}

void LegacyThrownTogetherRenderer::renderChainObject(const CSGChainObject& csgobj, bool showedges,
                                               const Renderer::shaderinfo_t *shaderinfo,
                                               bool highlight_mode, bool background_mode,
                                               bool fberror, OpenSCADOperator type) const
{
  if (!csgobj.leaf->polyset) return;
  if (this->geomVisitMark[std::make_pair(csgobj.leaf->polyset.get(), &csgobj.leaf->matrix)]++ > 0) return;

  const Color4f& c = csgobj.leaf->color;
  csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, type);
  ColorMode colormode = ColorMode::NONE;
  ColorMode edge_colormode = ColorMode::NONE;

  colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, fberror, type);
  const Transform3d& m = csgobj.leaf->matrix;

  if (shaderinfo && shaderinfo->type == Renderer::SELECT_RENDERING) {
    int identifier = csgobj.leaf->index;
    glUniform3f(shaderinfo->data.select_rendering.identifier, ((identifier >> 0) & 0xff) / 255.0f,
                ((identifier >> 8) & 0xff) / 255.0f, ((identifier >> 16) & 0xff) / 255.0f);
  } else {
    setColor(colormode, c.data(), shaderinfo);
  }
  glPushMatrix();

  Transform3d tmp = csgobj.leaf->matrix;
  if (csgobj.leaf->polyset->getDimension() == 2 && type == OpenSCADOperator::DIFFERENCE) {
    // Scale 2D negative objects 10% in the Z direction to avoid z fighting
    tmp *= Eigen::Scaling(1.0, 1.0, 1.1);
  }
  glMultMatrixd(tmp.data());
  render_surface(*csgobj.leaf->polyset, tmp, shaderinfo);
  glPopMatrix();
}

void LegacyThrownTogetherRenderer::renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges,
                                               const Renderer::shaderinfo_t *shaderinfo,
                                               bool highlight_mode, bool background_mode,
                                               bool fberror) const
{
  PRINTD("Thrown renderCSGProducts");
  glDepthFunc(GL_LEQUAL);
  this->geomVisitMark.clear();

  for (const auto& product : products->products) {
    for (const auto& csgobj : product.intersections) {
      renderChainObject(csgobj, showedges, shaderinfo, highlight_mode, background_mode, fberror, OpenSCADOperator::INTERSECTION);
    }
    for (const auto& csgobj : product.subtractions) {
      renderChainObject(csgobj, showedges, shaderinfo, highlight_mode, background_mode, fberror, OpenSCADOperator::DIFFERENCE);
    }
  }
}

Renderer::ColorMode LegacyThrownTogetherRenderer::getColorMode(const CSGNode::Flag& flags, bool highlight_mode,
                                                         bool background_mode, bool fberror, OpenSCADOperator type) const
{
  ColorMode colormode = ColorMode::NONE;

  if (highlight_mode) {
    colormode = ColorMode::HIGHLIGHT;
  } else if (background_mode) {
    if (flags & CSGNode::FLAG_HIGHLIGHT) {
      colormode = ColorMode::HIGHLIGHT;
    } else {
      colormode = ColorMode::BACKGROUND;
    }
  } else if (fberror) {
  } else if (type == OpenSCADOperator::DIFFERENCE) {
    if (flags & CSGNode::FLAG_HIGHLIGHT) {
      colormode = ColorMode::HIGHLIGHT;
    } else {
      colormode = ColorMode::CUTOUT;
    }
  } else {
    if (flags & CSGNode::FLAG_HIGHLIGHT) {
      colormode = ColorMode::HIGHLIGHT;
    } else {
      colormode = ColorMode::MATERIAL;
    }
  }
  return colormode;
}

BoundingBox LegacyThrownTogetherRenderer::getBoundingBox() const
{
  BoundingBox bbox;
  if (this->root_products) bbox = this->root_products->getBoundingBox(true);
  if (this->highlight_products) bbox.extend(this->highlight_products->getBoundingBox(true));
  if (this->background_products) bbox.extend(this->background_products->getBoundingBox(true));
  return bbox;
}
