#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <memory>
#include <cstdio>
#include <functional>
#include "enums.h"
#include "VertexArray.h"

#include "printutils.h"

void addAttributeValues(IAttributeData &) {}

void VertexData::fillInterleavedBuffer(GLbyte* interleaved_buffer) const
{
	// All attribute vectors need to be the same size to interleave
	if (attributes_.size()) {
		size_t idx = 0;
		size_t last_size = attributes_[0]->size() / attributes_[0]->count();

		GLbyte* dst_start = interleaved_buffer;
		for (const auto &data : attributes_) {
			size_t size = data->sizeofAttribute();
			GLbyte* dst = dst_start;
			const GLbyte* src = data->toBytes();
			for (size_t i = 0; i < last_size; ++i) {
				std::memcpy((void *)dst, (void *)src, size);
				src += size;
				dst += stride_;
			}
			dst_start += size;
		}
	}
}

void VertexData::getLastVertex(std::vector<GLbyte> &interleaved_buffer) const
{
	GLbyte* dst_start = interleaved_buffer.data();
	for (const auto &data : attributes_) {
		size_t size = data->sizeofAttribute();
		GLbyte* dst = dst_start;
		const GLbyte* src = data->toBytes() + data->sizeInBytes() - data->sizeofAttribute();
		std::memcpy((void *)dst, (void *)src, size);
		dst_start += size;
	}
}

void VertexData::remove(size_t count)
{
	for (const auto &data : attributes_) {
		data->remove(count);
	}
}

void VertexData::createInterleavedVBO(GLuint &vbo) const
{
	size_t total_bytes = this->sizeInBytes();
	if (total_bytes) {
		std::vector<GLbyte> interleaved_buffer;
		interleaved_buffer.resize(total_bytes);
		fillInterleavedBuffer(interleaved_buffer.data());
	
		if (vbo == 0) {
			glGenBuffers(1, &vbo);
		}

		GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo); GL_ERROR_CHECK();
		GL_TRACE("glBufferData(GL_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", total_bytes % (void *)interleaved_buffer.data());
		glBufferData(GL_ARRAY_BUFFER, total_bytes, interleaved_buffer.data(), GL_STATIC_DRAW); GL_ERROR_CHECK();
		GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
		glBindBuffer(GL_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
	}
}

std::shared_ptr<VertexData> VertexData::create() const
{
	std::shared_ptr<VertexData> copy = std::make_shared<VertexData>();
	for (const auto &attribute : attributes_) {
		copy->addAttributeData(attribute->create());
	}
	if (position_data_) {
		copy->position_index_ = position_index_;
		copy->position_data_ = copy->attributes_[copy->position_index_];
	}
	if (normal_data_) {
		copy->normal_index_ = normal_index_;
		copy->normal_data_ = copy->attributes_[copy->normal_index_];
	}
	if (color_data_) {
		copy->color_index_ = color_index_;
		copy->color_data_ = copy->attributes_[copy->color_index_];
	}
	copy->stride_ = stride_;
	return copy;
}

void VertexData::append(const VertexData &data) {
	size_t i = 0;
	for (auto &a : attributes_) {
		a->append(*(data.attributes_[i]));
		i++;
	}
}

void VertexState::draw(bool bind_buffers) const
{
	if (vertices_vbo_ && bind_buffers) {
		GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertices_vbo_);
		glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_); GL_ERROR_CHECK();
	}
	if (elements_vbo_ && bind_buffers) {
		GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d)", elements_vbo_);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements_vbo_); GL_ERROR_CHECK();
	}
	for (const auto &gl_func : gl_begin_) {
		gl_func();
	}
	if (draw_size_ > 0) {
		if (elements_vbo_) {
			GL_TRACE("glDrawElements(%s, %d, %s, %d)",
				(draw_mode_ == GL_POINTS ? "GL_POINTS" :
				 draw_mode_ == GL_LINES ? "GL_LINES" :
				 draw_mode_ == GL_LINE_LOOP ? "GL_LINE_LOOP" :
				 draw_mode_ == GL_LINE_STRIP ? "GL_LINE_STRIP" :
				 draw_mode_ == GL_TRIANGLES ? "GL_TRIANGLES" :
				 draw_mode_ == GL_TRIANGLE_STRIP ? "GL_TRIANGLE_STRIP" :
				 draw_mode_ == GL_TRIANGLE_FAN ? "GL_TRIANGLE_FAN" :
				 draw_mode_ == GL_QUADS ? "GL_QUADS" :
				 draw_mode_ == GL_QUAD_STRIP ? "GL_QUAD_STRIP" :
				 draw_mode_ == GL_POLYGON ? "GL_POLYGON" :
				 "UNKNOWN") % draw_size_ % 
				(draw_type_ == GL_UNSIGNED_SHORT ? "GL_UNSIGNED_SHORT" :
			 	 draw_type_ == GL_UNSIGNED_INT ? "GL_UNSIGNED_INT" :
				 "UNKNOWN") % element_offset_);
			glDrawElements(draw_mode_, draw_size_, draw_type_, (GLvoid *)element_offset_);
		} else {
			GL_TRACE("glDrawArrays(%s, 0, %d)",
				(draw_mode_ == GL_POINTS ? "GL_POINTS" :
				 draw_mode_ == GL_LINES ? "GL_LINES" :
				 draw_mode_ == GL_LINE_LOOP ? "GL_LINE_LOOP" :
				 draw_mode_ == GL_LINE_STRIP ? "GL_LINE_STRIP" :
				 draw_mode_ == GL_TRIANGLES ? "GL_TRIANGLES" :
				 draw_mode_ == GL_TRIANGLE_STRIP ? "GL_TRIANGLE_STRIP" :
				 draw_mode_ == GL_TRIANGLE_FAN ? "GL_TRIANGLE_FAN" :
				 draw_mode_ == GL_QUADS ? "GL_QUADS" :
				 draw_mode_ == GL_QUAD_STRIP ? "GL_QUAD_STRIP" :
				 draw_mode_ == GL_POLYGON ? "GL_POLYGON" :
				 "UNKNOWN") % draw_size_);
			glDrawArrays(draw_mode_, 0, draw_size_);
		}
	}
	for (const auto &gl_func : gl_end_) {
		gl_func();
	}
	if (elements_vbo_ && bind_buffers) {
		GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
	}
	if (vertices_vbo_ && bind_buffers) {
		GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
		glBindBuffer(GL_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
	}
}

void VertexArray::addSurfaceData()
{
	std::shared_ptr<VertexData> vertex_data = std::make_shared<VertexData>();
	vertex_data->addPositionData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
	vertex_data->addNormalData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
	vertex_data->addColorData(std::make_shared<AttributeData<GLfloat,4,GL_FLOAT>>());
	surface_index_ = vertices_.size();
	addVertexData(vertex_data);
}

void VertexArray::addEdgeData()
{
	std::shared_ptr<VertexData> vertex_data = std::make_shared<VertexData>();
	vertex_data->addPositionData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
	vertex_data->addColorData(std::make_shared<AttributeData<GLfloat,4,GL_FLOAT>>());
	edge_index_ = vertices_.size();
	addVertexData(vertex_data);
}

std::shared_ptr<VertexArray> VertexArray::create() const {
	std::shared_ptr<VertexArray> copy = std::make_shared<VertexArray>(factory_, states_);
	copy->write_index_ = write_index_;
	copy->surface_index_ = surface_index_;
	copy->edge_index_ = edge_index_;
	copy->vertices_.reserve(vertices_.size());
	for (const auto &data : vertices_) {
		copy->vertices_.emplace_back(data->create());
	}
	return copy;
}

void VertexArray::append(const VertexArray &vertex_array)
{
	size_t i = 0;
	for (auto &v : vertices_) {
		v->append(*(vertex_array.vertices_[i]));
		i++;
	}
}

void VertexArray::createVertex(const std::array<Vector3d,3> &points,
				const std::array<Vector3d,3> &normals,
				const Color4f &color,
				size_t active_point_index, size_t primitive_index,
				double z_offset, size_t shape_size,
				size_t shape_dimensions, bool outlines,
				bool mirror, CreateVertexCallback vertex_callback)
{
	if (vertex_callback)
		vertex_callback(*this, points, normals, color, active_point_index,
			primitive_index, z_offset, shape_size,
			shape_dimensions, outlines, mirror);
	
	addAttributeValues(*(data()->positionData()), points[active_point_index][0], points[active_point_index][1], points[active_point_index][2]);
	if (data()->hasNormalData()) {
		addAttributeValues(*(data()->normalData()), normals[active_point_index][0], normals[active_point_index][1], normals[active_point_index][2]);
	}
	if (data()->hasColorData()) {
		addAttributeValues(*(data()->colorData()), color[0], color[1], color[2], color[3]);
	}

	if (use_elements_) {
		std::vector<GLbyte> interleaved_vertex;
		interleaved_vertex.resize(data()->stride());
		data()->getLastVertex(interleaved_vertex);
		std::pair<ElementsMap::iterator,bool> entry;
		entry.first = elements_map_.find(interleaved_vertex);
		if (entry.first == elements_map_.end()) {
			// append vertex data if this is a new element
			if (vertices_size_) {
				if (interleaved_buffer_.size()) {
					memcpy(interleaved_buffer_.data() + vertices_offset_,interleaved_vertex.data(),interleaved_vertex.size());
				} else {
					GL_TRACE("glBufferSubData(GL_ARRAY_BUFFER, %d, %d, %p)", vertices_offset_ % interleaved_vertex.size() % interleaved_vertex.data());
					glBufferSubData(GL_ARRAY_BUFFER, vertices_offset_, interleaved_vertex.size(), interleaved_vertex.data());
					GL_ERROR_CHECK();
				}
				data()->clear();
			}
			vertices_offset_ += interleaved_vertex.size();
			entry = elements_map_.emplace(interleaved_vertex, elements_map_.size());
		} else {
			data()->remove();
#if 0
			if (OpenSCAD::debug != "") {
				// in debug, check for bad hash matches
				size_t i = 0;
				if (interleaved_vertex.size() != entry.first->first.size()) {
					PRINTDB("vertex index = %d", entry.first->second);
					assert(false && "VBORenderer invalid vertex match size!!!");
				}
				for (const auto &b : interleaved_vertex) {
					if (b != entry.first->first[i]) {
						PRINTDB("vertex index = %d", entry.first->second);
						assert(false && "VBORenderer invalid vertex value hash match!!!");
					}
					i++;
				}
			}
#endif // 0
		}

		// append element data
		if (!elements_size_ || Feature::ExperimentalVxORenderersPrealloc.is_enabled()) {
			addAttributeValues(*elementsData(), entry.first->second);
		} else {
			if (elementsData()->sizeofAttribute() == sizeof(GLubyte)) {
				GLubyte index = (GLubyte)entry.first->second;
				GL_TRACE("glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, %d, %d, %p)", elements_offset_ % elementsData()->sizeofAttribute() % (void *)&index);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, elements_offset_,
						elementsData()->sizeofAttribute(),
						&index); GL_ERROR_CHECK();
			} else if (elementsData()->sizeofAttribute() == sizeof(GLushort)) {
				GLushort index = (GLushort)entry.first->second;
				GL_TRACE("glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, %d, %d, %p)", elements_offset_ % elementsData()->sizeofAttribute() % (void *)&index);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, elements_offset_,
						elementsData()->sizeofAttribute(),
						&index); GL_ERROR_CHECK();
			} else if (elementsData()->sizeofAttribute() == sizeof(GLuint)) {
				GLuint index = (GLuint)entry.first->second;
				GL_TRACE("glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, %d, %d, %p)", elements_offset_ % elementsData()->sizeofAttribute() % (void *)&index);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, elements_offset_,
						elementsData()->sizeofAttribute(),
						&index); GL_ERROR_CHECK();
			} else {
				assert(false && "create_vertex invalid index attribute size");
			}
		}
		elements_offset_ += elementsData()->sizeofAttribute();
	} else {
		if (!vertices_size_) {
			vertices_offset_ = sizeInBytes();
		} else {
			std::vector<GLbyte> interleaved_vertex;
			interleaved_vertex.resize(data()->stride());
			data()->getLastVertex(interleaved_vertex);
			if (interleaved_buffer_.size()) {
				memcpy(interleaved_buffer_.data() + vertices_offset_,interleaved_vertex.data(),interleaved_vertex.size());
			} else {
				GL_TRACE("glBufferSubData(GL_ARRAY_BUFFER, %d, %d, %p)", vertices_offset_ % interleaved_vertex.size() % interleaved_vertex.data());
				glBufferSubData(GL_ARRAY_BUFFER, vertices_offset_, interleaved_vertex.size(), interleaved_vertex.data());
				GL_ERROR_CHECK();
			}
			vertices_offset_ += interleaved_vertex.size();
			data()->clear();
		}
	}
}

void VertexArray::fillInterleavedBuffer(std::vector<GLbyte> &interleaved_buffer) const
{
	size_t total_bytes = this->sizeInBytes();
	if (total_bytes) {
		interleaved_buffer.resize(total_bytes);
		GLbyte* dst = interleaved_buffer.data();

		for (const auto &data : vertices_) {
			data->fillInterleavedBuffer(dst);
			dst += data->sizeInBytes();
		}
	}
}

void VertexArray::createInterleavedVBOs()
{
	for (const auto &state : states_) {
		size_t index = state->drawOffset();
		state->drawOffset(this->indexOffset(index));
	}
	
	// If the upfront size was not known, the the buffer has to be built
	size_t total_size = this->sizeInBytes();
	// If VertexArray is not empty, and initial size is zero
	if (!vertices_size_ && total_size) {
		GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertices_vbo_);
		glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_); GL_ERROR_CHECK();
		GL_TRACE("glBufferData(GL_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", total_size % (void *)nullptr);
		glBufferData(GL_ARRAY_BUFFER, total_size, nullptr, GL_STATIC_DRAW); GL_ERROR_CHECK();
		
		size_t dst_start = 0;
		for (const auto &vertex_data : vertices_) {
			// All attribute vectors need to be the same size to interleave
			size_t idx = 0, last_size = 0, stride = vertex_data->stride(), dst = dst_start;
			for (const auto &data : vertex_data->attributes()) {
				size_t size = data->sizeofAttribute();
				const GLbyte* src = data->toBytes();
				dst = dst_start;

				if (src) {
					if (idx != 0) {
						if (last_size != data->size() / data->count()) {
							PRINTDB("attribute data for vertex incorrect size at index %d = %d", idx % (data->size() / data->count()));
							PRINTDB("last_size = %d", last_size);
							assert(false);
						}
					}
					last_size = data->size() / data->count();
					for (size_t i = 0; i < last_size; ++i) {
						GL_TRACE("glBufferSubData(GL_ARRAY_BUFFER, %p, %d, %p)", (void *)dst % size % (void *)src);
						glBufferSubData(GL_ARRAY_BUFFER, dst, size, src); GL_ERROR_CHECK();
						src += size;
						dst += stride;
					}
					dst_start += size;
				}
				idx++;
			}
			dst_start = vertex_data->sizeInBytes();
		}

		GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
		glBindBuffer(GL_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
	} else if (vertices_size_ && interleaved_buffer_.size()) {
		GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertices_vbo_);
		glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_); GL_ERROR_CHECK();
		GL_TRACE("glBufferData(GL_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", interleaved_buffer_.size() % (void *)interleaved_buffer_.data());
		glBufferData(GL_ARRAY_BUFFER, interleaved_buffer_.size(), interleaved_buffer_.data(), GL_STATIC_DRAW); GL_ERROR_CHECK();
		GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
		glBindBuffer(GL_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
	}
	
	PRINTDB("use_elements_ = %d, elements_size_ = %d", use_elements_ % elements_size_);
	if (use_elements_ && (!elements_size_ || Feature::ExperimentalVxORenderersPrealloc.is_enabled())) {
		GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d)", elements_vbo_);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements_vbo_); GL_ERROR_CHECK();
		if (!Feature::ExperimentalVxORenderersPrealloc.is_enabled()) {
			GL_TRACE("glBufferData(GL_ELEMENT_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", elements_.sizeInBytes() % (void *)nullptr);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_.sizeInBytes(), nullptr, GL_STATIC_DRAW); GL_ERROR_CHECK();
		}
		size_t last_size = 0;
		for (const auto &e : elements_.attributes()) {
			GL_TRACE("glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, %d, %d, %p)", last_size % e->sizeInBytes() % (void *)e->toBytes());
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, last_size, e->sizeInBytes(), e->toBytes()); GL_ERROR_CHECK();
			last_size += e->sizeInBytes();
		}
		GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
	}
}

void VertexArray::addAttributePointers(size_t start_offset)
{
	if (!this->data()) return;
	
	std::shared_ptr<VertexData> vertex_data = this->data();
	std::shared_ptr<VertexState> vs = this->states().back();

	GLsizei count = vertex_data->positionData()->count();
	GLenum type = vertex_data->positionData()->glType();
	GLsizei stride = vertex_data->stride();
	size_t offset = start_offset + vertex_data->interleavedOffset(vertex_data->positionIndex());
	vs->glBegin().emplace_back([]() { GL_TRACE0("glEnableClientState(GL_VERTEX_ARRAY)"); glEnableClientState(GL_VERTEX_ARRAY); GL_ERROR_CHECK(); });
	vs->glBegin().emplace_back([count, type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
		auto vs = vs_ptr.lock();
		if (vs) {
			GL_TRACE("glVertexPointer(%d, %d, %d, %p)",
				count % type % stride % (GLvoid *)(vs->drawOffset() + offset));
			glVertexPointer(count, type, stride, (GLvoid *)(vs->drawOffset() + offset));
			GL_ERROR_CHECK();
		}
	});
	vs->glEnd().emplace_back([]() { GL_TRACE0("glDisableClientState(GL_VERTEX_ARRAY)"); glDisableClientState(GL_VERTEX_ARRAY); GL_ERROR_CHECK(); });
	
	if (vertex_data->hasNormalData()) {
		type = vertex_data->normalData()->glType();
		size_t offset = start_offset + vertex_data->interleavedOffset(vertex_data->normalIndex());
		vs->glBegin().emplace_back([]() { GL_TRACE0("glEnableClientState(GL_NORMAL_ARRAY)"); glEnableClientState(GL_NORMAL_ARRAY); GL_ERROR_CHECK(); });
		vs->glBegin().emplace_back([type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
			auto vs = vs_ptr.lock();
			if (vs) {
				GL_TRACE("glNormalPointer(%d, %d, %p)", type % stride % (GLvoid *)(vs->drawOffset() + offset));
				glNormalPointer(type, stride, (GLvoid *)(vs->drawOffset() + offset));
				GL_ERROR_CHECK();
			}
		});
		vs->glEnd().emplace_back([]() { GL_TRACE0("glDisableClientState(GL_NORMAL_ARRAY)"); glDisableClientState(GL_NORMAL_ARRAY); GL_ERROR_CHECK(); });
	}
	if (vertex_data->hasColorData()) {
		count = vertex_data->colorData()->count();
		type = vertex_data->colorData()->glType();
		size_t offset = start_offset + vertex_data->interleavedOffset(vertex_data->colorIndex());
		vs->glBegin().emplace_back([]() { GL_TRACE0("glEnableClientState(GL_COLOR_ARRAY)"); glEnableClientState(GL_COLOR_ARRAY); GL_ERROR_CHECK(); });
		vs->glBegin().emplace_back([count, type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
			auto vs = vs_ptr.lock();
			if (vs) {
				GL_TRACE("glColorPointer(%d, %d, %d, %p)", count % type % stride % (GLvoid *)(vs->drawOffset() + offset));
				glColorPointer(count, type, stride, (GLvoid *)(vs->drawOffset() + offset));
				GL_ERROR_CHECK();
			}
		});
		vs->glEnd().emplace_back([]() { GL_TRACE0("glDisableClientState(GL_COLOR_ARRAY)"); glDisableClientState(GL_COLOR_ARRAY); GL_ERROR_CHECK(); });
	}
}