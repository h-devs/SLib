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

#ifndef CHECKHEADER_SLIB_RENDER_BUFFER
#define CHECKHEADER_SLIB_RENDER_BUFFER

#include "base.h"

#include "../core/memory.h"

namespace slib
{

	class RenderBuffer;

	class SLIB_EXPORT RenderBufferInstance : public RenderBaseObjectInstance
	{
		SLIB_DECLARE_OBJECT

	protected:
		RenderBufferInstance();

		~RenderBufferInstance();

	public:
		virtual void notifyUpdated(RenderBuffer* buffer, sl_uint32 offset, sl_uint32 size);

	protected:
		sl_uint32 m_updatedOffset;
		sl_uint32 m_updatedSize;

	};

	class SLIB_EXPORT RenderBuffer : public RenderBaseObject
	{
		SLIB_DECLARE_OBJECT

	protected:
		RenderBuffer(sl_uint32 size);

		~RenderBuffer();

	public:
		sl_uint32 getSize();

		void update(sl_uint32 offset, sl_uint32 size);

		void update();

	public:
		virtual Memory getSource() = 0;

	protected:
		sl_uint32 m_size;

	};

	class SLIB_EXPORT VertexBufferInstance : public RenderBufferInstance
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		VertexBufferInstance();
		
		~VertexBufferInstance();
		
	};

	class SLIB_EXPORT VertexBuffer : public RenderBuffer
	{
		SLIB_DECLARE_OBJECT

	protected:
		VertexBuffer(sl_uint32 size);

		~VertexBuffer();

	public:
		static Ref<VertexBuffer> create(const Memory& mem);

		static Ref<VertexBuffer> create(const void* buf, sl_size size);

	public:
		Ref<VertexBufferInstance> getInstance(RenderEngine* engine);

	};

	class SLIB_EXPORT MemoryVertexBuffer : public VertexBuffer
	{
		SLIB_DECLARE_OBJECT

	public:
		MemoryVertexBuffer(const Memory& mem);

		~MemoryVertexBuffer();

	public:
		Memory getSource() override;

	protected:
		Memory m_mem;

	};
	
	class SLIB_EXPORT IndexBufferInstance : public RenderBufferInstance
	{
		SLIB_DECLARE_OBJECT

	protected:
		IndexBufferInstance();

		~IndexBufferInstance();

	};

	class SLIB_EXPORT IndexBuffer : public RenderBuffer
	{
		SLIB_DECLARE_OBJECT

	protected:
		IndexBuffer(sl_uint32 size);

		~IndexBuffer();

	public:
		static Ref<IndexBuffer> create(const Memory& mem);

		static Ref<IndexBuffer> create(const void* buf, sl_size size);

	public:
		Ref<IndexBufferInstance> getInstance(RenderEngine* engine);

	};

	class SLIB_EXPORT MemoryIndexBuffer : public IndexBuffer
	{
		SLIB_DECLARE_OBJECT

	public:
		MemoryIndexBuffer(const Memory& mem);

		~MemoryIndexBuffer();

	public:
		Memory getSource() override;

	protected:
		Memory m_mem;

	};

}

#endif

