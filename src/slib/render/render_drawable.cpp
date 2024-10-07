/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/render/drawable.h"

#include "slib/core/stringify.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(RenderDrawable, Drawable)

	RenderDrawable::RenderDrawable()
	{
	}

	RenderDrawable::~RenderDrawable()
	{
	}

	void RenderDrawable::onDrawAll(Canvas* _canvas, const Rectangle& rectDst, const DrawParam& param)
	{
		RenderCanvas* canvas = CastInstance<RenderCanvas>(_canvas);
		if (canvas) {
			invokeRender(canvas, rectDst, param);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(RenderDrawable, Render, (RenderCanvas* canvas, const Rectangle& rectDst, const Drawable::DrawParam& param), canvas, rectDst, param)


	SLIB_DEFINE_OBJECT(ShaderDrawable, Drawable)

	ShaderDrawable::ShaderDrawable()
	{
		SLIB_STATIC_STRING(glslVertexShader, SLIB_STRINGIFY(
			uniform mat3 u_Transform;
			uniform vec4 u_Color;
			attribute vec2 a_Position;
			varying vec2 v_Position;
			void main() {
				vec3 P = vec3(a_Position.x, a_Position.y, 1.0) * u_Transform;
				gl_Position = vec4(P.x, P.y, 0.0, 1.0);
				v_Position = a_Position;
			}
		))
		m_glslVertexShader = glslVertexShader;
	}

	ShaderDrawable::~ShaderDrawable()
	{
	}

	String ShaderDrawable::getGLSLVertexShader()
	{
		return m_glslVertexShader;
	}

	void ShaderDrawable::setGLSLVertexShader(const String& shader)
	{
		m_glslVertexShader = shader;
		m_program.setNull();
	}

	String ShaderDrawable::getGLSLFragmentShader()
	{
		return m_glslFragmentShader;
	}

	void ShaderDrawable::setGLSLFragmentShader(const String& shader)
	{
		m_glslFragmentShader = shader;
		m_program.setNull();
	}

	String ShaderDrawable::getHLSLVertexShader()
	{
		return m_hlslVertexShader;
	}

	void ShaderDrawable::setHLSLVertexShader(const String& shader)
	{
		m_hlslVertexShader = shader;
		m_program.setNull();
	}

	String ShaderDrawable::getHLSLPixelShader()
	{
		return m_hlslPixelShader;
	}

	void ShaderDrawable::setHLSLPixelShader(const String& shader)
	{
		m_hlslPixelShader = shader;
		m_program.setNull();
	}

	namespace
	{
		class ShaderDrawable_Program : public render2d::program::Position
		{
		public:
			String GLSLVertexShader;
			String GLSLFragmentShader;
			String HLSLVertexShader;
			String HLSLPixelShader;

		public:
			String getGLSLVertexShader(RenderEngine* engine) override
			{
				return GLSLVertexShader;
			}

			String getGLSLFragmentShader(RenderEngine* engine) override
			{
				return GLSLFragmentShader;
			}

			String getHLSLVertexShader(RenderEngine* engine) override
			{
				return HLSLVertexShader;
			}

			String getHLSLPixelShader(RenderEngine* engine) override
			{
				return HLSLPixelShader;
			}
		};
	}

	void ShaderDrawable::onRender(RenderCanvas* canvas, const Rectangle& rectDst, const DrawParam& param)
	{
		Ref<render2d::program::Position> program = m_program;
		if (program.isNull()) {
			Ref<ShaderDrawable_Program> shaderProgram = new ShaderDrawable_Program;
			if (shaderProgram.isNull()) {
				return;
			}
			shaderProgram->GLSLVertexShader = m_glslVertexShader;
			shaderProgram->GLSLFragmentShader = m_glslFragmentShader;
			shaderProgram->HLSLVertexShader = m_hlslVertexShader;
			shaderProgram->HLSLPixelShader = m_hlslPixelShader;
			m_program = shaderProgram;
			program = Move(shaderProgram);
		}
		Ref<RenderEngine> engine = canvas->getEngine();
		if (engine.isNull()) {
			return;
		}
		RenderProgramScope<render2d::state::Position> scope;
		if (scope.begin(engine, program)) {
			canvas->drawRectangle(rectDst, scope.getState(), param);
		}
	}

}
