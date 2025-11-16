#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/CSGNode.h"
#include "core/enums.h"
#include "geometry/linalg.h"
#include "glview/system-gl.h"
#include "glview/VBORenderer.h"
#include "glview/VertexState.h"
#include "glview/ShaderUtils.h"
#include "glview/VBOBuilder.h"

class CSGProducts;
class CSGChainObject;

class TTRVertexState : public VertexState
{
public:
  TTRVertexState(size_t csg_object_index = 0) : csg_object_index_(csg_object_index) {}
  TTRVertexState(GLenum draw_mode, GLsizei draw_size, GLenum draw_type, size_t draw_offset,
                 size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo,
                 size_t csg_object_index = 0)
    : VertexState(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo,
                  elements_vbo),
      csg_object_index_(csg_object_index)
  {
  }

  [[nodiscard]] size_t csgObjectIndex() const { return csg_object_index_; }
  void setCsgObjectIndex(size_t csg_object_index) { csg_object_index_ = csg_object_index; }

private:
  size_t csg_object_index_;
};

class TTRVertexStateFactory : public VertexStateFactory
{
public:
  TTRVertexStateFactory() = default;

  [[nodiscard]] std::shared_ptr<VertexState> createVertexState(GLenum draw_mode, size_t draw_size,
                                                               GLenum draw_type, size_t draw_offset,
                                                               size_t element_offset,
                                                               GLuint vertices_vbo,
                                                               GLuint elements_vbo) const override
  {
    return std::make_shared<TTRVertexState>(draw_mode, draw_size, draw_type, draw_offset, element_offset,
                                            vertices_vbo, elements_vbo);
  }
};

class ThrownTogetherRenderer : public VBORenderer
{
public:
  ThrownTogetherRenderer(std::shared_ptr<CSGProducts> root_products,
                         std::shared_ptr<CSGProducts> highlight_products,
                         std::shared_ptr<CSGProducts> background_products);
  ~ThrownTogetherRenderer() override = default;
  void prepare(const ShaderUtils::ShaderInfo *shaderinfo = nullptr) override;
  void draw(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo = nullptr) const override;

  BoundingBox getBoundingBox() const override;

private:
  void renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges = false,
                         const ShaderUtils::ShaderInfo *shaderinfo = nullptr,
                         bool highlight_mode = false, bool background_mode = false,
                         bool fberror = false) const;

  void createCSGProducts(const CSGProducts& products, VertexStateContainer& container,
                         VBOBuilder& vbo_builder, bool highlight_mode, bool background_mode,
                         const ShaderUtils::ShaderInfo *shaderinfo);
  void createChainObject(VertexStateContainer& container, VBOBuilder& vbo_builder,
                         const CSGChainObject& csgobj, bool highlight_mode, bool background_mode,
                         OpenSCADOperator type, const ShaderUtils::ShaderInfo *shaderinfo);

  std::shared_ptr<CSGProducts> root_products_;
  std::shared_ptr<CSGProducts> highlight_products_;
  std::shared_ptr<CSGProducts> background_products_;
  std::vector<VertexStateContainer> vertex_state_containers_;
};
