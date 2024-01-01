#pragma once

#include "Renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "CSGNode.h"

#include "VBORenderer.h"

class CSGChainObject;
class CSGProducts;
class OpenCSGPrim;

class LegacyOpenCSGRenderer : public VBORenderer
{
public:
  LegacyOpenCSGRenderer(std::shared_ptr<CSGProducts> root_products,
                  std::shared_ptr<CSGProducts> highlights_products,
                  std::shared_ptr<CSGProducts> background_products);
  ~LegacyOpenCSGRenderer() override {
  }
  void draw(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) const override;

  BoundingBox getBoundingBox() const override;
private:
#ifdef ENABLE_OPENCSG
  OpenCSGPrim *createCSGPrimitive(const CSGChainObject& csgobj, OpenCSG::Operation operation, bool highlight_mode, bool background_mode, OpenSCADOperator type) const;
#endif // ENABLE_OPENCSG
  void renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges = false, const Renderer::shaderinfo_t *shaderinfo = nullptr,
                         bool highlight_mode = false, bool background_mode = false) const;

private:
  std::shared_ptr<CSGProducts> root_products_;
  std::shared_ptr<CSGProducts> highlights_products_;
  std::shared_ptr<CSGProducts> background_products_;
};
