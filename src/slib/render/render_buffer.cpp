/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/render/buffer.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(RenderBufferInstance, RenderBaseObjectInstance)

	RenderBufferInstance::RenderBufferInstance()
	{
	}

	RenderBufferInstance::~RenderBufferInstance()
	{
	}

	void RenderBufferInstance::notifyUpdated(RenderBuffer* buffer, sl_uint32 offset, sl_uint32 size)
	{
		ObjectLocker lock(this);
		if (m_flagUpdated) {
			sl_uint32 oldLeft = m_updatedOffset;
			sl_uint32 oldRight = oldLeft + m_updatedSize;
			sl_uint32 newRight = offset + size;
			if (oldLeft < offset) {
				offset = oldLeft;
			}
			if (oldRight > newRight) {
				newRight = oldRight;
			}
			m_updatedOffset = offset;
			m_updatedSize = newRight - offset;
		} else {
			m_updatedOffset = offset;
			m_updatedSize = size;
			m_flagUpdated = sl_true;
		}
		tryUpdate(buffer);
	}


	SLIB_DEFINE_OBJECT(RenderBuffer, RenderBaseObject)

	RenderBuffer::RenderBuffer(sl_uint32 size)
	{
		m_size = size;
	}

	RenderBuffer::~RenderBuffer()
	{
	}

	sl_uint32 RenderBuffer::getSize()
	{
		return m_size;
	}

	void RenderBuffer::update(sl_uint32 offset, sl_uint32 size)
	{
		sl_uint32 total = m_size;
		if (offset >= total) {
			return;
		}
		if (size > total - offset) {
			size = total - offset;
		}
		for (int i = 0; i < SLIB_MAX_RENDER_ENGINE_COUNT_PER_OBJECT; i++) {
			Ref<RenderBaseObjectInstance> instance = m_instances[i];
			if (instance.isNotNull()) {
				((RenderBufferInstance*)(instance.get()))->notifyUpdated(this, offset, size);
			}
		}
	}

	void RenderBuffer::update()
	{
		update(0, getSize());
	}


	SLIB_DEFINE_OBJECT(VertexBufferInstance, RenderBufferInstance)
	
	VertexBufferInstance::VertexBufferInstance()
	{
	}
	
	VertexBufferInstance::~VertexBufferInstance()
	{
	}
	

	SLIB_DEFINE_OBJECT(VertexBuffer, RenderBuffer)

	VertexBuffer::VertexBuffer(sl_uint32 size) : RenderBuffer(size)
	{
	}

	VertexBuffer::~VertexBuffer()
	{
	}
	
	Ref<VertexBuffer> VertexBuffer::create(const Memory& mem)
	{
		if (mem.isNotNull()) {
			return new MemoryVertexBuffer(mem);
		}
		return sl_null;
	}
	
	Ref<VertexBuffer> VertexBuffer::create(const void* buf, sl_size size)
	{
		return create(Memory::create(buf, size));
	}

	Ref<VertexBufferInstance> VertexBuffer::getInstance(RenderEngine* engine)
	{
		return Ref<VertexBufferInstance>::from(RenderBuffer::getInstance(engine));
	}


	SLIB_DEFINE_OBJECT(MemoryVertexBuffer, VertexBuffer)

	MemoryVertexBuffer::MemoryVertexBuffer(const Memory& mem)
		: VertexBuffer((sl_uint32)(mem.getSize())), m_mem(mem)
	{
	}

	MemoryVertexBuffer::~MemoryVertexBuffer()
	{
	}

	Memory MemoryVertexBuffer::getSource()
	{
		return m_mem;
	}


	SLIB_DEFINE_OBJECT(IndexBufferInstance, RenderBufferInstance)

	IndexBufferInstance::IndexBufferInstance()
	{
	}

	IndexBufferInstance::~IndexBufferInstance()
	{
	}


	SLIB_DEFINE_OBJECT(IndexBuffer, RenderBuffer)

	IndexBuffer::IndexBuffer(sl_uint32 size) : RenderBuffer(size)
	{
	}

	IndexBuffer::~IndexBuffer()
	{
	}

	Ref<IndexBuffer> IndexBuffer::create(const Memory& mem)
	{
		if (mem.isNotNull()) {
			return new MemoryIndexBuffer(mem);
		}
		return sl_null;
	}

	Ref<IndexBuffer> IndexBuffer::create(const void* buf, sl_size size)
	{
		return create(Memory::create(buf, size));
	}

	Ref<IndexBufferInstance> IndexBuffer::getInstance(RenderEngine* engine)
	{
		return Ref<IndexBufferInstance>::from(RenderBuffer::getInstance(engine));
	}


	SLIB_DEFINE_OBJECT(MemoryIndexBuffer, IndexBuffer)

	MemoryIndexBuffer::MemoryIndexBuffer(const Memory& mem)
		: IndexBuffer((sl_uint32)(mem.getSize())), m_mem(mem)
	{
	}

	MemoryIndexBuffer::~MemoryIndexBuffer()
	{
	}

	Memory MemoryIndexBuffer::getSource()
	{
		return m_mem;
	}

}
