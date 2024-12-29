#pragma once

#include <array>
#include <utility>
#include <memory>
#include <cstddef>
#include "glview/Renderer.h"
#include "glview/system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "core/CSGNode.h"
#include "glview/VertexArray.h"
#include <unordered_map>
#include <boost/functional/hash.hpp>


class VBOShaderVertexState : public VertexState
{
public:
  VBOShaderVertexState(size_t draw_offset, size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo)
    : VertexState(0, 0, 0, draw_offset, element_offset, vertices_vbo, elements_vbo) {}
};

class VBORenderer : public Renderer
{
public:
  VBORenderer();
  void resize(int w, int h) override;
  virtual bool getShaderColor(Renderer::ColorMode colormode, const Color4f& col, Color4f& outcolor) const;
  virtual size_t getSurfaceBufferSize(const std::shared_ptr<CSGProducts>& products, bool unique_geometry = false) const;
  virtual size_t getSurfaceBufferSize(const CSGChainObject& csgobj, bool unique_geometry = false) const;
  virtual size_t getSurfaceBufferSize(const PolySet& polyset) const;
  virtual size_t getEdgeBufferSize(const PolySet& polyset) const;
  virtual size_t getEdgeBufferSize(const Polygon2d& polygon) const;

  virtual void create_surface(const PolySet& ps, VertexArray& vertex_array,
                              csgmode_e csgmode, const Transform3d& m,
                              const Color4f& default_color, bool force_default_color = false) const;

  virtual void create_edges(const Polygon2d& polygon, VertexArray& vertex_array,
                            const Transform3d& m, const Color4f& color) const;

  virtual void create_polygons(const PolySet& ps, VertexArray& vertex_array,
                               const Transform3d& m, const Color4f& color) const;

  virtual void create_triangle(VertexArray& vertex_array, const Color4f& color,
                               const Vector3d& p0, const Vector3d& p1, const Vector3d& p2,
                               size_t primitive_index = 0, size_t shape_size = 0,
                               bool outlines = false, bool mirror = false) const;

  virtual void create_vertex(VertexArray& vertex_array, const Color4f& color,
                             const std::array<Vector3d, 3>& points,
                             const std::array<Vector3d, 3>& normals,
                             size_t active_point_index = 0, size_t primitive_index = 0,
                             size_t shape_size = 0, bool outlines = false, bool mirror = false) const;
  void add_shader_pointers(VertexArray& vertex_array); // This could stay protected, were it not for VertexStateManager
  void add_color(VertexArray& vertex_array, const Color4f& color);

protected:
  void add_shader_data(VertexArray& vertex_array);
  void shader_attribs_enable() const;
  void shader_attribs_disable() const;

  mutable std::unordered_map<std::pair<const PolySet *, const Transform3d *>, int,
                             boost::hash<std::pair<const PolySet *, const Transform3d *>>> geomVisitMark;

private:
  void add_shader_attributes(VertexArray& vertex_array,
                             size_t active_point_index = 0, size_t primitive_index = 0,
                             size_t shape_size = 0, bool outlines = false) const;

  size_t shader_attributes_index{0};
  enum ShaderAttribIndex {
    BARYCENTRIC_ATTRIB
  };
};
