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

#ifndef CHECKHEADER_SLIB_RENDER_DRAWABLE
#define CHECKHEADER_SLIB_RENDER_DRAWABLE

#include "canvas.h"

#include "../graphics/drawable.h"
#include "../core/event_handler.h"

namespace slib
{

	class RenderDrawable : public Drawable
	{
		SLIB_DECLARE_OBJECT

	public:
		RenderDrawable();

		~RenderDrawable();

	public:
		void onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param) override;

	public:
		SLIB_DECLARE_EVENT_HANDLER(RenderDrawable, Render, RenderCanvas*, Rectangle const&, DrawParam const&)

	};

	class ShaderDrawable : public RenderDrawable
	{
		SLIB_DECLARE_OBJECT

	public:
		ShaderDrawable();

		~ShaderDrawable();

	public:
		String getShader(RenderShaderType type);

		void setShader(RenderShaderType type, const String& shader);

	protected:
		void onRender(RenderCanvas* canvas, const Rectangle& rectDst, const DrawParam& param) override;

	public:
		AtomicString m_shaders[SLIB_RENDER_SHADER_TYPE_MAX];
		AtomicRef<RenderProgram> m_program;

	};

}

#endif
