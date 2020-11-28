#pragma once

#ifndef __VERTEX_H__
#define __VERTEX_H__

#include <system-gl.h>
#include <printutils.h>
#include <unordered_map>

// Hash function for opengl vertex data.
template<typename T>
struct vertex_hash : std::unary_function<T, size_t> {
  std::size_t operator()(T const& vertex) const {
    size_t seed = 0;
    for (size_t i = 0; i < vertex.size(); ++i) {
      auto elem = *(vertex.data() + i);
      seed ^= std::hash<typename T::value_type>()(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

typedef std::unordered_map<std::vector<GLbyte>, GLuint, vertex_hash<std::vector<GLbyte>>> ElementsMap;

// Interface class for basic attribute data that will be loaded into VBO
class IAttributeData
{
public:
	IAttributeData() {}
	virtual ~IAttributeData() {}
	
	// Return number of elements that make up one attribute
	virtual size_t count() const = 0;
	// Return number of elements in vector
	virtual size_t size() const = 0;
	// Return size in bytes of the element type
	virtual size_t sizeofType() const = 0;
	// Return the total size in bytes of one attribute
	virtual size_t sizeofAttribute() const = 0;
	// Return the total size in bytes of entire element vector
	virtual size_t sizeInBytes() const = 0;
	// Return the OpenGL type of the element
	virtual GLenum glType() const = 0;
	// Return pointer to the raw bytes of the element vector
	virtual const GLbyte* toBytes() const = 0;
	// Create an empty copy of the attribute data type
	virtual std::shared_ptr<IAttributeData> create() const = 0;
	virtual void append(const IAttributeData &data) = 0;

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
void addAttributeValues(IAttributeData &);
// Template helper function to load multiple attribute values in one call
template <typename T, typename... Args>
void addAttributeValues(IAttributeData &attrib, T value, Args... values) {
	attrib.addData(value);
	addAttributeValues(attrib, values...);
}

// Template helper function to load multiple copies of the same multiple attribute values in one call.
// Used to add the same normal and colors to multiple triangle points.
template <typename T, typename... Args>
void addAttributeValues(size_t copies, IAttributeData &attrib, T value, Args... values) {
	if (copies > 0) {
		addAttributeValues(attrib, value, values...);
		addAttributeValues(copies-1, attrib, value, values...);
	}
}

// Template class for implementing IAttributeData interface abstract class
template <typename T, size_t C, GLenum E>
class AttributeData : public IAttributeData
{
public:
	AttributeData() : data_() {}
	virtual ~AttributeData() {}

	inline size_t count() const override { return C; }
	inline size_t size() const override { return data_.size(); }
	inline size_t sizeofType() const override { return sizeof(T); }
	inline size_t sizeofAttribute() const override { return sizeof(T) * C; }
	inline size_t sizeInBytes() const override { return data_.size() * sizeof(T); }
	inline GLenum glType() const override { return E; }
	inline std::shared_ptr<IAttributeData> create() const override { return std::make_shared<AttributeData<T,C,E>>(); }
	void append(const IAttributeData &data) override {
		const AttributeData<T,C,E> *from = dynamic_cast<const AttributeData<T,C,E> *>(&data);
		if (from != nullptr) {
			data_.insert(data_.end(),from->data_.begin(),from->data_.end());
		} else {
			PRINTD("AttributeData append type mismatch!!!");
		}
	}
	inline const GLbyte* toBytes() const override { return (GLbyte *)(data_.data()); }
	
	inline void addData(GLbyte value) override { add_data((T)value); }
	inline void addData(GLshort value) override { add_data((T)value); }
	inline void addData(GLushort value) override { add_data((T)value); }
	inline void addData(GLint value) override { add_data((T)value); }
	inline void addData(GLuint value) override { add_data((T)value); }
	inline void addData(GLfloat value) override { add_data((T)value); }
	inline void addData(GLdouble value) override { add_data((T)value); }
	
	// Return the template type element vector
	inline std::shared_ptr<std::vector<T>> getData() const { return std::shared_ptr<std::vector<T>>(data_); }
	
private:
	// Internal method to add data of template type to element vector
	void add_data(T value) { data_.emplace_back(value); }

	std::vector<T> data_;
};

// Storeage and access class for multiple AttributeData that make up one vertex.
class VertexData
{
public:
	VertexData()
		:  position_index_(0), position_data_(nullptr),
		   normal_index_(0), normal_data_(nullptr),
		   color_index_(0), color_data_(nullptr),
		   stride_(0)
	{}
	virtual ~VertexData() {}

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
	
	void append(const VertexData &data);
	
	// Return reference to internal IAttributeData vector
	inline const std::vector<std::shared_ptr<IAttributeData>> &attributes() const { return attributes_; }
	// Return reference to IAttributeData
	inline const std::shared_ptr<IAttributeData> attributeData() const { if (attributes_.size()) return attributes_.back(); else return nullptr; }
	// Return reference to position attribute data
	inline const std::shared_ptr<IAttributeData> &positionData() const { return position_data_; }
	// Return reference to normal attribute data
	inline const std::shared_ptr<IAttributeData> &normalData() const { return normal_data_; }
	// Return reference to color data
	inline const std::shared_ptr<IAttributeData> &colorData() const { return color_data_; }
	// Check if VertexData has position data
	inline bool hasPositionData() const { return (position_data_ != nullptr); }
	// Return position attribute data vector index
	inline size_t positionIndex() const { return position_index_; }
	// Check if VertexData has normal data
	inline bool hasNormalData() const { return (normal_data_ != nullptr); }
	// Return normal attribute data vector index
	inline size_t normalIndex() const { return normal_index_; }
	// Check if VertexData has color data
	inline bool hasColorData() const { return (color_data_ != nullptr); }
	// Return color attribute data vector index
	inline size_t colorIndex() const { return color_index_; }
	// Return stride of VertexData
	inline size_t stride() const { return stride_; }
	
	// Calculate the offset of interleaved attribute data based on VertexData index
	size_t interleavedOffset(size_t index) const {
		if (index && attributes_.size()) {
			--index;
			return (attributes_[index]->sizeofAttribute()+interleavedOffset(index));
		}
		return 0;
	}
	// Calculate the total size of the buffer in bytes
	size_t sizeInBytes() const { size_t size = 0; for (const auto &data : attributes_) size+=data->sizeInBytes(); return size; }
	// Calculate the total number of items in buffer
	inline size_t size() const {
		if (stride_) { return sizeInBytes()/stride(); }
		else { size_t size = 0; for (const auto &data : attributes_) size+=data->size(); return size; }
	}
	inline bool empty() const { return attributes_.empty(); }

	// Create an interleaved buffer and return it as GLbyte array pointer
	void fillInterleavedBuffer(GLbyte* buf) const;
	// Create an interleaved buffer in the provided vbo.
	// If the vbo does not exist it will be created and returned.
	void createInterleavedVBO(GLuint &vbo) const;
	
	// Create an empty copy of the this vertex data
	std::shared_ptr<VertexData> create() const;
	
private:
	std::vector<std::shared_ptr<IAttributeData>> attributes_;
	size_t position_index_;
	std::shared_ptr<IAttributeData> position_data_;
	size_t normal_index_;
	std::shared_ptr<IAttributeData> normal_data_;
	size_t color_index_;
	std::shared_ptr<IAttributeData> color_data_;
	size_t stride_;
};

// Storage for minimum state information necessary to draw VBO.
class VertexState
{
public:
	VertexState()
		: draw_mode_(GL_TRIANGLES), draw_size_(0), draw_type_(0), draw_offset_(0),
		  element_offset_(0), vertices_vbo_(0), elements_vbo_(0)
	{}
	VertexState(GLenum draw_mode, GLsizei draw_size, GLenum draw_type, size_t draw_offset, size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo)
		: draw_mode_(draw_mode), draw_size_(draw_size), draw_type_(draw_type), draw_offset_(draw_offset),
		  element_offset_(element_offset), vertices_vbo_(vertices_vbo), elements_vbo_(elements_vbo)
	{}
	virtual ~VertexState() {}

	// Return the OpenGL mode for glDrawArrays/glDrawElements call
	inline GLenum drawMode() const { return draw_mode_; }
	// Set the OpenGL mode for glDrawArrays/glDrawElements call
	inline void drawMode(GLenum draw_mode) { draw_mode_ = draw_mode; }
	// Return the number of vertices for glDrawArrays/glDrawElements call
	inline GLsizei drawSize() const { return draw_size_; }
	// Set the number of vertices for glDrawArrays/glDrawElements call
	inline void drawSize(GLsizei draw_size) { draw_size_ = draw_size; }
	// Return the OpenGL type for glDrawElements call
	inline GLenum drawType() const { return draw_type_; }
	// Set the OpenGL type for glDrawElements call
	inline void drawType(GLenum draw_type) { draw_type_ = draw_type; }
	// Return the VBO offset for glDrawArrays call
	inline size_t drawOffset() const { return draw_offset_; }
	// Set the VBO offset for glDrawArrays call
	inline void drawOffset(size_t draw_offset) { draw_offset_ = draw_offset; }
	// Return the Element VBO offset for glDrawElements call
	inline size_t elementOffset() const { return element_offset_; }
	// Set the Element VBO offset for glDrawElements call
	inline void elementOffset(size_t element_offset) { element_offset_ = element_offset; }
	
	// Wrap glDrawArrays/glDrawElements call and use gl_begin/gl_end state information
	void draw() const;
	
	// Mimic VAO state functionality. Lambda functions used to hold OpenGL state calls.
	inline std::vector<std::function<void()>> &glBegin() { return gl_begin_; }
	inline std::vector<std::function<void()>> &glEnd() { return gl_end_; }
	
	inline GLuint &verticesVBO() { return vertices_vbo_; }
	inline GLuint &elementsVBO() { return elements_vbo_; }

private:
	GLenum draw_mode_;
	GLsizei draw_size_;
	GLenum draw_type_;
	size_t draw_offset_;
	size_t element_offset_;
	GLuint vertices_vbo_;
	GLuint elements_vbo_;
	std::vector<std::function<void()>> gl_begin_;
	std::vector<std::function<void()>> gl_end_;
};
// A set of VertexState objects
typedef std::vector<std::shared_ptr<VertexState>> VertexStates;

// Allows Renderers to override VertexState objects with their own derived
// type. VertexArray will create the appropriate type for creating
// a VertexState object.
class VertexStateFactory {
public:
	VertexStateFactory() {}
	virtual ~VertexStateFactory() {}
	
	// Create and return a VertexState object
	virtual std::shared_ptr<VertexState> createVertexState(GLenum draw_mode, size_t draw_size, GLenum draw_type, size_t draw_offset, size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo) const {
		return std::make_shared<VertexState>(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo, elements_vbo);
	}
};

// Combine vertex data with vertex states. Creates VBOs.
class VertexArray
{
public:
	VertexArray(std::shared_ptr<VertexStateFactory> factory, VertexStates &states, bool use_elements = false)
		: factory_(std::move(factory)), states_(states), write_index_(0),
		  surface_index_(0), edge_index_(0), vertices_vbo_(0), elements_vbo_(0),
		  use_elements_(use_elements)
	{
		glGenBuffers(1, &vertices_vbo_);
		if (use_elements)
			glGenBuffers(1, &elements_vbo_);
	}
	virtual ~VertexArray() {}
	
	// Add generic VertexData to VertexArray
	void addVertexData(std::shared_ptr<VertexData> data) { vertices_.emplace_back(std::move(data)); }
	// Add common surface data vertex layout PNC
	void addSurfaceData();
	// Add common edge data vertex layout PC
	void addEdgeData();
	// Add elements data to VertexArray
	void addElementsData(std::shared_ptr<IAttributeData> data) { elements_.addAttributeData(data); }
	
	void append(const VertexArray &vertex_array);
	
	// Return reference to the VertexStates
	inline VertexStates &states() { return states_; }
	// Return reference to VertexData at current internal write index
	inline std::shared_ptr<VertexData> data() { return vertices_[write_index_]; }
	// Return reference to VertexData at custom external write index
	inline std::shared_ptr<VertexData> data(size_t write_index) { return vertices_[write_index]; }
	// Return reference to surface VertexData if it exists
	inline std::shared_ptr<VertexData> surfaceData() { return vertices_[surface_index_]; }
	// Return reference to edge VertexData if it exists
	inline std::shared_ptr<VertexData> edgeData() { return vertices_[edge_index_]; }
	// Return reference to elements
	inline VertexData &elements() { return elements_; }
	// Return reference to elements data if it exists
	inline std::shared_ptr<IAttributeData> elementsData() { return elements_.attributeData(); }
	// Return the number of VertexData in the array
	inline size_t size() const { return vertices_.size(); }
	// Calculate the total size of the buffer in bytes
	inline size_t sizeInBytes() const { size_t size = 0; for (const auto &data : vertices_) size+=data->sizeInBytes(); return size; }
	// Return the current internal write index
	inline size_t writeIndex() const { return write_index_; }
	// Set the internal write index
	inline void writeIndex(size_t index) { if (index < vertices_.size()) write_index_ = index; }
	// Set the internal write index to the surface index
	inline void writeSurface() { write_index_ = surface_index_; }
	// Set the internal write index to the edge index
	inline void writeEdge() { write_index_ = edge_index_; }
	
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

	// Create an interleaved buffer and return it as GLbyte array pointer
	void fillInterleavedBuffer(std::vector<GLbyte> &buf) const;

	// Create an interleaved VBO from the VertexData in the array.
	void createInterleavedVBOs();
	
	// Method adds begin/end states that enable and point to the VertexData in the array
	void addAttributePointers(size_t start_offset = 0);
	
	// Create an empty copy of the vertex array type
	std::shared_ptr<VertexArray> create() const;
	
	inline bool useElements() { return use_elements_; }
	inline GLuint &verticesVBO() { return vertices_vbo_; }
	inline GLuint &elementsVBO() { return elements_vbo_; }
	inline ElementsMap &elementsMap() { return elements_map_; }

private:
	std::shared_ptr<VertexStateFactory> factory_;
	VertexStates &states_;
	size_t write_index_;
	size_t surface_index_;
	size_t edge_index_;
	GLuint vertices_vbo_;
	GLuint elements_vbo_;
	bool use_elements_;
	std::vector<std::shared_ptr<VertexData>> vertices_;
	VertexData elements_;
	ElementsMap elements_map_;
};

#endif // __VERTEX_H__