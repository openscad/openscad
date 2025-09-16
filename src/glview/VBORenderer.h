#pragma once

#include <array>
#include <utility>
#include <memory>
#include <cstddef>
#include "glview/Renderer.h"
#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
#include "glview/system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "core/CSGNode.h"
#include "glview/VBOBuilder.h"
#include <unordered_map>
#include <boost/functional/hash.hpp>

namespace VBOUtils {

void shader_attribs_enable(const ShaderUtils::ShaderInfo& shaderinfo);
void shader_attribs_disable(const ShaderUtils::ShaderInfo& shaderinfo);

}  // namespace VBOUtils

class VBOShaderVertexState : public VertexState
{
public:
  VBOShaderVertexState(size_t draw_offset, size_t element_offset, GLuint vertices_vbo,
                       GLuint elements_vbo)
    : VertexState(0, 0, 0, draw_offset, element_offset, vertices_vbo, elements_vbo)
  {
  }
};

class VBORenderer : public Renderer
{
public:
  VBORenderer();
  virtual size_t calcNumVertices(const std::shared_ptr<CSGProducts>& products,
                                 bool unique_geometry = false) const;
  virtual size_t calcNumVertices(const CSGChainObject& csgobj, bool unique_geometry = false) const;
  virtual size_t calcNumVertices(const PolySet& polyset) const;
  virtual size_t calcNumEdgeVertices(const PolySet& polyset) const;
  virtual size_t calcNumEdgeVertices(const Polygon2d& polygon) const;

  void add_shader_pointers(
    VBOBuilder& vbo_builder,
    const ShaderUtils::ShaderInfo
      *shaderinfo);  // This could stay protected, were it not for VertexStateManager

protected:
  void add_shader_data(VBOBuilder& vbo_builder);
  void shader_attribs_enable(const ShaderUtils::ShaderInfo&) const;
  void shader_attribs_disable(const ShaderUtils::ShaderInfo&) const;

  mutable std::unordered_map<std::pair<const PolySet *, const Transform3d *>, int,
                             boost::hash<std::pair<const PolySet *, const Transform3d *>>>
    geom_visit_mark_;

private:
};
