#pragma once

#include "glview/VertexState.h"
#include "glview/ShaderUtils.h"
#include "geometry/linalg.h"
#include "glview/Renderer.h"
#include "glview/system-gl.h"
#include <utility>
#include <memory>
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "core/CSGNode.h"

#include "glview/VBORenderer.h"

#include <cstddef>
#include <string>
#include <vector>

class OpenCSGVBOProduct : public VertexStateContainer
{
public:
  OpenCSGVBOProduct() = default;
  OpenCSGVBOProduct(const OpenCSGVBOProduct& o) = delete;
  OpenCSGVBOProduct(OpenCSGVBOProduct&& o) = delete;
  virtual ~OpenCSGVBOProduct() = default;

  void addPrimitive(std::unique_ptr<OpenCSG::Primitive> p)
  {
    primitives_.push_back(p.get());
    owned_primitives_.push_back(std::move(p));
  }

  [[nodiscard]] const std::vector<OpenCSG::Primitive *>& primitives() const { return primitives_; }

private:
  // primitives_ is used to create the OpenCSG depth buffer (unlit rendering).
  // states_ is used for color rendering (using GL_EQUAL).
  // Both may use the same underlying VBOs
  std::vector<OpenCSG::Primitive *> primitives_;
  std::vector<std::unique_ptr<OpenCSG::Primitive>> owned_primitives_;
};

class OpenCSGRenderer : public VBORenderer
{
public:
  OpenCSGRenderer(std::shared_ptr<CSGProducts> root_products,
                  std::shared_ptr<CSGProducts> highlights_products,
                  std::shared_ptr<CSGProducts> background_products);
  ~OpenCSGRenderer() override = default;
  void prepare(const ShaderUtils::Shader *shader) override;
  void draw(bool showedges, const ShaderUtils::Shader *shader) const override;

  BoundingBox getBoundingBox() const override;

private:
  void createCSGVBOProducts(const CSGProducts& products, bool highlight_mode, bool background_mode,
                            const ShaderUtils::Shader *shader);

  std::vector<std::unique_ptr<OpenCSGVBOProduct>> vertex_state_containers_;
  std::shared_ptr<CSGProducts> root_products_;
  std::shared_ptr<CSGProducts> highlights_products_;
  std::shared_ptr<CSGProducts> background_products_;
  std::string opencsg_vertex_shader_code_;
};
