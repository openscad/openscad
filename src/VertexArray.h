#pragma once

#ifndef __VERTEX_H__
#define __VERTEX_H__

#include <system-gl.h>
#include <printutils.h>

class IAttributeData
{
public:
	IAttributeData() {}
	virtual ~IAttributeData() {}
	
	virtual size_t count() const = 0;
	virtual size_t size() const = 0;
	virtual size_t sizeofType() const = 0;
	virtual size_t sizeofAttribute() const = 0;
	virtual size_t sizeInBytes() const = 0;
	virtual GLenum glType() const = 0;
	virtual const std::shared_ptr<GLbyte> toBytes() const = 0;

	virtual void addData(GLbyte data) = 0;
	virtual void addData(GLshort data) = 0;
	virtual void addData(GLushort data) = 0;
	virtual void addData(GLint data) = 0;
	virtual void addData(GLuint data) = 0;
	virtual void addData(GLfloat data) = 0;
	virtual void addData(GLdouble data) = 0;
};

void addAttributeValues(IAttributeData &);
template <typename T, typename... Args>
void addAttributeValues(IAttributeData &attrib, T value, Args... values) { attrib.addData(value); addAttributeValues(attrib, values...); }

template <typename T, typename... Args>
void addAttributeValues(size_t copies, IAttributeData &attrib, T value, Args... values) { if (copies > 0) { addAttributeValues(attrib, value, values...); addAttributeValues(copies-1, attrib, value, values...); } }

template <typename T, size_t C, GLenum E>
class AttributeData : public IAttributeData
{
public:
	AttributeData() : data_(std::make_shared<std::vector<T>>()) {}
	virtual ~AttributeData() {}

	size_t count() const override { return C; }
	size_t size() const override { return data_->size(); }
	size_t sizeofType() const override { return sizeof(T); }
	size_t sizeofAttribute() const override { return sizeof(T) * C; }
	size_t sizeInBytes() const override { return data_->size() * sizeof(T); }
	GLenum glType() const override { return E; }

	const std::shared_ptr<GLbyte> toBytes() const override { return std::shared_ptr<GLbyte>(data_, (GLbyte *)(data_.get()->data())); }
	
	void addData(GLbyte value) override { add_data((T)value); }
	void addData(GLshort value) override { add_data((T)value); }
	void addData(GLushort value) override { add_data((T)value); }
	void addData(GLint value) override { add_data((T)value); }
	void addData(GLuint value) override { add_data((T)value); }
	void addData(GLfloat value) override { add_data((T)value); }
	void addData(GLdouble value) override { add_data((T)value); }
	
	std::shared_ptr<std::vector<T>> getData() const { return std::shared_ptr<std::vector<T>>(data_); }
	
private:
	void add_data(T value) { data_->emplace_back(value); }

	
	std::shared_ptr<std::vector<T>> data_;
};

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

	void addAttributeData(std::shared_ptr<IAttributeData> data)
	{
		stride_ += data->sizeofAttribute();
		attributes_.emplace_back(std::move(data));
	}
	
	void addPositionData(std::shared_ptr<IAttributeData> data)
	{
		position_index_ = attributes_.size();
		stride_ += data->sizeofAttribute();
		attributes_.emplace_back(std::move(data));
		position_data_ = std::shared_ptr<IAttributeData>(attributes_.back());
	}
	void addNormalData(std::shared_ptr<IAttributeData> data)
	{
		normal_index_ = attributes_.size();
		stride_ += data->sizeofAttribute();
		attributes_.emplace_back(std::move(data));
		normal_data_ = std::shared_ptr<IAttributeData>(attributes_.back());
	}
	void addColorData(std::shared_ptr<IAttributeData> data)
	{
		color_index_ = attributes_.size();
		stride_ += data->sizeofAttribute();
		attributes_.emplace_back(std::move(data));
		color_data_ = std::shared_ptr<IAttributeData>(attributes_.back());
	}
	
	const std::vector<std::shared_ptr<IAttributeData>> &attributes() const { return attributes_; }
	const std::shared_ptr<IAttributeData> &positionData() const { return position_data_; }
	const std::shared_ptr<IAttributeData> &normalData() const { return normal_data_; }
	const std::shared_ptr<IAttributeData> &colorData() const { return color_data_; }
	bool hasPositionData() const { return (position_data_ != nullptr); }
	size_t positionIndex() const { return position_index_; }
	bool hasNormalData() const { return (normal_data_ != nullptr); }
	size_t normalIndex() const { return normal_index_; }
	bool hasColorData() const { return (color_data_ != nullptr); }
	size_t colorIndex() const { return color_index_; }
	
	size_t stride() const { return stride_; }
	size_t interleavedOffset(size_t index) const {
		if (index && attributes_.size()) { return (attributes_[--index]->sizeofAttribute()+interleavedOffset(index)); }
		return 0;
	}
	size_t sequentialOffset(size_t index) const {
		if (index) return (attributes_[--index]->sizeInBytes()+sequentialOffset(index));
		return 0;
	}
	size_t sizeInBytes() const { size_t size = 0; for (const auto &data : attributes_) size+=data->sizeInBytes(); return size; }
	size_t size() const { return sizeInBytes()/stride(); }

	std::unique_ptr<std::vector<GLbyte>> createInterleavedBuffer() const;
	void createInterleavedVBO(GLuint &vbo) const;
	
	std::unique_ptr<std::vector<GLbyte>> createSequentialBuffer() const;
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

	GLenum drawType() const { return draw_type_; }
	void drawType(GLenum draw_type) { draw_type_ = draw_type; }
	GLsizei drawSize() const { return draw_size_; }
	void drawSize(GLsizei draw_size) { draw_size_ = draw_size; }
	size_t drawOffset() const { return draw_offset_; }
	void drawOffset(size_t draw_offset) { draw_offset_ = draw_offset; }
	
	void drawArrays() const;
	
	// Mimic VAO state functionality.
	std::vector<std::function<void()>> &glBegin() { return gl_begin_; }
	std::vector<std::function<void()>> &glEnd() { return gl_end_; }

private:
	GLenum draw_type_;
	GLsizei draw_size_;
	size_t draw_offset_;
	std::vector<std::function<void()>> gl_begin_;
	std::vector<std::function<void()>> gl_end_;
};
typedef std::vector<std::shared_ptr<VertexState>> VertexStates;

// Allows Renderers to override VertexState objects with their own derived
// type. VertexArray will create the appropriate type for creating
// a VertexState object.
class VertexStateFactory {
public:
	VertexStateFactory() {}
	virtual ~VertexStateFactory() {}
	
	virtual std::shared_ptr<VertexState> createVertexState(GLenum draw_type, size_t draw_size, size_t draw_offset) const {
		return std::make_shared<VertexState>(draw_type, draw_size, draw_offset);
	}
};

// Used to combine vertex buffers with vertex states. Creates VBOs.
class VertexArray
{
public:
	VertexArray(std::unique_ptr<VertexStateFactory> factory, VertexStates &states)
		: factory_(std::move(factory)), states_(states), write_index_(0)
	{};
	virtual ~VertexArray() {};
	
	void addVertexData(std::shared_ptr<VertexData> data) { array_.emplace_back(std::move(data)); }
	VertexStates &states() { return states_; }
	VertexData &data() { return *(array_[write_index_].get()); }
	void writeIndex(size_t index) { if (index < array_.size()) write_index_ = index; }
	size_t writeIndex() const { return write_index_; }
	size_t indexOffset(size_t index) const {
		if (index > 0) return array_[--index]->sizeInBytes() + indexOffset(index);
		return 0;
	}
	std::shared_ptr<VertexState> createVertexState(GLenum draw_type, size_t draw_size, size_t draw_offset = 0) const {
		return factory_->createVertexState(draw_type, draw_size, draw_offset);
	}

	void createInterleavedVBO(GLuint &vbo) const;
	void createSequentialVBO(GLuint &vbo) const;
	void addAttributePointers(size_t start_offset = 0);

private:
	std::unique_ptr<VertexStateFactory> factory_;
	VertexStates &states_;
	size_t write_index_;
	std::vector<std::shared_ptr<VertexData>> array_;
};

#endif // __VERTEX_H__