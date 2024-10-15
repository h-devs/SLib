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

#include "slib/render/program_ext.h"
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
		m_shaders[(int)(RenderShaderType::GLSL_Vertex)] = glslVertexShader;
	}

	ShaderDrawable::~ShaderDrawable()
	{
	}

	String ShaderDrawable::getShader(RenderShaderType type)
	{
		return m_shaders[(int)type];
	}

	void ShaderDrawable::setShader(RenderShaderType type, const String& shader)
	{
		m_shaders[(int)type] = shader;
		m_program.setNull();
	}

	namespace
	{
		class ShaderDrawable_Program : public render2d::program::Position
		{
		public:
			String shaders[SLIB_RENDER_SHADER_TYPE_MAX];

		public:
			String getShader(RenderEngine* engine, RenderShaderType type) override
			{
				return shaders[(int)type];
			}
		};
	}

	void ShaderDrawable::onRender(RenderCanvas* canvas, const Rectangle& rectDst, const DrawParam& param)
	{
		Ref<RenderProgram> program = m_program;
		if (program.isNull()) {
			Ref<ShaderDrawable_Program> shaderProgram = new ShaderDrawable_Program;
			if (shaderProgram.isNull()) {
				return;
			}
			for (sl_size i = 0; i < SLIB_RENDER_SHADER_TYPE_MAX; i++) {
				shaderProgram->shaders[i] = m_shaders[i];
			}
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
