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


#include "slib/render/program_ext.h"

#include "slib/render/engine.h"
#include "slib/core/stringify.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(RenderProgramInstance, RenderBaseObjectInstance)

	RenderProgramInstance::RenderProgramInstance()
	{
	}

	RenderProgramInstance::~RenderProgramInstance()
	{
	}


	SLIB_DEFINE_OBJECT(RenderProgram, RenderBaseObject)

	RenderProgram::RenderProgram()
	{
	}

	RenderProgram::~RenderProgram()
	{
	}

	sl_bool RenderProgram::onInit(RenderEngine* engine, RenderProgramInstance* instance, RenderProgramState* state)
	{
		return sl_true;
	}

	sl_bool RenderProgram::onPreRender(RenderEngine* engine, RenderProgramInstance* instance, RenderProgramState* state)
	{
		if (engine->isInputLayoutAvailable()) {
			state->updateInputLayout(this);
			engine->setInputLayout(state->getInputLayout());
		}
		return sl_true;
	}

	void RenderProgram::onPostRender(RenderEngine* engine, RenderProgramInstance* instance, RenderProgramState* state)
	{
	}

	sl_bool RenderProgram::getInputLayoutParam(RenderProgramState* state, RenderInputLayoutParam& param)
	{
		return sl_false;
	}

	String RenderProgram::getShader(RenderEngine* engine, RenderShaderType type)
	{
		return sl_null;
	}

	Memory RenderProgram::getCompiledShader(RenderEngine* engine, RenderShaderType type)
	{
		return sl_null;
	}

	sl_uint32 RenderProgram::getVertexShaderConstantBufferCount()
	{
		return 1;
	}

	sl_uint32 RenderProgram::getVertexShaderConstantBufferSize(sl_uint32 bufferNo)
	{
		return 128;
	}

	sl_uint32 RenderProgram::getPixelShaderConstantBufferCount()
	{
		return 1;
	}

	sl_uint32 RenderProgram::getPixelShaderConstantBufferSize(sl_uint32 bufferNo)
	{
		return 128;
	}

	Ref<RenderProgramInstance> RenderProgram::getInstance(RenderEngine* engine)
	{
		return Ref<RenderProgramInstance>::cast(RenderBaseObject::getInstance(engine));
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RenderProgramStateItem)

	RenderProgramStateItem::RenderProgramStateItem() : name(sl_null), kind(RenderProgramStateKind::None)
	{
	}

	RenderProgramStateItem::RenderProgramStateItem(const char* _name) : name(_name), kind(RenderProgramStateKind::Uniform)
	{
		uniform.shader = RenderShaderStage::Undefined;
		uniform.location = -1;
		uniform.registerNo = -1;
	}

	RenderProgramStateItem::RenderProgramStateItem(const char* _name, RenderShaderStage _shaderType, sl_int32 _registerNo, sl_uint32 _bufferNo) : name(_name), kind(RenderProgramStateKind::Uniform)
	{
		uniform.shader = _shaderType;
		uniform.location = -1;
		uniform.registerNo = _registerNo;
		uniform.bufferNo = _bufferNo;
	}

	RenderProgramStateItem::RenderProgramStateItem(const char* _name, RenderInputType type, sl_uint32 offset, RenderInputSemanticName semanticName, sl_uint32 semanticIndex, sl_uint32 slot) : name(_name), kind(RenderProgramStateKind::Input)
	{
		input.type = type;
		input.offset = offset;
		input.semanticName = semanticName;
		input.semanticIndex = semanticIndex;
		input.slot = slot;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RenderInputLayoutParam)

	RenderInputLayoutParam::RenderInputLayoutParam()
	{
	}

	SLIB_DEFINE_ROOT_OBJECT(RenderInputLayout)

	RenderInputLayout::RenderInputLayout()
	{
	}

	RenderInputLayout::~RenderInputLayout()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(RenderProgramState)

	RenderProgramState::RenderProgramState()
	{
		m_programInstance = sl_null;
	}

	RenderProgramState::~RenderProgramState()
	{
	}

	RenderProgramInstance* RenderProgramState::getProgramInstance()
	{
		return m_programInstance;
	}

	void RenderProgramState::setProgramInstance(RenderProgramInstance* instance)
	{
		m_programInstance = instance;
	}

	RenderInputLayout* RenderProgramState::getInputLayout()
	{
		return m_inputLayout;
	}

	void RenderProgramState::updateInputLayout(RenderProgram* program, sl_bool forceUpdate)
	{
		RenderProgramInstance* instance = m_programInstance;
		if (!instance) {
			return;
		}
		if (!forceUpdate) {
			if (m_inputLayout.isNotNull()) {
				return;
			}
		}
		RenderInputLayoutParam param;
		if (program->getInputLayoutParam(this, param)) {
			m_inputLayout = instance->createInputLayout(param);
		}
	}

	sl_bool RenderProgramState::getUniformLocation(const char* name, RenderUniformLocation* outLocation)
	{
		RenderProgramInstance* instance = m_programInstance;
		if (instance) {
			return instance->getUniformLocation(name, outLocation);
		}
		return sl_false;
	}

	void RenderProgramState::setUniform(const RenderUniformLocation& location, RenderUniformType type, const void* data, sl_uint32 nItems)
	{
		RenderProgramInstance* instance = m_programInstance;
		if (instance) {
			instance->setUniform(location, type, data, nItems);
		}
	}

	void RenderProgramState::setFloatValue(const RenderUniformLocation& location, float value)
	{
		setUniform(location, RenderUniformType::Float, &value, 1);
	}

	void RenderProgramState::setFloatArray(const RenderUniformLocation& location, const float* arr, sl_uint32 n)
	{
		setUniform(location, RenderUniformType::Float, arr, n);
	}

	void RenderProgramState::setIntValue(const RenderUniformLocation& location, sl_int32 value)
	{
		setUniform(location, RenderUniformType::Int, &value, 1);
	}

	void RenderProgramState::setIntArray(const RenderUniformLocation& location, const sl_int32* arr, sl_uint32 n)
	{
		setUniform(location, RenderUniformType::Int, arr, n);
	}

	void RenderProgramState::setFloat2Value(const RenderUniformLocation& location, const Vector2& value)
	{
		setUniform(location, RenderUniformType::Float2, &value, 1);
	}

	void RenderProgramState::setFloat2Array(const RenderUniformLocation& location, const Vector2* arr, sl_uint32 n)
	{
		setUniform(location, RenderUniformType::Float2, arr, n);
	}

	void RenderProgramState::setFloat3Value(const RenderUniformLocation& location, const Vector3& value)
	{
		setUniform(location, RenderUniformType::Float3, &value, 1);
	}

	void RenderProgramState::setFloat3Array(const RenderUniformLocation& location, const Vector3* arr, sl_uint32 n)
	{
		setUniform(location, RenderUniformType::Float3, arr, n);
	}

	void RenderProgramState::setFloat4Value(const RenderUniformLocation& location, const Vector4& value)
	{
		setUniform(location, RenderUniformType::Float4, &value, 1);
	}

	void RenderProgramState::setFloat4Array(const RenderUniformLocation& location, const Vector4* arr, sl_uint32 n)
	{
		setUniform(location, RenderUniformType::Float4, arr, n);
	}

	void RenderProgramState::setMatrix3Value(const RenderUniformLocation& location, const Matrix3& value)
	{
		setUniform(location, RenderUniformType::Matrix3, &value, 1);
	}

	void RenderProgramState::setMatrix3Array(const RenderUniformLocation& location, const Matrix3* arr, sl_uint32 n)
	{
		setUniform(location, RenderUniformType::Matrix3, arr, n);
	}

	void RenderProgramState::setMatrix4Value(const RenderUniformLocation& location, const Matrix4& value)
	{
		setUniform(location, RenderUniformType::Matrix4, &value, 1);
	}

	void RenderProgramState::setMatrix4Array(const RenderUniformLocation& location, const Matrix4* arr, sl_uint32 n)
	{
		setUniform(location, RenderUniformType::Matrix4, arr, n);
	}

	void RenderProgramState::setTextureValue(const RenderUniformLocation& location, const Ref<Texture>& texture)
	{
		RenderProgramInstance* instance = m_programInstance;
		if (instance) {
			Ref<RenderEngine> engine = instance->getEngine();
			if (engine.isNotNull()) {
				engine->applyTexture(texture, location.registerNo);
				setUniform(location, RenderUniformType::Sampler, &(location.registerNo), 1);
			}
		}
	}

	namespace
	{
		class RenderProgramStateTemplate : public RenderProgramState
		{
		public:
			sl_uint32 vertexSize;
			List<RenderInputLayoutItem> inputLayout;
			RenderProgramStateItem items[1];
		};
	}

	namespace priv
	{
		sl_bool RenderProgramTemplate::onInit(RenderEngine* engine, RenderProgramInstance* instance, RenderProgramState* _state)
		{
			RenderProgramStateTemplate* state = (RenderProgramStateTemplate*)_state;
			List<RenderInputLayoutItem> layouts;
			RenderProgramStateItem* item = state->items;
			while (item->kind != RenderProgramStateKind::None) {
				if (item->kind == RenderProgramStateKind::Uniform) {
					if (item->name) {
						state->getUniformLocation(item->name, &(item->uniform));
					}
				} else if (item->kind == RenderProgramStateKind::Input) {
					RenderInputLayoutItem layoutItem;
					*((RenderInputDesc*)&layoutItem) = item->input;
					layoutItem.name = item->name;
					state->inputLayout.add_NoLock(layoutItem);
				}
				item++;
			}
			return sl_true;
		}

		sl_bool RenderProgramTemplate::getInputLayoutParam(RenderProgramState* _state, RenderInputLayoutParam& param)
		{
			RenderProgramStateTemplate* state = (RenderProgramStateTemplate*)_state;
			param.strides.add(state->vertexSize);
			param.items = state->inputLayout;
			return sl_true;
		}
	}

	namespace render2d
	{
		namespace program
		{
			String PositionTexture::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat3 u_Transform;
							uniform mat3 u_TextureTransform;
							attribute vec2 a_Position;
							attribute vec2 a_TexCoord;
							varying vec2 v_TexCoord;
							void main() {
								vec3 P = vec3(a_Position.x, a_Position.y, 1.0) * u_Transform;
								gl_Position = vec4(P.x, P.y, 0.0, 1.0);
								vec3 t = vec3(a_TexCoord, 1.0) * u_TextureTransform;
								v_TexCoord = t.xy;
							}
						))
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform vec4 u_Color;
							uniform sampler2D u_Texture;
							varying vec2 v_TexCoord;
							void main() {
								vec4 colorTexture = texture2D(u_Texture, v_TexCoord);
								gl_FragColor = colorTexture * u_Color;
							}
						))
					case RenderShaderType::HLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float3x3 u_Transform : register(c0);
							float3x3 u_TextureTransform : register(c3);
							struct VS_OUTPUT {
								float2 texcoord : TEXCOORD;
								float4 pos : POSITION;
							};
							VS_OUTPUT main(float2 a_Position : POSITION, float2 a_TexCoord : TEXCOORD) {
								VS_OUTPUT ret;
								float3 P = mul(float3(a_Position.x, a_Position.y, 1.0), u_Transform);
								ret.pos = float4(P.x, P.y, 0.0, 1.0);
								float3 t = mul(float3(a_TexCoord, 1.0), u_TextureTransform);
								ret.texcoord = t.xy;
								return ret;
							}
						))
					case RenderShaderType::HLSL_Pixel:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float4 u_Color;
							sampler u_Texture;
							float4 main(float2 v_TexCoord : TEXCOORD) : COLOR{
								float4 colorTexture = tex2D(u_Texture, v_TexCoord);
								return colorTexture * u_Color;
							}
						))
					case RenderShaderType::Assembly_Vertex:
						SLIB_RETURN_STRING(
							"vs.1.0\n"
							"def c50, 1.0f, 0.0f, 0.0f, 1.0f\n"
							"mov r0.xy, v0.xy\n"
							"mov r0.z, c50.x\n"
							"m3x3 r1, r0, c0\n"
							"mov r1.zw, c50.zw\n"
							"mov oPos, r1\n"
							"mov r0.xy, v1.xy\n"
							"m3x3 r1, r0, c3\n"
							"mov r1.zw, c50.zw\n"
							"mov oT0, r1\n"
						)
					case RenderShaderType::Assembly_Pixel:
						SLIB_RETURN_STRING(
							"ps.1.0\n"
							"tex t0\n"
							"mul r0, t0, c0\n"
						)
					default:
						break;
				}
				return sl_null;
			}

			String PositionTextureYUV::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform vec4 u_Color;
							uniform sampler2D u_Texture;
							varying vec2 v_TexCoord;
							void main() {
								vec4 YUV = texture2D(u_Texture, v_TexCoord);
								float R = YUV.r + 1.370705*(YUV.b - 0.5);
								float G = YUV.r - 0.698001*(YUV.g - 0.5) - 0.337633*(YUV.b - 0.5);
								float B = YUV.r + 1.732446*(YUV.g - 0.5);
								gl_FragColor = vec4(R, G, B, YUV.a) * u_Color;
							}
						))
					case RenderShaderType::HLSL_Pixel:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float4 u_Color;
							sampler u_Texture;
							float4 main(float2 v_TexCoord : TEXCOORD) : COLOR{
								float4 YUV = tex2D(u_Texture, v_TexCoord);
								float R = YUV.r + 1.370705*(YUV.b - 0.5);
								float G = YUV.r - 0.698001*(YUV.g - 0.5) - 0.337633*(YUV.b - 0.5);
								float B = YUV.r + 1.732446*(YUV.g - 0.5);
								return float4(R, G, B, YUV.a) * u_Color;
							}
						))
					default:
						break;
				}
				return PositionTexture::getShader(engine, type);
			}

			String PositionTextureOES::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(
							"#extension GL_OES_EGL_image_external : require\n"
							SLIB_STRINGIFY(
								precision mediump float;
								uniform vec4 u_Color;
								uniform samplerExternalOES u_Texture;
								varying vec2 v_TexCoord;
								void main() {
									vec4 colorTexture = texture2D(u_Texture, v_TexCoord);
									gl_FragColor = colorTexture * u_Color;
								}
							)
						)
					default:
						break;
				}
				return PositionTexture::getShader(engine, type);
			}

			String PositionColor::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat3 u_Transform;
							uniform vec4 u_Color;
							attribute vec2 a_Position;
							attribute vec4 a_Color;
							varying vec4 v_Color;
							void main() {
								vec3 P = vec3(a_Position.x, a_Position.y, 1.0) * u_Transform;
								gl_Position = vec4(P.x, P.y, 0.0, 1.0);
								v_Color = a_Color * u_Color;
							}
						))
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							varying vec4 v_Color;
							void main() {
								gl_FragColor = v_Color;
							}
						))
					case RenderShaderType::HLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float3x3 u_Transform : register(c0);
							float4 u_Color : register(c3);
							struct VS_OUTPUT {
								float4 color : COLOR;
								float4 pos : POSITION;
							};
							VS_OUTPUT main(in float2 a_Position : POSITION, in float4 a_Color : COLOR) {
								VS_OUTPUT output;
								float3 P = mul(float3(a_Position.x, a_Position.y, 1.0), u_Transform);
								output.pos = float4(P.x, P.y, 0.0, 1.0);
								output.color = u_Color * a_Color;
								return output;
							}
						))
					case RenderShaderType::HLSL_Pixel:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float4 main(in float4 v_Color : COLOR) : COLOR{
								return v_Color;
							}
						))
					case RenderShaderType::Assembly_Vertex:
						SLIB_RETURN_STRING(
							"vs.1.0\n"
							"def c50, 1.0f, 0.0f, 0.0f, 1.0f\n"
							"mov r0.xy, v0.xy\n"
							"mov r0.z, c50.x\n"
							"m3x3 r1, r0, c0\n"
							"mov r1.zw, c50.zw\n"
							"mov oPos, r1\n"
							"mul oD0, c3, v1\n"
						)
					case RenderShaderType::Assembly_Pixel:
						SLIB_RETURN_STRING(
							"ps.1.0\n"
							"mov r0, v0\n"
						)
					default:
						break;
				}
				return sl_null;
			}

			String Position::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat3 u_Transform;
							attribute vec2 a_Position;
							void main() {
								vec3 P = vec3(a_Position.x, a_Position.y, 1.0) * u_Transform;
								gl_Position = vec4(P.x, P.y, 0.0, 1.0);
							}
						))
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform vec4 u_Color;
							void main() {
								gl_FragColor = u_Color;
							}
						))
					case RenderShaderType::HLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float3x3 u_Transform;
							float4 main(in float2 a_Position : POSITION) : POSITION{
								float3 P = mul(float3(a_Position.x, a_Position.y, 1.0), u_Transform);
								return float4(P.x, P.y, 0.0, 1.0);
							}
						))
					case RenderShaderType::HLSL_Pixel:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float4 u_Color;
							float4 main() : COLOR{
								return u_Color;
							}
						))
					case RenderShaderType::Assembly_Vertex:
						SLIB_RETURN_STRING(
							"vs.1.0\n"
							"def c50, 1.0f, 0.0f, 0.0f, 1.0f\n"
							"mov r0.xy, v0.xy\n"
							"mov r0.z, c50.x\n"
							"m3x3 r1, r0, c0\n"
							"mov r1.zw, c50.zw\n"
							"mov oPos, r1\n"
						)
					case RenderShaderType::Assembly_Pixel:
						SLIB_RETURN_STRING(
							"ps.1.0\n"
							"mov r0, c0\n"
						)
					default:
						break;
				}
				return sl_null;
			}

			String HatchFill::getShader(RenderEngine* engine, RenderShaderType type)
			{
				return getShader(type, m_style);
			}

			String HatchFill::getShader(RenderShaderType type, HatchStyle style)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat3 u_Transform;
							uniform mat3 u_HatchTransform;
							attribute vec2 a_Position;
							varying vec2 hatch;
							void main() {
								vec3 P = vec3(a_Position.x, a_Position.y, 1.0) * u_Transform;
								gl_Position = vec4(P.x, P.y, 0.0, 1.0);
								vec3 H = vec3(a_Position.x, a_Position.y, 1.0) * u_HatchTransform;
								hatch = vec2(H.x, H.y);
							}
						))
					case RenderShaderType::GLSL_Fragment:
						{
							String snippet = getShaderSnippet(RenderShaderLanguage::GLSL, style);
							if (snippet.isNull()) {
								return sl_null;
							}
							return String::concat(SLIB_STRINGIFY(
								uniform vec4 u_ForeColor;
								uniform vec4 u_BackColor;
								uniform float hatchLineWidth;
								uniform float hatchSmoothWidth;
								varying vec2 hatch;
								void main() {
							), snippet, SLIB_STRINGIFY(
									gl_FragColor = u_BackColor * (1.0 - hatchFactor) + u_ForeColor * hatchFactor;
								}
							));
						}
					case RenderShaderType::HLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float3x3 u_Transform : register(c0);
							float3x3 u_HatchTransform : register(c3);
							struct VS_OUTPUT {
								float2 hatch : TEXCOORD;
								float4 pos : POSITION;
							};
							VS_OUTPUT main(in float2 a_Position : POSITION) {
								VS_OUTPUT ret;
								float3 P = mul(float3(a_Position.x, a_Position.y, 1.0), u_Transform);
								ret.pos = float4(P.x, P.y, 0.0, 1.0);
								float3 H = mul(float3(a_Position.x, a_Position.y, 1.0), u_HatchTransform);
								ret.hatch = float2(H.x, H.y);
								return ret;
							}
						))
					case RenderShaderType::HLSL_Pixel:
						{
							String snippet = getShaderSnippet(RenderShaderLanguage::HLSL, style);
							if (snippet.isNull()) {
								return sl_null;
							}
							return String::concat(SLIB_STRINGIFY(
								float4 u_ForeColor : register(c0);
								float4 u_BackColor : register(c1);
								float hatchLineWidth : register(c2);
								float hatchSmoothWidth : register(c3);
								float4 main(float2 hatch : TEXCOORD) : COLOR{
							), snippet, SLIB_STRINGIFY(
									return lerp(u_BackColor, u_ForeColor, hatchFactor);
								}
							));
						}
					default:
						break;
				}
				return sl_null;
			}

			String HatchFill::getShaderSnippet(RenderShaderLanguage lang, HatchStyle style)
			{
				switch (style) {
					case HatchStyle::Solid:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float hatchFactor = 1.0;
						))
					case HatchStyle::Vertical:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float hatchX = hatch.x - floor(hatch.x);
							float hatchFactor = smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchX - 0.5));
						))
					case HatchStyle::Horizontal:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float hatchY = hatch.y - floor(hatch.y);
							float hatchFactor = smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchY - 0.5));
						))
					case HatchStyle::ForwardDiagonal:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float hatchX = 0.7071 * (hatch.x + hatch.y);
							hatchX = hatchX - floor(hatchX);
							float hatchFactor = smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchX - 0.5));
						))
					case HatchStyle::BackwardDiagonal:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float hatchX = 0.7071 * (hatch.x - hatch.y);
						hatchX = hatchX - floor(hatchX);
						float hatchFactor = smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchX - 0.5));
						))
					case HatchStyle::Cross:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float hatchX = hatch.x - floor(hatch.x);
							float hatchY = hatch.y - floor(hatch.y);
							float hatchFactor = clamp(smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchX - 0.5)) + smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchY - 0.5)), 0.0, 1.0);
						))
					case HatchStyle::DiagonalCross:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float hatchX = 0.7071 * (hatch.x + hatch.y);
							float hatchY = 0.7071 * (hatch.x - hatch.y);
							hatchX = hatchX - floor(hatchX);
							hatchY = hatchY - floor(hatchY);
							float hatchFactor = clamp(smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchX - 0.5)) + smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchY - 0.5)), 0.0, 1.0);
						))
					case HatchStyle::Dots:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float hatchX = hatch.x - floor(hatch.x);
							float hatchY = hatch.y - floor(hatch.y);
							float hatchFactor = smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchX - 0.5)) * smoothstep((hatchLineWidth + hatchSmoothWidth)*-0.5, hatchLineWidth*-0.5, -abs(hatchY - 0.5));
						))
					default:
						break;
				}
				return sl_null;
			}
		}
	}

	namespace render3d
	{
		namespace program
		{
			String PositionNormalColor::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat4 u_Transform;
							uniform mat4 u_MatrixModelViewIT;
							uniform vec3 u_DirectionalLight;
							uniform vec3 u_DiffuseColor;
							uniform vec3 u_AmbientColor;
							uniform float u_Alpha;
							attribute vec3 a_Position;
							attribute vec3 a_Normal;
							attribute vec4 a_Color;
							varying vec4 v_Color;
							void main() {
								vec4 P = vec4(a_Position, 1.0) * u_Transform;
								vec4 N = vec4(a_Normal, 0.0) * u_MatrixModelViewIT;
								vec3 L = u_DirectionalLight;
								float diffuse = max(dot(N.xyz, L), 0.0);
								gl_Position = P;
								v_Color = vec4(diffuse * u_DiffuseColor + u_AmbientColor, u_Alpha) * a_Color;
							}
						))
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							varying vec4 v_Color;
							void main() {
								gl_FragColor = v_Color;
							}
						))
					default:
						break;
				}
				return sl_null;
			}

			String PositionColor::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat4 u_Transform;
							uniform vec4 u_Color;
							attribute vec3 a_Position;
							attribute vec4 a_Color;
							varying vec4 v_Color;
							void main() {
								vec4 P = vec4(a_Position, 1.0) * u_Transform;
								vec4 C = u_Color * a_Color;
								gl_Position = P;
								v_Color = C;
							}
						))
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							varying vec4 v_Color;
							void main() {
								gl_FragColor = v_Color;
							}
						))
					default:
						break;
				}
				return sl_null;
			}

			String PositionNormalTexture::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat4 u_Transform;
							uniform mat4 u_MatrixModelViewIT;
							uniform vec3 u_DirectionalLight;
							uniform vec3 u_DiffuseColor;
							uniform vec3 u_AmbientColor;
							attribute vec3 a_Position;
							attribute vec3 a_Normal;
							attribute vec2 a_TexCoord;
							varying vec2 v_TexCoord;
							varying vec3 v_Color;
							void main() {
								vec4 P = vec4(a_Position, 1.0) * u_Transform;
								vec4 N = vec4(a_Normal, 0.0) * u_MatrixModelViewIT;
								vec3 L = u_DirectionalLight;
								float diffuse = max(dot(N.xyz, L), 0.0);
								gl_Position = P;
								v_Color = diffuse * u_DiffuseColor + u_AmbientColor;
								v_TexCoord = a_TexCoord;
							}
						))
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform sampler2D u_Texture;
							uniform float u_Alpha;
							varying vec2 v_TexCoord;
							varying vec3 v_Color;
							void main() {
								vec4 colorTexture = texture2D(u_Texture, v_TexCoord);
								vec4 C = vec4(v_Color, u_Alpha);
								gl_FragColor = C * colorTexture;
							}
						))
					default:
						break;
				}
				return sl_null;
			}

			String PositionTexture::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat4 u_Transform;
							attribute vec3 a_Position;
							attribute vec2 a_TexCoord;
							varying vec2 v_TexCoord;
							void main() {
								vec4 P = vec4(a_Position, 1.0) * u_Transform;
								gl_Position = P;
								v_TexCoord = a_TexCoord;
							}
						))
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform sampler2D u_Texture;
							uniform vec4 u_Color;
							varying vec2 v_TexCoord;
							void main() {
								vec4 colorTexture = texture2D(u_Texture, v_TexCoord);
								gl_FragColor = u_Color * colorTexture;
							}
						))
					default:
						break;
				}
				return sl_null;
			}

			String PositionNormal::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat4 u_Transform;
							uniform mat4 u_MatrixModelViewIT;
							uniform vec3 u_DirectionalLight;
							uniform vec3 u_DiffuseColor;
							uniform vec3 u_AmbientColor;
							attribute vec3 a_Position;
							attribute vec3 a_Normal;
							varying vec3 v_Color;
							void main() {
								vec4 P = vec4(a_Position, 1.0) * u_Transform;
								vec4 N = vec4(a_Normal, 0.0) * u_MatrixModelViewIT;
								vec3 L = u_DirectionalLight;
								float diffuse = max(dot(N.xyz, L), 0.0);
								gl_Position = P;
								v_Color = diffuse * u_DiffuseColor + u_AmbientColor;
							}
						))
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform float u_Alpha;
							varying vec3 v_Color;
							void main() {
								vec4 C = vec4(v_Color, u_Alpha);
								gl_FragColor = C;
							}
						))
					default:
						break;
				}
				return sl_null;
			}

			String Position::getShader(RenderEngine* engine, RenderShaderType type)
			{
				return getShader(type);
			}

			String Position::getShader(RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat4 u_Transform;
							attribute vec3 a_Position;
							void main() {
								vec4 P = vec4(a_Position, 1.0) * u_Transform;
								gl_Position = P;
							}
						))
					case RenderShaderType::GLSL_Fragment:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform vec4 u_Color;
							void main() {
								gl_FragColor = u_Color;
							}
						))
					case RenderShaderType::HLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float4x4 u_Transform : register(c0);
							struct VS_OUTPUT {
								float4 pos : POSITION;
							};
							VS_OUTPUT main(float3 a_Position : POSITION) {
								VS_OUTPUT ret;
								ret.pos = mul(float4(a_Position, 1.0), u_Transform);
								return ret;
							}
						))
					case RenderShaderType::HLSL_Pixel:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float4 u_Color : register(c0);
							float4 main() : COLOR{
								return u_Color;
							}
						))
					default:
						break;
				}
				return sl_null;
			}

			String Position2D::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat4 u_Transform;
							attribute vec2 a_Position;
							void main() {
								vec4 P = vec4(a_Position, 1.0, 1.0) * u_Transform;
								gl_Position = P;
							}
						))
					case RenderShaderType::HLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float4x4 u_Transform : register(c0);
							struct VS_OUTPUT {
								float4 pos : POSITION;
							};
							VS_OUTPUT main(float2 a_Position : POSITION) {
								VS_OUTPUT ret;
								ret.pos = mul(float4(a_Position, 1.0, 1.0), u_Transform);
								return ret;
							}
						))
					default:
						break;
				}
				return Position::getShader(type);
			}

			String HatchFill2D::getShader(RenderEngine* engine, RenderShaderType type)
			{
				switch (type) {
					case RenderShaderType::GLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							uniform mat4 u_Transform;
							uniform mat4 u_HatchTransform;
							attribute vec3 a_Position;
							varying vec2 hatch;
							void main() {
								gl_Position = vec4(a_Position, 1.0, 1.0) * u_Transform;
								hatch = (vec4(a_Position, 1.0, 1.0) * u_HatchTransform).xy;
							}
						))
					case RenderShaderType::HLSL_Vertex:
						SLIB_RETURN_STRING(SLIB_STRINGIFY(
							float4x4 u_Transform : register(c0);
							float4x4 u_HatchTransform : register(c4);
							struct VS_OUTPUT {
								float2 hatch : TEXCOORD;
								float4 pos : POSITION;
							};
							VS_OUTPUT main(in float2 a_Position : POSITION) {
								VS_OUTPUT ret;
								ret.pos = mul(float4(a_Position, 1.0, 1.0), u_Transform);
								ret.hatch = (mul(float4(a_Position, 1.0, 1.0), u_HatchTransform)).xy;
								return ret;
							}
						))
					default:
						break;
				}
				return render2d::program::HatchFill::getShader(type, m_style);
			}
		}
	}

}
