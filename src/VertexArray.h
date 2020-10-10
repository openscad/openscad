#pragma once

#ifndef __VERTEX_H__
#define __VERTEX_H__

#include <system-gl.h>
#include <printutils.h>

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
	
	// Return reference to internal IAttributeData vector
	inline const std::vector<std::shared_ptr<IAttributeData>> &attributes() const { return attributes_; }
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
		if (index && attributes_.size()) { return (attributes_[--index]->sizeofAttribute()+interleavedOffset(index)); }
		return 0;
	}
	// Calculate the offset of sequential attribute data based on VertexData index
	size_t sequentialOffset(size_t index) const {
		if (index) return (attributes_[--index]->sizeInBytes()+sequentialOffset(index));
		return 0;
	}
	// Calculate the total size of the buffer in bytes
	size_t sizeInBytes() const { size_t size = 0; for (const auto &data : attributes_) size+=data->sizeInBytes(); return size; }
	// Calculate the total number of vertices in buffer
	inline size_t size() const { return sizeInBytes()/stride(); }

	// Create an interleaved buffer and return it as unique_ptr
	std::unique_ptr<std::vector<GLbyte>> createInterleavedBuffer() const;
	// Create an interleaved buffer in the provided vbo.
	// If the vbo does not exist it will be created and returned.
	void createInterleavedVBO(GLuint &vbo) const;
	
	// Create a sequential buffer and return it as unique_ptr
	std::unique_ptr<std::vector<GLbyte>> createSequentialBuffer() const;
	// Create a sequential buffer in the provided vbo.
	// If the vbo does not exist it will be created and returned.
	void createSequentialVBO(GLuint &vbo) const;
	
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
		: draw_type_(GL_TRIANGLES), draw_size_(0), draw_offset_(0)
	{}
	VertexState(GLenum draw_type, GLsizei draw_size, size_t draw_offset = 0)
		: draw_type_(draw_type), draw_size_(draw_size), draw_offset_(draw_offset)
	{}
	virtual ~VertexState() {}

	// Return the OpenGL type for glDrawArrays call
	inline GLenum drawType() const { return draw_type_; }
	// Set the OpenGL type for glDrawArrays call
	inline void drawType(GLenum draw_type) { draw_type_ = draw_type; }
	// Return the number of vertices for glDrawArrays call
	inline GLsizei drawSize() const { return draw_size_; }
	// Set the number of vertices for glDrawArrays call
	inline void drawSize(GLsizei draw_size) { draw_size_ = draw_size; }
	// Return the VBO offset for glDrawArrays call
	inline size_t drawOffset() const { return draw_offset_; }
	// Set the VBO offset for glDrawArrays call
	inline void drawOffset(size_t draw_offset) { draw_offset_ = draw_offset; }
	
	// Wrap glDrawArrays call and use gl_begin/gl_end state information
	void drawArrays() const;
	
	// Mimic VAO state functionality. Lambda functions used to hold OpenGL state calls.
	inline std::vector<std::function<void()>> &glBegin() { return gl_begin_; }
	inline std::vector<std::function<void()>> &glEnd() { return gl_end_; }

private:
	GLenum draw_type_;
	GLsizei draw_size_;
	size_t draw_offset_;
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
	virtual std::shared_ptr<VertexState> createVertexState(GLenum draw_type, size_t draw_size, size_t draw_offset) const {
		return std::make_shared<VertexState>(draw_type, draw_size, draw_offset);
	}
};

// Combine vertex data with vertex states. Creates VBOs.
class VertexArray
{
public:
	VertexArray(std::unique_ptr<VertexStateFactory> factory, VertexStates &states)
		: factory_(std::move(factory)), states_(states), write_index_(0),
		  surface_index_(0), edge_index_(0)
	{};
	virtual ~VertexArray() {};
	
	// Add generic VertexData to VertexArray
	void addVertexData(std::shared_ptr<VertexData> data) { array_.emplace_back(std::move(data)); }
	// Add common surface data vertex layout PNC
	void addSurfaceData();
	// Add common edge data vertex layout PC
	void addEdgeData();
	
	// Return reference to the VertexStates
	inline VertexStates &states() { return states_; }
	// Return reference to VertexData at current internal write index
	inline std::shared_ptr<VertexData> data() { return array_[write_index_]; }
	// Return reference to VertexData at custom external write index
	inline std::shared_ptr<VertexData> data(size_t write_index) { return array_[write_index]; }
	// Return reference to surface VertexData if it exists
	inline std::shared_ptr<VertexData> surfaceData() { return array_[surface_index_]; }
	// Return reference to edge VertexData if it exists
	inline std::shared_ptr<VertexData> edgeData() { return array_[edge_index_]; }
	// Return the number of VertexData in the array
	inline size_t size() const { return array_.size(); }
	// Return the current internal write index
	inline size_t writeIndex() const { return write_index_; }
	// Set the internal write index
	inline void writeIndex(size_t index) { if (index < array_.size()) write_index_ = index; }
	// Set the internal write index to the surface index
	inline void writeSurface() { write_index_ = surface_index_; }
	// Set the internal write index to the edge index
	inline void writeEdge() { write_index_ = edge_index_; }
	
	// Calculate and return the offset in bytes of a given index
	size_t indexOffset(size_t index) const {
		if (index > 0) return array_[--index]->sizeInBytes() + indexOffset(index);
		return 0;
	}
	// Use VertexStateFactory to create a new VertexState object
	std::shared_ptr<VertexState> createVertexState(GLenum draw_type, size_t draw_size, size_t draw_offset = 0) const {
		return factory_->createVertexState(draw_type, draw_size, draw_offset);
	}

	// Create an interleaved VBO in the provided VBO from the VertexData in the array.
	// If the vbo does not exist it will be created and returned.
	void createInterleavedVBO(GLuint &vbo) const;
	// Create an sequential VBO in the provided VBO from the VertexData in the array.
	// If the vbo does not exist it will be created and returned.
	void createSequentialVBO(GLuint &vbo) const;
	
	// Method adds begin/end states that enable and point to the VertexData in the array
	void addAttributePointers(size_t start_offset = 0);

private:
	std::unique_ptr<VertexStateFactory> factory_;
	VertexStates &states_;
	size_t write_index_;
	size_t surface_index_;
	size_t edge_index_;
	std::vector<std::shared_ptr<VertexData>> array_;
};

#endif // __VERTEX_H__