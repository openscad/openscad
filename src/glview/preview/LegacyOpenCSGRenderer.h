#pragma once

#include "glview/Renderer.h"
#include "glview/system-gl.h"
#include <memory>
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "core/CSGNode.h"

#include "glview/VBORenderer.h"

class CSGChainObject;
class CSGProducts;
class OpenCSGPrim;

class LegacyOpenCSGRenderer : public Renderer
{
public:
  LegacyOpenCSGRenderer(std::shared_ptr<CSGProducts> root_products,
                  std::shared_ptr<CSGProducts> highlights_products,
                  std::shared_ptr<CSGProducts> background_products);
  ~LegacyOpenCSGRenderer() override = default;
  void draw(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) const override;

  BoundingBox getBoundingBox() const override;
private:
  void renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges = false, const Renderer::shaderinfo_t *shaderinfo = nullptr,
                         bool highlight_mode = false, bool background_mode = false) const;

private:
  std::shared_ptr<CSGProducts> root_products_;
  std::shared_ptr<CSGProducts> highlights_products_;
  std::shared_ptr<CSGProducts> background_products_;
};
