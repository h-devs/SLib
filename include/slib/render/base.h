/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_RENDER_BASE
#define CHECKHEADER_SLIB_RENDER_BASE

#include "definition.h"

#include "../core/object.h"

namespace slib
{

	class RenderObjectFlags
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(RenderObjectFlags, value)

		enum {
			StaticDraw = 1, // OpenGL

			CpuAccessRead = 0x10000,
			CpuAccessWrite = 0x20000
		};
	};

	class RenderEngine;
	class RenderBaseObject;
	
	class SLIB_EXPORT RenderBaseObjectInstance : public Object
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		RenderBaseObjectInstance();
		
		~RenderBaseObjectInstance();
		
	public:
		void link(RenderEngine* engine, RenderBaseObject* object);
		
		Ref<RenderEngine> getEngine();

	protected:
		virtual void onUpdate(RenderBaseObject* object);

	public:
		void doUpdate(RenderBaseObject* object);

		sl_bool isUpdated();
		
	protected:
		WeakRef<RenderEngine> m_engine;
		sl_bool m_flagUpdated;
		
		friend class RenderBaseObject;
	};

	class SLIB_EXPORT RenderBaseObject : public Object
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		RenderBaseObject();
		
		~RenderBaseObject();
		
	public:
		Ref<RenderBaseObjectInstance> getInstance(RenderEngine* engine);

		RenderObjectFlags getFlags();

		void setFlags(const RenderObjectFlags& flags);

	protected:
		AtomicRef<RenderBaseObjectInstance> m_instance;
		RenderObjectFlags m_flags;

		friend class RenderBaseObjectInstance;
	};

}

#endif
