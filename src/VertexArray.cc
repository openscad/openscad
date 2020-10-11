#include <iostream>
#include <vector>
#include <memory>
#include <cstdio>
#include <functional>
#include <enums.h>
#include <VertexArray.h>

#include <printutils.h>

void addAttributeValues(IAttributeData &) {}

void VertexData::fillInterleavedBuffer(GLbyte* interleaved_buffer) const
{
	// All attribute vectors need to be the same size to interleave
	size_t idx = 0, last_size = 0, stride = 0;
	for (const auto &data : attributes_) {
		if (idx != 0) {
			if (last_size != data->size() / data->count()) {
				PRINTDB("attribute data for vertex incorrect size at index %d = %d", idx % data->size());
				PRINTDB("last_size = %d", last_size);
				return;
			}
		}
		last_size = data->size() / data->count();
		idx++;
		stride += data->sizeofAttribute();
	}

	GLbyte* dst_start = interleaved_buffer;
	for (const auto &data : attributes_) {
		size_t size = data->sizeofAttribute();
		GLbyte* dst = dst_start;
		const GLbyte* src = data->toBytes();
		for (size_t i = 0; i < last_size; ++i) {
			std::memcpy((void *)dst, (void *)src, size);
			src += size;
			dst += stride;
		}
		dst_start += size;
	}
	PRINTDB("interleaved_buffer size = %d", last_size);
}

void VertexData::createInterleavedVBO(GLuint &vbo) const
{
  size_t total_bytes = this->sizeInBytes();
	if (total_bytes) {
		GLbyte* interleaved_buffer = new GLbyte[total_bytes];
		fillInterleavedBuffer(interleaved_buffer);
	
		if (vbo == 0) {
			glGenBuffers(1, &vbo);
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, total_bytes, interleaved_buffer, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		delete[] interleaved_buffer;
	}
}

void VertexData::fillSequentialBuffer(GLbyte *sequential_buffer) const
{
	GLbyte * dst = sequential_buffer;
	for (const auto &data : attributes_) {
		const size_t size = data->sizeInBytes();
		std::memcpy((void *)dst, (void *)data->toBytes(), size);
		dst += size;
	}
}

void VertexData::createSequentialVBO(GLuint &vbo) const
{
	size_t total_bytes = this->sizeInBytes();

	if (total_bytes) {
		GLbyte* sequential_buffer = new GLbyte[total_bytes];
		fillSequentialBuffer(sequential_buffer);

		if (vbo == 0) {
			glGenBuffers(1, &vbo);
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, total_bytes, sequential_buffer, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		delete[] sequential_buffer;
	}
}

void VertexState::drawArrays() const
{
	for (const auto &gl_func : gl_begin_) {
		gl_func();
	}
	if (draw_size_ > 0) {
		if (OpenSCAD::debug != "") PRINTDB("glDrawArrays(%s, 0, %d)",
						(draw_type_ == GL_TRIANGLES ? "GL_TRIANGLES" :
						 draw_type_ == GL_LINES ? "GL_LINES" :
						 draw_type_ == GL_LINE_LOOP ? "GL_LINE_LOOP" :
						 draw_type_ == GL_POINTS ? "GL_POINTS" :
						 "UNKNOWN") % draw_size_);
		glDrawArrays(draw_type_, 0, draw_size_);
	}
	for (const auto &gl_func : gl_end_) {
		gl_func();
	}
}

void VertexArray::addSurfaceData()
{
	std::shared_ptr<VertexData> vertex_data = std::make_shared<VertexData>();
	vertex_data->addPositionData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
	vertex_data->addNormalData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
	vertex_data->addColorData(std::make_shared<AttributeData<GLfloat,4,GL_FLOAT>>());
	surface_index_ = array_.size();
	addVertexData(vertex_data);
}

void VertexArray::addEdgeData()
{
	std::shared_ptr<VertexData> vertex_data = std::make_shared<VertexData>();
	vertex_data->addPositionData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
	vertex_data->addColorData(std::make_shared<AttributeData<GLfloat,4,GL_FLOAT>>());
	edge_index_ = array_.size();
	addVertexData(vertex_data);
}

void VertexArray::createInterleavedVBO(GLuint &vbo) const
{

	for (const auto &state : states_) {
		size_t index = state->drawOffset();
		state->drawOffset(this->indexOffset(index));
	}
	
	size_t total_bytes = this->indexOffset(this->size());
	if (total_bytes) {
		GLbyte* interleaved_buffer = new GLbyte[total_bytes];
		GLbyte* dst = interleaved_buffer;

		for (const auto &data : array_) {
			data->fillInterleavedBuffer(dst);
			dst += data->sizeInBytes();
		}

		if (vbo == 0) {
			glGenBuffers(1, &vbo);
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, total_bytes, interleaved_buffer, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		delete[] interleaved_buffer;
	}
}

void VertexArray::createSequentialVBO(GLuint &vbo) const
{
	for (const auto &state : states_) {
		size_t index = state->drawOffset();
		state->drawOffset(this->indexOffset(index));
	}

	size_t total_bytes = this->indexOffset(this->size());
	if (total_bytes) {
		GLbyte* sequential_buffer = new GLbyte[total_bytes];
		GLbyte* dst = sequential_buffer;
		for (const auto &data : array_) {
			size_t data_bytes = data->sizeInBytes();
			if (data_bytes) {
				data->fillSequentialBuffer(dst);
			}
			dst += data_bytes;
		}

		if (vbo == 0) {
			glGenBuffers(1, &vbo);
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, total_bytes, sequential_buffer, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		delete[] sequential_buffer;
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
	vs->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_VERTEX_ARRAY)"); glEnableClientState(GL_VERTEX_ARRAY); });
	vs->glBegin().emplace_back([count, type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
		auto vs = vs_ptr.lock();
		if (vs) {
			if (OpenSCAD::debug != "") PRINTDB("glVertexPointer(%d, %d, %d, %p)",
				count % type % stride % (GLvoid *)(vs->drawOffset() + offset));
			glVertexPointer(count, type, stride, (GLvoid *)(vs->drawOffset() + offset));
		}
	});
	vs->glEnd().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glDisableClientState(GL_VERTEX_ARRAY)"); glDisableClientState(GL_VERTEX_ARRAY); });
	
	if (vertex_data->hasNormalData()) {
		type = vertex_data->normalData()->glType();
		size_t offset = start_offset + vertex_data->interleavedOffset(vertex_data->normalIndex());
		vs->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_NORMAL_ARRAY)"); glEnableClientState(GL_NORMAL_ARRAY); });
		vs->glBegin().emplace_back([type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
			auto vs = vs_ptr.lock();
			if (vs) {
				if (OpenSCAD::debug != "") PRINTDB("glNormalPointer(%d, %d, %p)",
					type % stride % (GLvoid *)(vs->drawOffset() + offset));
				glNormalPointer(type, stride, (GLvoid *)(vs->drawOffset() + offset));
			}
		});
		vs->glEnd().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glDisableClientState(GL_NORMAL_ARRAY)"); glDisableClientState(GL_NORMAL_ARRAY); });
	}
	if (vertex_data->hasColorData()) {
		count = vertex_data->colorData()->count();
		type = vertex_data->colorData()->glType();
		size_t offset = start_offset + vertex_data->interleavedOffset(vertex_data->colorIndex());
		vs->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_COLOR_ARRAY)"); glEnableClientState(GL_COLOR_ARRAY); });
		vs->glBegin().emplace_back([count, type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
			auto vs = vs_ptr.lock();
			if (vs) {
				if (OpenSCAD::debug != "") PRINTDB("glColorPointer(%d, %d, %d, %p)",
					count % type % stride % (GLvoid *)(vs->drawOffset() + offset));
				glColorPointer(count, type, stride, (GLvoid *)(vs->drawOffset() + offset));
			}
		});
		vs->glEnd().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glDisableClientState(GL_COLOR_ARRAY)"); glDisableClientState(GL_COLOR_ARRAY); });
	}
}