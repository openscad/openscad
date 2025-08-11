#pragma once

#include "glview/VertexState.h"
#include "geometry/linalg.h"
#include "glview/Renderer.h"
#include "glview/system-gl.h"
#include <memory>
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "core/CSGNode.h"

#include "glview/VBORenderer.h"

#include <cstddef>
#include <string>
#include <vector>

class OpenCSGVertexState : public VertexState
{
public:
  OpenCSGVertexState(size_t csg_object_index = 0) : csg_object_index_(csg_object_index) {}
  OpenCSGVertexState(GLenum draw_mode, GLsizei draw_size, GLenum draw_type, size_t draw_offset,
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

class OpenCSGVertexStateFactory : public VertexStateFactory
{
public:
  OpenCSGVertexStateFactory() = default;

  [[nodiscard]] std::shared_ptr<VertexState> createVertexState(GLenum draw_mode, size_t draw_size,
                                                               GLenum draw_type, size_t draw_offset,
                                                               size_t element_offset,
                                                               GLuint vertices_vbo,
                                                               GLuint elements_vbo) const override
  {
    return std::make_shared<OpenCSGVertexState>(draw_mode, draw_size, draw_type, draw_offset,
                                                element_offset, vertices_vbo, elements_vbo);
  }
};

class OpenCSGVBOProduct : public VertexStateContainer
{
public:
  OpenCSGVBOProduct() = default;
  OpenCSGVBOProduct(const OpenCSGVBOProduct& o) = delete;
  OpenCSGVBOProduct(OpenCSGVBOProduct&& o) = delete;
  virtual ~OpenCSGVBOProduct() = default;

  [[nodiscard]] std::vector<OpenCSG::Primitive *>& primitives() { return primitives_; }

private:
  // primitives_ is used to create the OpenCSG depth buffer (unlit rendering).
  // states_ is used for color rendering (using GL_EQUAL).
  // Both may use the same underlying VBOs
  std::vector<OpenCSG::Primitive *> primitives_;
};

class OpenCSGRenderer : public VBORenderer
{
public:
  OpenCSGRenderer(std::shared_ptr<CSGProducts> root_products,
                  std::shared_ptr<CSGProducts> highlights_products,
                  std::shared_ptr<CSGProducts> background_products);
  ~OpenCSGRenderer() override = default;
  void prepare(const ShaderUtils::ShaderInfo *shaderinfo = nullptr) override;
  void draw(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo = nullptr) const override;

  BoundingBox getBoundingBox() const override;

private:
  void createCSGVBOProducts(const CSGProducts& products, bool highlight_mode, bool background_mode,
                            const ShaderUtils::ShaderInfo *shaderinfo);

  std::vector<std::unique_ptr<OpenCSGVBOProduct>> vertex_state_containers_;
  std::shared_ptr<CSGProducts> root_products_;
  std::shared_ptr<CSGProducts> highlights_products_;
  std::shared_ptr<CSGProducts> background_products_;
  std::string opencsg_vertex_shader_code_;
};
