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

#include "glview/preview/LegacyOpenCSGRenderer.h"

#include "core/enums.h"
#include "glview/system-gl.h"
#include "glview/LegacyRendererUtils.h"

#include <memory>
#include <memory.h>
#include <utility>
#include "geometry/PolySet.h"

#include <vector>

#ifdef ENABLE_OPENCSG

class OpenCSGPrim : public OpenCSG::Primitive
{
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity, const LegacyOpenCSGRenderer& renderer) :
    OpenCSG::Primitive(operation, convexity), renderer(renderer) { }
  std::shared_ptr<const PolySet> polyset;
  Transform3d m;
  Renderer::csgmode_e csgmode{Renderer::CSGMODE_NONE};

  // This is used by OpenCSG to render depth values
  void render() override {
    if (polyset) {
      glPushMatrix();
      glMultMatrixd(m.data());
      render_surface(*polyset, m);
      glPopMatrix();
    }
  }
private:
  const LegacyOpenCSGRenderer& renderer;
};

// Primitive for depth rendering using OpenCSG
std::unique_ptr<OpenCSGPrim> createCSGPrimitive(const CSGChainObject& csgobj, OpenCSG::Operation operation,
                                bool highlight_mode, bool background_mode, OpenSCADOperator type,
                                const LegacyOpenCSGRenderer &renderer) {
  auto prim = std::make_unique<OpenCSGPrim>(operation, csgobj.leaf->polyset->getConvexity(), renderer);
  prim->polyset = csgobj.leaf->polyset;
  prim->m = csgobj.leaf->matrix;
  if (prim->polyset->getDimension() == 2 && type == OpenSCADOperator::DIFFERENCE) {
    // Scale 2D negative objects 10% in the Z direction to avoid z fighting
    prim->m *= Eigen::Scaling(1.0, 1.0, 1.1);
  }
  prim->csgmode = Renderer::get_csgmode(highlight_mode, background_mode, type);
  return prim;
}

#endif // ENABLE_OPENCSG

LegacyOpenCSGRenderer::LegacyOpenCSGRenderer(std::shared_ptr<CSGProducts> root_products,
                                 std::shared_ptr<CSGProducts> highlights_products,
                                 std::shared_ptr<CSGProducts> background_products)
  : root_products_(std::move(root_products)),
  highlights_products_(std::move(highlights_products)),
  background_products_(std::move(background_products))
{
}

void LegacyOpenCSGRenderer::draw(bool /*showfaces*/, bool showedges, const shaderinfo_t *shaderinfo) const
{
  if (!shaderinfo && showedges) shaderinfo = &getShader();

  if (root_products_) {
    renderCSGProducts(root_products_, showedges, shaderinfo, false, false);
  }
  if (background_products_) {
    renderCSGProducts(background_products_, showedges, shaderinfo, false, true);
  }
  if (highlights_products_) {
    renderCSGProducts(highlights_products_, showedges, shaderinfo, true, false);
  }
}

void LegacyOpenCSGRenderer::renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges,
                                        const Renderer::shaderinfo_t *shaderinfo,
                                        bool highlight_mode, bool background_mode) const
{
#ifdef ENABLE_OPENCSG
  for (const auto& product : products->products) {
    // owned_primitives is only for memory management
    std::vector<std::unique_ptr<OpenCSG::Primitive>> owned_primitives;
    std::vector<OpenCSG::Primitive *> primitives;
    for (const auto& csgobj : product.intersections) {
      if (csgobj.leaf->polyset) {
        owned_primitives.push_back(createCSGPrimitive(csgobj, OpenCSG::Intersection, highlight_mode, background_mode, OpenSCADOperator::INTERSECTION, *this));
        primitives.push_back(owned_primitives.back().get());
      }
    }
    for (const auto& csgobj : product.subtractions) {
      if (csgobj.leaf->polyset) {
        owned_primitives.push_back(createCSGPrimitive(csgobj, OpenCSG::Subtraction, highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE, *this));
        primitives.push_back(owned_primitives.back().get());
      }
    }
    if (primitives.size() > 1) {
      OpenCSG::render(primitives);
      GL_CHECKD(glDepthFunc(GL_EQUAL));
    }

    if (shaderinfo && shaderinfo->progid) {
      if (shaderinfo->type != EDGE_RENDERING || (shaderinfo->type == EDGE_RENDERING && showedges)) {
        GL_CHECKD(glUseProgram(shaderinfo->progid));
      }
    }

    for (const auto& csgobj : product.intersections) {
      if (!csgobj.leaf->polyset) continue;

      if (shaderinfo && shaderinfo->type == Renderer::SELECT_RENDERING) {
        int identifier = csgobj.leaf->index;
        GL_CHECKD(glUniform3f(shaderinfo->data.select_rendering.identifier,
                              ((identifier >> 0) & 0xff) / 255.0f, ((identifier >> 8) & 0xff) / 255.0f,
                              ((identifier >> 16) & 0xff) / 255.0f));
      }

      const Color4f& c = csgobj.leaf->color;
      csgmode_e csgmode = get_csgmode(highlight_mode, background_mode);

      ColorMode colormode = ColorMode::NONE;
      if (highlight_mode) {
        colormode = ColorMode::HIGHLIGHT;
      } else if (background_mode) {
        colormode = ColorMode::BACKGROUND;
      } else {
        colormode = ColorMode::MATERIAL;
      }

      glPushMatrix();
      glMultMatrixd(csgobj.leaf->matrix.data());

      const Color4f color = setColor(colormode, c.data(), shaderinfo);
      if (color[3] == 1.0f) {
        // object is opaque, draw normally
        render_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, shaderinfo);
      } else {
        // object is transparent, so draw rear faces first.  Issue #1496
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        render_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, shaderinfo);
        glCullFace(GL_BACK);
        render_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, shaderinfo);
        glDisable(GL_CULL_FACE);
      }

      glPopMatrix();
    }
    for (const auto& csgobj : product.subtractions) {
      if (!csgobj.leaf->polyset) continue;

      const Color4f& c = csgobj.leaf->color;
      csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE);

      ColorMode colormode = ColorMode::NONE;
      if (highlight_mode) {
        colormode = ColorMode::HIGHLIGHT;
      } else if (background_mode) {
        colormode = ColorMode::BACKGROUND;
      } else {
        colormode = ColorMode::CUTOUT;
      }

      (void) setColor(colormode, c.data(), shaderinfo);
      glPushMatrix();
      Transform3d mat = csgobj.leaf->matrix;
      if (csgobj.leaf->polyset->getDimension() == 2) {
        // Scale 2D negative objects 10% in the Z direction to avoid z fighting
        mat *= Eigen::Scaling(1.0, 1.0, 1.1);
      }
      glMultMatrixd(mat.data());
      // negative objects should only render rear faces
      glEnable(GL_CULL_FACE);
      glCullFace(GL_FRONT);
      render_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, shaderinfo);
      glDisable(GL_CULL_FACE);

      glPopMatrix();
    }

    if (shaderinfo) glUseProgram(0);
    glDepthFunc(GL_LEQUAL);
  }
#endif // ENABLE_OPENCSG
}

BoundingBox LegacyOpenCSGRenderer::getBoundingBox() const
{
  BoundingBox bbox;
  if (root_products_) bbox = root_products_->getBoundingBox();
  if (highlights_products_) bbox.extend(highlights_products_->getBoundingBox());
  if (background_products_) bbox.extend(background_products_->getBoundingBox());

  return bbox;
}
