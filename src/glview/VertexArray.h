#pragma once

#include <array>
#include <functional>
#include <memory>
#include <cstddef>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <utility>
#include <vector>

#include "glview/system-gl.h"
#include "utils/printutils.h"
#include "geometry/linalg.h"
#include "Feature.h"
#include "glview/VertexState.h"

// Hash function for opengl vertex data.
template <typename T>
struct vertex_hash {
  std::size_t operator()(T const& vertex) const {
    size_t seed = 0;
    for (size_t i = 0; i < vertex.size(); ++i) boost::hash_combine(seed, vertex.data()[i]);
    return seed;
  }
};

using ElementsMap = std::unordered_map<std::vector<GLbyte>, GLuint, vertex_hash<std::vector<GLbyte>>>;

// Interface class for basic attribute data that will be loaded into VBO
class IAttributeData
{
public:
  IAttributeData() = default;
  virtual ~IAttributeData() = default;

  // Return number of elements that make up one attribute
  [[nodiscard]] virtual size_t count() const = 0;
  // Return number of elements in vector
  [[nodiscard]] virtual size_t size() const = 0;
  // Return size in bytes of the element type
  [[nodiscard]] virtual size_t sizeofType() const = 0;
  // Return the total size in bytes of one attribute
  [[nodiscard]] virtual size_t sizeofAttribute() const = 0;
  // Return the total size in bytes of entire element vector
  [[nodiscard]] virtual size_t sizeInBytes() const = 0;
  // Return the OpenGL type of the element
  [[nodiscard]] virtual GLenum glType() const = 0;
  // Return pointer to the raw bytes of the element vector
  [[nodiscard]] virtual const GLbyte *toBytes() const = 0;
  // Clear the entire attribute
  virtual void clear() = 0;
  // Remove data from the end of the attribute
  virtual void remove(size_t count) = 0;

  // Add common types to element vector
  virtual void addData(GLbyte data) = 0;
  virtual void addData(GLshort data) = 0;
  virtual void addData(GLushort data) = 0;
  virtual void addData(GLint data) = 0;
  virtual void addData(GLuint data) = 0;
  virtual void addData(GLfloat data) = 0;
  virtual void addData(GLdouble data) = 0;
};

// Helper function to finish recursion in addAttributeValues call
void addAttributeValues(IAttributeData&);
// Template helper function to load multiple attribute values in one call
template <typename T, typename ... Args>
void addAttributeValues(IAttributeData& attrib, T value, Args... values) {
  attrib.addData(value);
  addAttributeValues(attrib, values ...);
}

// Template helper function to load multiple copies of the same multiple attribute values in one call.
// Used to add the same normal and colors to multiple triangle points.
template <typename T, typename ... Args>
void addAttributeValues(size_t copies, IAttributeData& attrib, T value, Args... values) {
  if (copies > 0) {
    addAttributeValues(attrib, value, values ...);
    addAttributeValues(copies - 1, attrib, value, values ...);
  }
}

// Template class for implementing IAttributeData interface abstract class
template <typename T, size_t C, GLenum E>
class AttributeData : public IAttributeData
{
public:
  AttributeData() : data_() {}

  [[nodiscard]] inline size_t count() const override { return C; }
  [[nodiscard]] inline size_t size() const override { return data_.size(); }
  [[nodiscard]] inline size_t sizeofType() const override { return sizeof(T); }
  [[nodiscard]] inline size_t sizeofAttribute() const override { return sizeof(T) * C; }
  [[nodiscard]] inline size_t sizeInBytes() const override { return data_.size() * sizeof(T); }
  [[nodiscard]] inline GLenum glType() const override { return E; }
  void clear() override { data_.clear(); }
  void remove(size_t count) override { data_.erase(data_.end() - (count * C), data_.end()); }
  [[nodiscard]] inline const GLbyte *toBytes() const override { return (GLbyte *)(data_.data()); }

  inline void addData(GLbyte value) override { add_data((T)value); }
  inline void addData(GLshort value) override { add_data((T)value); }
  inline void addData(GLushort value) override { add_data((T)value); }
  inline void addData(GLint value) override { add_data((T)value); }
  inline void addData(GLuint value) override { add_data((T)value); }
  inline void addData(GLfloat value) override { add_data((T)value); }
  inline void addData(GLdouble value) override { add_data((T)value); }

  // Return the template type element vector
  [[nodiscard]] inline std::shared_ptr<std::vector<T>> getData() const { return std::shared_ptr<std::vector<T>>(data_); }

private:
  // Internal method to add data of template type to element vector
  void add_data(T value) { data_.emplace_back(value); }

  std::vector<T> data_;
};

// Storage and access class for multiple AttributeData that make up one vertex.
class VertexData
{
public:
  VertexData() : position_data_(nullptr), normal_data_(nullptr), color_data_(nullptr) {}
  virtual ~VertexData() = default;

  // Add generic attribute data to vertex vector
  void addAttributeData(std::shared_ptr<IAttributeData> data)
  {
    stride_ += data->sizeofAttribute();
    attributes_.emplace_back(std::move(data));
  }

  // Add position attribute data to vertex vector
  void addPositionData(std::shared_ptr<IAttributeData> data)
  {
    position_index_ = attributes_.size();
    stride_ += data->sizeofAttribute();
    attributes_.emplace_back(std::move(data));
    position_data_ = std::shared_ptr<IAttributeData>(attributes_.back());
  }
  // Add normal attribute data to vertex vector
  void addNormalData(std::shared_ptr<IAttributeData> data)
  {
    normal_index_ = attributes_.size();
    stride_ += data->sizeofAttribute();
    attributes_.emplace_back(std::move(data));
    normal_data_ = std::shared_ptr<IAttributeData>(attributes_.back());
  }
  // Add color attribute data to vertex vector
  void addColorData(std::shared_ptr<IAttributeData> data)
  {
    color_index_ = attributes_.size();
    stride_ += data->sizeofAttribute();
    attributes_.emplace_back(std::move(data));
    color_data_ = std::shared_ptr<IAttributeData>(attributes_.back());
  }

  void clear() { for (auto& a : attributes_) a->clear(); }
  // Remove the last n interleaved vertices
  void remove(size_t count = 1);

  // Return reference to internal IAttributeData vector
  [[nodiscard]] inline const std::vector<std::shared_ptr<IAttributeData>>& attributes() const { return attributes_; }
  // Return reference to the last added IAttributeData. This is typically where elements data is stored.
  [[nodiscard]] inline const std::shared_ptr<IAttributeData> attributeData() const { if (attributes_.size()) return attributes_.back(); else return nullptr; }
  // Return reference to position attribute data
  [[nodiscard]] inline const std::shared_ptr<IAttributeData>& positionData() const { return position_data_; }
  // Return reference to normal attribute data
  [[nodiscard]] inline const std::shared_ptr<IAttributeData>& normalData() const { return normal_data_; }
  // Return reference to color data
  [[nodiscard]] inline const std::shared_ptr<IAttributeData>& colorData() const { return color_data_; }
  // Check if VertexData has position data
  [[nodiscard]] inline bool hasPositionData() const { return (position_data_ != nullptr); }
  // Return position attribute data vector index
  [[nodiscard]] inline size_t positionIndex() const { return position_index_; }
  // Check if VertexData has normal data
  [[nodiscard]] inline bool hasNormalData() const { return (normal_data_ != nullptr); }
  // Return normal attribute data vector index
  [[nodiscard]] inline size_t normalIndex() const { return normal_index_; }
  // Check if VertexData has color data
  [[nodiscard]] inline bool hasColorData() const { return (color_data_ != nullptr); }
  // Return color attribute data vector index
  [[nodiscard]] inline size_t colorIndex() const { return color_index_; }
  // Return stride of VertexData
  [[nodiscard]] inline size_t stride() const { return stride_; }

  // Calculate the offset of interleaved attribute data based on VertexData index
  [[nodiscard]] size_t interleavedOffset(size_t index) const {
    if (index && attributes_.size()) {
      --index;
      return (attributes_[index]->sizeofAttribute() + interleavedOffset(index));
    }
    return 0;
  }
  // Calculate the total size of the buffer in bytes
  [[nodiscard]] size_t sizeInBytes() const { size_t size = 0; for (const auto& data : attributes_) size += data->sizeInBytes(); return size; }
  // Calculate the total number of items in buffer
  [[nodiscard]] inline size_t size() const {
    if (stride_) {
      return sizeInBytes() / stride();
    } else {
      size_t size = 0; for (const auto& data : attributes_) size += data->size(); return size;
    }
  }
  [[nodiscard]] inline bool empty() const { return attributes_.empty(); }

  void allocateBuffers(size_t num_vertices);

  // Get the last interleaved vertex
  void getLastVertex(std::vector<GLbyte>& interleaved_buffer) const;
  // Create an interleaved buffer in the provided vbo.
  // If the vbo does not exist it will be created and returned.
  // void createInterleavedVBO(GLuint& vbo) const;

private:
  std::vector<std::shared_ptr<IAttributeData>> attributes_;
  size_t position_index_{0};
  std::shared_ptr<IAttributeData> position_data_;
  size_t normal_index_{0};
  std::shared_ptr<IAttributeData> normal_data_;
  size_t color_index_{0};
  std::shared_ptr<IAttributeData> color_data_;
  size_t stride_{0};
};


// Combine vertex data with vertex states. Creates VBOs.
class VertexArray
{
public:
  using CreateVertexCallback = std::function<void (VertexArray& vertex_array,
                                                   size_t active_point_index, size_t primitive_index,
                                                   size_t shape_size, bool outlines)>;


  VertexArray(std::unique_ptr<VertexStateFactory> factory, std::vector<std::shared_ptr<VertexState>>& states,
              GLuint vertices_vbo, GLuint elements_vbo)
    : factory_(std::move(factory)), states_(states),
      vertices_vbo_(vertices_vbo), elements_vbo_(elements_vbo)
  {
  }
  virtual ~VertexArray() = default;

  // Add generic VertexData to VertexArray
  void addVertexData(std::shared_ptr<VertexData> data) { vertices_.emplace_back(std::move(data)); }
  // Add common surface data vertex layout PNC
  void addSurfaceData();
  // Add common edge data vertex layout PC
  void addEdgeData();
  // Add elements data to VertexArray
  void addElementsData(std::shared_ptr<IAttributeData> data) {
    elements_.addAttributeData(std::move(data));
  }

  // Clear all data from the VertexArray
  void clear() { for (auto& v : vertices_) v->clear(); }

  // Create a single vertex in the VertexArray
  // The method parameters provide a common interface to pass all data
  // necessary to create a complete vertex
  void createVertex(const std::array<Vector3d, 3>& points,
                    const std::array<Vector3d, 3>& normals,
                    const Color4f& color,
                    size_t active_point_index = 0, size_t primitive_index = 0,
                    size_t shape_size = 0,
                    bool outlines = false, bool mirror = false,
                    const CreateVertexCallback& vertex_callback = nullptr);

  // Return reference to the VertexStates
  inline std::vector<std::shared_ptr<VertexState>>& states() { return states_; }
  // Return reference to VertexData at current internal write index
  inline std::shared_ptr<VertexData> data() { return vertices_[write_index_]; }
  // Return reference to VertexData at custom external write index
  inline std::shared_ptr<VertexData> data(size_t write_index) { return vertices_[write_index]; }
  // Return reference to surface VertexData if it exists
  inline std::shared_ptr<VertexData> surfaceData() { return vertices_[surface_index_]; }
  // Return reference to edge VertexData if it exists
  inline std::shared_ptr<VertexData> edgeData() { return vertices_[edge_index_]; }
  // Return reference to elements
  inline VertexData& elements() { return elements_; }
  // Return reference to elements data if it exists
  inline std::shared_ptr<IAttributeData> elementsData() { return elements_.attributeData(); }
  // Return the number of VertexData in the array
  inline size_t size() const { return vertices_.size(); }
  // Calculate the total size of the buffer in bytes
  inline size_t sizeInBytes() const { size_t size = 0; for (const auto& data : vertices_) size += data->sizeInBytes(); return size; }
  // Return the current internal write index
  inline size_t writeIndex() const { return write_index_; }
  // Set the internal write index
  inline void writeIndex(size_t index) { if (index < vertices_.size()) write_index_ = index; }
  // Set the internal write index to the surface index
  inline void writeSurface() { write_index_ = surface_index_; }
  // Set the internal write index to the edge index
  inline void writeEdge() { write_index_ = edge_index_; }
  // Return the total stride for all buffers
  inline size_t stride() const {
    size_t stride = 0; for (const auto& v : vertices_) {
      stride += v->stride();
    }
    return stride;
  }

  // Calculate and return the offset in bytes of a given index
  size_t indexOffset(size_t index) const {
    if (index) {
      --index;
      return vertices_[index]->sizeInBytes() + indexOffset(index);
    }
    return 0;
  }

  // Use VertexStateFactory to create a new VertexState object
  std::shared_ptr<VertexState> createVertexState(GLenum draw_mode, size_t draw_size, GLenum draw_type, size_t draw_offset, size_t element_offset) const {
    return factory_->createVertexState(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo_, elements_vbo_);
  }

  void allocateBuffers(size_t num_vertices);

  // Create an interleaved VBO from the VertexData in the array.
  void createInterleavedVBOs();

  // Method adds begin/end states that enable and point to the VertexData in the array
  void addAttributePointers(size_t start_offset = 0);

  inline GLuint verticesVBO() const { return vertices_vbo_; }
  inline size_t verticesSize() const { return vertices_size_; }
  inline void setVerticesSize(size_t vertices_size) {
    vertices_size_ = vertices_size;
    interleaved_buffer_.resize(vertices_size_);
  }
  inline size_t verticesOffset() const { return vertices_offset_; }
  inline void setVerticesOffset(size_t offset) { vertices_offset_ = offset; }

  // Return whether this Vertex Array uses elements (indexed rendering)
  inline bool useElements() const { return elements_vbo_ != 0; }
  inline GLuint elementsVBO() const { return elements_vbo_; }
  inline size_t elementsSize() const { return elements_size_; }
  inline void setElementsSize(size_t elements_size) { elements_size_ = elements_size; }
  inline size_t elementsOffset() const { return elements_offset_; }
  inline void setElementsOffset(size_t offset) { elements_offset_ = offset; }

  // Return the internal unique vertex/element map
  inline ElementsMap& elementsMap() { return elements_map_; }

private:
  std::unique_ptr<VertexStateFactory> factory_;
  std::vector<std::shared_ptr<VertexState>>& states_;
  size_t write_index_{0};
  size_t surface_index_{0};
  size_t edge_index_{0};
  std::vector<std::shared_ptr<VertexData>> vertices_;
  std::vector<GLbyte> interleaved_buffer_;

  // Vertex VBO
  GLuint vertices_vbo_;
  // Allocated size of vertex VBO
  size_t vertices_size_{0};
  size_t vertices_offset_{0};

  // Element VBO
  GLuint elements_vbo_;
  // Allocated size of vertex VBO
  size_t elements_size_{0};
  size_t elements_offset_{0};

  VertexData elements_;
  ElementsMap elements_map_;
};
