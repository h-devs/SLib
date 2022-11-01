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

#include "slib/render/canvas.h"

#include "slib/render/engine.h"
#include "slib/render/opengl.h"
#include "slib/math/transform2d.h"
#include "slib/graphics/util.h"
#include "slib/graphics/font_atlas.h"
#include "slib/core/string_buffer.h"
#include "slib/core/variant.h"
#include "slib/core/safe_static.h"
#include "slib/core/stringify.h"

#define MAX_PROGRAM_COUNT 256
#define MAX_SHADER_CLIP 8

namespace slib
{

	namespace priv
	{
		namespace render_canvas
		{

			SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderCanvasProgramState, RenderVertex2D_Position)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(Transform, u_Transform, RenderShaderType::Vertex, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color, RenderShaderType::Pixel, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(Texture, u_Texture, RenderShaderType::Pixel, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(ColorFilterR, u_ColorFilterR, RenderShaderType::Pixel, 1)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(ColorFilterG, u_ColorFilterG, RenderShaderType::Pixel, 2)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(ColorFilterB, u_ColorFilterB, RenderShaderType::Pixel, 3)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(ColorFilterA, u_ColorFilterA, RenderShaderType::Pixel, 4)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(ColorFilterC, u_ColorFilterC, RenderShaderType::Pixel, 5)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(ColorFilterM, u_ColorFilterM, RenderShaderType::Pixel, 6)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(RectSrc, u_RectSrc, RenderShaderType::Vertex, 3)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3_ARRAY(ClipTransform, u_ClipTransform, RenderShaderType::Vertex, 32)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4_ARRAY(ClipRect, u_ClipRect, RenderShaderType::Vertex | RenderShaderType::Pixel, 16)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position, RenderInputSemanticName::Position, 0)
			SLIB_RENDER_PROGRAM_STATE_END

			class RenderCanvasProgramParam
			{
			public:
				RenderShaderLanguage language;
				sl_bool flagUseTexture;
				sl_bool flagUseColorFilter;
				RenderCanvasClip* clips[MAX_SHADER_CLIP + 1];
				sl_uint32 countClips;

			public:
				RenderCanvasProgramParam()
				{
					language = RenderShaderLanguage::GLSL;
					flagUseTexture = sl_false;
					flagUseColorFilter = sl_false;
					countClips = 0;
				}

				void prepare(RenderCanvasState* state, sl_bool flagIgnoreRectClip)
				{
					if (SLIB_RENDER_CHECK_ENGINE_TYPE(state->engineType, D3D)) {
						if (SLIB_RENDER_CHECK_ENGINE_TYPE(state->engineType, D3D8)) {
							language = RenderShaderLanguage::Assembly;
						} else {
							language = RenderShaderLanguage::HLSL;
						}
					} else {
						language = RenderShaderLanguage::GLSL;
					}
					countClips = 0;
					if (!flagIgnoreRectClip && state->flagClipRect) {
						storageRectClip.type = RenderCanvasClipType::Rectangle;
						storageRectClip.region = state->clipRect;
						clips[0] = &storageRectClip;
						countClips++;
					}
					ListElements<RenderCanvasClip> stateClips(state->clips);
					for (sl_size i = 0; i < stateClips.count; i++) {
						clips[countClips] = &(stateClips[i]);
						countClips++;
						if (countClips >= MAX_SHADER_CLIP) {
							break;
						}
					}
				}

				void addFinalClip(RenderCanvasClip* clip)
				{
					clips[countClips] = clip;
					countClips++;
				}

				void applyToProgramState(RenderCanvasProgramState* state, const Matrix3& transform)
				{
					Matrix3 clipTransforms[MAX_SHADER_CLIP + 1];
					Vector4 clipRects[MAX_SHADER_CLIP + 1];
					for (sl_uint32 i = 0; i < countClips; i++) {
						RenderCanvasClip* clip = clips[i];
						Rectangle& r = clip->region;
						clipRects[i] = Vector4(r.left, r.top, r.right, r.bottom);
						if (clip->flagTransform) {
							clipTransforms[i] = transform * clip->transform;
						} else {
							clipTransforms[i] = transform;
						}
					}
					state->setClipRect(clipRects, countClips);
					state->setClipTransform(clipTransforms, countClips);
				}

			private:
				RenderCanvasClip storageRectClip;

			};

			class RenderCanvasProgram : public RenderProgramT<RenderCanvasProgramState>
			{
			public:
				String m_vertexShader;
				String m_fragmentShader;

			public:
				String getGLSLVertexShader(RenderEngine* engine) override
				{
					return m_vertexShader;
				}

				String getGLSLFragmentShader(RenderEngine* engine) override
				{
					return m_fragmentShader;
				}

				String getHLSLVertexShader(RenderEngine* engine) override
				{
					return m_vertexShader;
				}

				String getHLSLPixelShader(RenderEngine* engine) override
				{
					return m_fragmentShader;
				}

				sl_uint32 getVertexShaderConstantBufferSize(sl_uint32 slot) override
				{
					return 1024;
				}

				sl_uint32 getPixelShaderConstantBufferSize(sl_uint32 slot) override
				{
					return 1024;
				}

				String getAssemblyVertexShader(RenderEngine* engine) override
				{
					return m_vertexShader;
				}

				String getAssemblyPixelShader(RenderEngine* engine) override
				{
					return m_fragmentShader;
				}

				static void generateShaderSources(const RenderCanvasProgramParam& param, char* signatures, StringBuffer* bufVertexShader, StringBuffer* bufFragmentShader)
				{
					RenderShaderLanguage lang = param.language;

					StringBuffer bufVBHeader;
					StringBuffer bufVBContent;
					StringBuffer bufFBHeader;
					StringBuffer bufFBContent;

					// For HLSL
					StringBuffer bufVSInput;
					StringBuffer bufVSOutput;
					StringBuffer bufPSInput;

					if (signatures) {
						*(signatures++) = 'S';
					}

					if (bufVertexShader) {
						switch (lang) {
						case RenderShaderLanguage::HLSL:
							bufVBHeader.addStatic(SLIB_STRINGIFY(
								float3x3 u_Transform : register(c0);
							));
							bufVSInput.addStatic(SLIB_STRINGIFY(
								float2 pos : POSITION;
							));
							bufVSOutput.addStatic(SLIB_STRINGIFY(
								float4 pos : POSITION;
							));
							bufVBContent.addStatic(SLIB_STRINGIFY(
								VS_OUTPUT main(VS_INPUT input) {
									VS_OUTPUT ret;
									ret.pos = float4(mul(float3(input.pos, 1.0), u_Transform).xy, 0.0, 1.0);
							));
							bufFBHeader.addStatic(SLIB_STRINGIFY(
								float4 u_Color : register(c0);
							));
							bufPSInput.addStatic(SLIB_STRINGIFY(
								float4 pos : POSITION;
							));
							bufFBContent.addStatic(SLIB_STRINGIFY(
								float4 main(PS_INPUT input) : COLOR {
									float4 l_Color = u_Color;
							));
							break;
						case RenderShaderLanguage::GLSL:
							bufVBHeader.addStatic(SLIB_STRINGIFY(
								uniform mat3 u_Transform;
								attribute vec2 a_Position;
							));
							bufVBContent.addStatic(SLIB_STRINGIFY(
								void main() {
									gl_Position = vec4((vec3(a_Position, 1.0) * u_Transform).xy, 0.0, 1.0);
							));
							bufFBHeader.addStatic(SLIB_STRINGIFY(
								uniform vec4 u_Color;
							));
							bufFBContent.addStatic(SLIB_STRINGIFY(
								void main() {
									vec4 l_Color = u_Color;
							));
							break;
						case RenderShaderLanguage::Assembly:
							bufVBContent.addStatic(
								"vs.1.0\n"
								"def c50, 1.0f, 0.0f, 0.0f, 1.0f\n"
								"mov r0.xy, v0.xy\n"
								"mov r0.z, c50.x\n"
								"m3x3 r1, r0, c0\n"
								"mov r1.zw, c50.zw\n"
								"mov oPos, r1\n"
							);
							bufFBContent.addStatic("ps.1.0\n");
							break;
						default:
							break;
						}
					}
					if (param.flagUseTexture) {
						if (bufVertexShader) {
							if (lang == RenderShaderLanguage::HLSL) {
								bufVSOutput.addStatic(SLIB_STRINGIFY(
									float2 texCoord : TEXCOORD0;
								));
								bufPSInput.addStatic(SLIB_STRINGIFY(
									float2 texCoord : TEXCOORD0;
								));
							}
						}
					}
					if (param.countClips > 0 && (lang != RenderShaderLanguage::Assembly)) {
						if (bufVertexShader) {
							switch (lang) {
							case RenderShaderLanguage::HLSL:
								bufVBHeader.add(String::format(SLIB_STRINGIFY(
									float4 u_ClipRect[%d] : register(c16);
									float3x3 u_ClipTransform[%d] : register(c32);
								), param.countClips));
								bufVSOutput.add(String::format(SLIB_STRINGIFY(
									float2 clipPos[%d] : TEXCOORD1;
								), param.countClips));
								bufFBHeader.add(String::format(SLIB_STRINGIFY(
									float4 u_ClipRect[%d] : register(c16);
								), param.countClips));
								bufPSInput.add(String::format(SLIB_STRINGIFY(
									float2 clipPos[%d] : TEXCOORD1;
								), param.countClips));
								break;
							case RenderShaderLanguage::GLSL:
								bufVBHeader.add(String::format(SLIB_STRINGIFY(
									varying vec2 v_ClipPos[%d];
									uniform vec4 u_ClipRect[%d];
									uniform mat3 u_ClipTransform[%d];
								), param.countClips));
								bufFBHeader.add(String::format(SLIB_STRINGIFY(
									varying vec2 v_ClipPos[%d];
									uniform vec4 u_ClipRect[%d];
								), param.countClips));
								break;
							default:
								break;
							}
						}
						for (sl_uint32 i = 0; i < param.countClips; i++) {
							RenderCanvasClipType type = param.clips[i]->type;
							if (type == RenderCanvasClipType::Ellipse) {
								sl_bool flagOval = Math::isAlmostZero(param.clips[i]->region.getWidth() - param.clips[i]->region.getHeight());
								if (signatures) {
									if (flagOval) {
										*(signatures++) = 'O';
									} else {
										*(signatures++) = 'E';
									}
								}
								if (bufVertexShader) {
									switch (lang) {
									case RenderShaderLanguage::HLSL:
										bufVBContent.add(String::format(SLIB_STRINGIFY(
											ret.clipPos[%d] = mul(float3(input.pos.x, input.pos.y, 1.0), u_ClipTransform[%d]).xy - (u_ClipRect[%d].xy + u_ClipRect[%d].zw) / 2.0;
										), i));
										bufFBContent.add(String::format(SLIB_STRINGIFY(
											float xClip%d = input.clipPos[%d].x;
											float yClip%d = input.clipPos[%d].y;
										), i));
										break;
									case RenderShaderLanguage::GLSL:
										bufVBContent.add(String::format(SLIB_STRINGIFY(
											v_ClipPos[%d] = (vec3(a_Position, 1.0) * u_ClipTransform[%d]).xy - (u_ClipRect[%d].xy + u_ClipRect[%d].zw) / 2.0;
										), i));
										bufFBContent.add(String::format(SLIB_STRINGIFY(
											float xClip%d = v_ClipPos[%d].x;
											float yClip%d = v_ClipPos[%d].y;
										), i));
										break;
									default:
										break;
									}
									bufFBContent.add(String::format(SLIB_STRINGIFY(
										float wClip%d = (u_ClipRect[%d].z - u_ClipRect[%d].x) / 2.0;
										float hClip%d = (u_ClipRect[%d].w - u_ClipRect[%d].y) / 2.0;
										xClip%d /= wClip%d;
										yClip%d /= hClip%d;
										float lenClip%d = xClip%d * xClip%d + yClip%d * yClip%d;
										if (lenClip%d > 1.0) {
											discard;
										}
									), i));
									if (flagOval) {
										bufFBContent.add(String::format(SLIB_STRINGIFY(
											else {
												lenClip%d = sqrt(lenClip%d);
												l_Color.w *= smoothstep(0.0, 1.5 / sqrt(wClip%d * hClip%d), 1.0 - lenClip%d);
											}
										), i));
									}
								}
							} else {
								if (signatures) {
									*(signatures++) = 'C';
								}
								if (bufVertexShader) {
									switch (lang) {
									case RenderShaderLanguage::HLSL:
										bufVBContent.add(String::format(SLIB_STRINGIFY(
											ret.clipPos[%d] = mul(float3(input.pos.x, input.pos.y, 1.0), u_ClipTransform[%d]).xy;
										), i));
										bufFBContent.add(String::format(SLIB_STRINGIFY(
											float xClip%d = input.clipPos[%d].x;
											float yClip%d = input.clipPos[%d].y;
										), i));
										break;
									case RenderShaderLanguage::GLSL:
										bufVBContent.add(String::format(SLIB_STRINGIFY(
											v_ClipPos[%d] = (vec3(a_Position, 1.0) * u_ClipTransform[%d]).xy;
										), i));
										bufFBContent.add(String::format(SLIB_STRINGIFY(
											float xClip%d = v_ClipPos[%d].x;
											float yClip%d = v_ClipPos[%d].y;
										), i));
										break;
									default:
										break;
									}
									bufFBContent.add(String::format(SLIB_STRINGIFY(
										float fClip%d = step(u_ClipRect[%d].x, xClip%d) * step(u_ClipRect[%d].y, yClip%d) * step(xClip%d, u_ClipRect[%d].z) * step(yClip%d, u_ClipRect[%d].w);
									if (fClip%d < 0.5) {
										discard;
									}
									), i));
								}
							}
						}
					}

					if (param.flagUseTexture) {
						if (signatures) {
							*(signatures++) = 'T';
						}
						if (bufVertexShader) {
							switch (lang) {
							case RenderShaderLanguage::HLSL:
								bufVBHeader.addStatic(SLIB_STRINGIFY(
									float4 u_RectSrc : register(c3);
								));
								bufVBContent.addStatic(SLIB_STRINGIFY(
									ret.texCoord = input.pos * u_RectSrc.zw + u_RectSrc.xy;
								));
								bufFBHeader.addStatic(SLIB_STRINGIFY(
									sampler u_Texture;
								));
								break;
							case RenderShaderLanguage::GLSL:
								bufVBHeader.addStatic(SLIB_STRINGIFY(
									uniform vec4 u_RectSrc;
									varying vec2 v_TexCoord;
								));
								bufVBContent.addStatic(SLIB_STRINGIFY(
									v_TexCoord = a_Position * u_RectSrc.zw + u_RectSrc.xy;
								));
								bufFBHeader.addStatic(SLIB_STRINGIFY(
									uniform sampler2D u_Texture;
									varying vec2 v_TexCoord;
								));
								break;
							case RenderShaderLanguage::Assembly:
								bufVBContent.addStatic(
									"mov r1.xy, c3.zw\n"
									"mad r0.xy, v0.xy, r1.xy, c3.xy\n"
									"mov r0.zw, c50.zw\n"
									"mov oT0, r0\n"
								);
								break;
							default:
								break;
							}
						}

						if (param.flagUseColorFilter) {
							if (signatures) {
								*(signatures++) = 'F';
							}
							if (bufVertexShader) {
								switch (lang) {
								case RenderShaderLanguage::HLSL:
									bufFBHeader.addStatic(SLIB_STRINGIFY(
										float4 u_ColorFilterR : register(c1);
										float4 u_ColorFilterG : register(c2);
										float4 u_ColorFilterB : register(c3);
										float4 u_ColorFilterA : register(c4);
										float4 u_ColorFilterC : register(c5);
									));
									bufFBContent.addStatic(SLIB_STRINGIFY(
										float4 color = tex2D(u_Texture, input.texCoord);
										color = float4(dot(color, u_ColorFilterR), dot(color, u_ColorFilterG), dot(color, u_ColorFilterB), dot(color, u_ColorFilterA)) + u_ColorFilterC;
										color = color * l_Color;
									));
									break;
								case RenderShaderLanguage::GLSL:
									bufFBHeader.addStatic(SLIB_STRINGIFY(
										uniform vec4 u_ColorFilterR;
										uniform vec4 u_ColorFilterG;
										uniform vec4 u_ColorFilterB;
										uniform vec4 u_ColorFilterA;
										uniform vec4 u_ColorFilterC;
									));
									bufFBContent.addStatic(SLIB_STRINGIFY(
										vec4 color = texture2D(u_Texture, v_TexCoord);
										color = vec4(dot(color, u_ColorFilterR), dot(color, u_ColorFilterG), dot(color, u_ColorFilterB), dot(color, u_ColorFilterA)) + u_ColorFilterC;
										color = color * l_Color;
									));
									break;
								case RenderShaderLanguage::Assembly:
									bufFBContent.addStatic(
										"tex t0\n"
										"mad r0, c6, t0, c5\n"
										"mul r0, r0, c0\n"
									);
									break;
								default:
									break;
								}
							}
						} else {
							if (bufVertexShader) {
								switch (lang) {
								case RenderShaderLanguage::HLSL:
									bufFBContent.addStatic(SLIB_STRINGIFY(
										float4 color = tex2D(u_Texture, input.texCoord) * l_Color;
									));
									break;
								case RenderShaderLanguage::GLSL:
									bufFBContent.addStatic(SLIB_STRINGIFY(
										vec4 color = texture2D(u_Texture, v_TexCoord) * l_Color;
									));
									break;
								case RenderShaderLanguage::Assembly:
									bufFBContent.addStatic("tex t0\nmul r0, t0, c0\n");
									break;
								default:
									break;
								}
							}
						}
					} else {
						if (bufVertexShader) {
							switch (lang) {
							case RenderShaderLanguage::HLSL:
								bufFBContent.addStatic(SLIB_STRINGIFY(
									float4 color = l_Color;
								));
								break;
							case RenderShaderLanguage::GLSL:
								bufFBContent.addStatic(SLIB_STRINGIFY(
									vec4 color = l_Color;
								));
								break;
							case RenderShaderLanguage::Assembly:
								bufFBContent.addStatic("mov r0, c0\n");
								break;
							default:
								break;
							}
						}
					}
					if (bufVertexShader) {
						switch (lang) {
						case RenderShaderLanguage::HLSL:
							bufVBContent.addStatic(SLIB_STRINGIFY(
								return ret;
							));
							bufFBContent.addStatic(SLIB_STRINGIFY(
								return color;
							));
							bufVBHeader.addStatic("struct VS_INPUT {");
							bufVBHeader.link(bufVSInput);
							bufVBHeader.addStatic("}; struct VS_OUTPUT {");
							bufVBHeader.link(bufVSOutput);
							bufVBHeader.addStatic("};");
							bufFBHeader.addStatic("struct PS_INPUT {");
							bufFBHeader.link(bufPSInput);
							bufFBHeader.addStatic("};");
							break;
						case RenderShaderLanguage::GLSL:
							bufFBContent.addStatic(SLIB_STRINGIFY(
								gl_FragColor = color;
							));
							break;
						default:
							break;
						}
						if (lang != RenderShaderLanguage::Assembly) {
							bufVBContent.addStatic("}");
							bufFBContent.addStatic("}");
						}
						bufVertexShader->link(bufVBHeader);
						bufVertexShader->link(bufVBContent);
						bufFragmentShader->link(bufFBHeader);
						bufFragmentShader->link(bufFBContent);
					}

				}

				static Ref<RenderCanvasProgram> create(const RenderCanvasProgramParam& param)
				{
					StringBuffer sbVB;
					StringBuffer sbFB;
					RenderCanvasProgram::generateShaderSources(param, sl_null, &sbVB, &sbFB);
					String vertexShader = sbVB.merge();
					String fragmentShader = sbFB.merge();
					if (vertexShader.isNotEmpty() && fragmentShader.isNotEmpty()) {
						Ref<RenderCanvasProgram> ret = new RenderCanvasProgram;
						if (ret.isNotNull()) {
							ret->m_vertexShader = vertexShader;
							ret->m_fragmentShader = fragmentShader;
							return ret;
						}
					}
					return sl_null;
				}

			};

			class EngineContext : public Referable
			{
			public:
				CHashMap< String, Ref<RenderCanvasProgram> > programs;
				Ref<VertexBuffer> vbRectangle;

			public:
				EngineContext()
				{
					static RenderVertex2D_Position v[] = {
						{ { 0, 0 } }
						, { { 1, 0 } }
						, { { 0, 1 } }
						, { { 1, 1 } }
					};
					vbRectangle = VertexBuffer::create(v, sizeof(v));
				}

				Ref<RenderCanvasProgram> getProgram(const RenderCanvasProgramParam& param)
				{
					char sig[64] = {0};
					RenderCanvasProgram::generateShaderSources(param, sig, sl_null, sl_null);
					Ref<RenderCanvasProgram> program;
					if (!(programs.get_NoLock(sig, &program))) {
						program = RenderCanvasProgram::create(param);
						if (program.isNull()) {
							return sl_null;
						}
						if (programs.getCount() > MAX_PROGRAM_COUNT) {
							programs.removeAll_NoLock();
						}
						programs.put_NoLock(sig, program);
					}
					return program;
				}

			};

			class EngineHelper : public RenderEngine
			{
			public:
				EngineContext* getContext()
				{
					if (m_canvasContext.isNull()) {
						m_canvasContext = new EngineContext;
					}
					return (EngineContext*)(m_canvasContext.get());
				}

			};

			class RenderCanvasHelper : public RenderCanvas
			{
			public:
				EngineContext * getContext()
				{
					return ((EngineHelper*)(m_engine.get()))->getContext();
				}

			};

			static EngineContext* GetEngineContext(RenderCanvas* canvas)
			{
				return ((RenderCanvasHelper*)canvas)->getContext();
			}

		}
	}

	using namespace priv::render_canvas;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RenderCanvasClip)

	RenderCanvasClip::RenderCanvasClip(): type(RenderCanvasClipType::Rectangle), rx(0), ry(0), flagTransform(sl_false)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RenderCanvasState)

	RenderCanvasState::RenderCanvasState()
	 : engineType(RenderEngineType::Any), matrix(Matrix3::identity()), flagClipRect(sl_false)
	{
	}


	SLIB_DEFINE_OBJECT(RenderCanvas, Canvas)

	RenderCanvas::RenderCanvas()
	{
		m_width = 0;
		m_height = 0;
	}

	RenderCanvas::~RenderCanvas()
	{
	}

	Ref<RenderCanvas> RenderCanvas::create(const Ref<RenderEngine>& engine, sl_real width, sl_real height)
	{
		if (engine.isNotNull()) {

			Ref<RenderCanvasState> state = new RenderCanvasState;

			if (state.isNotNull()) {

				Ref<RenderCanvas> ret = new RenderCanvas;
				if (ret.isNotNull()) {
					ret->m_engine = engine;
					ret->m_width = width;
					ret->m_height = height;
					ret->m_matViewport = Matrix3(2/width, 0, 0, 0, -2/height, 0, -1, 1, 1);

					ret->m_state = state;
					state->engineType = engine->getEngineType();

					ret->setType(CanvasType::Render);
					ret->setSize(Size(width, height));

					return ret;
				}

			}

		}
		return sl_null;
	}

	const Ref<RenderEngine>& RenderCanvas::getEngine()
	{
		return m_engine;
	}

	RenderCanvasState* RenderCanvas::getCurrentState()
	{
		return m_state.get();
	}

	void RenderCanvas::save()
	{
		RenderCanvasState* stateOld = m_state.get();
		Ref<RenderCanvasState> stateNew = new RenderCanvasState(*stateOld);
		if (stateNew.isNotNull()) {
			m_stackStates.push_NoLock(stateOld);
			m_state = stateNew;
		}
	}

	void RenderCanvas::restore()
	{
		Ref<RenderCanvasState> stateBack;
		if (m_stackStates.pop_NoLock(&stateBack)) {
			m_state = stateBack;
		}
	}

	Rectangle RenderCanvas::getClipBounds()
	{
		Ref<RenderCanvasState>& state = m_state;
		Rectangle rect;
		if (state->flagClipRect) {
			rect = state->clipRect;
		} else {
			rect = Rectangle(0, 0, m_width, m_height);
		}
		ListElements<RenderCanvasClip> clips(state->clips);
		for (sl_size i = 0; i < clips.count; i++) {
			RenderCanvasClip& clip = clips[i];
			Rectangle r = clip.region;
			if (clip.flagTransform) {
				r.transform(clip.transform.inverse());
			}
			if (!(rect.intersectRectangle(r, &rect))) {
				return Rectangle::zero();
			}
		}
		return rect;
	}

	void RenderCanvas::clipToRectangle(const Rectangle& rect)
	{
		RenderCanvasState* state = m_state.get();
		if (state->flagClipRect) {
			state->clipRect.intersectRectangle(rect, &(state->clipRect));
		} else {
			state->flagClipRect = sl_true;
			state->clipRect = rect;
		}
	}

	void RenderCanvas::clipToPath(const Ref<GraphicsPath>& path)
	{
	}

	void RenderCanvas::clipToRoundRect(const Rectangle& rect, const Size& radius)
	{
		RenderCanvasState* state = m_state.get();
		RenderCanvasClip clip;
		clip.type = RenderCanvasClipType::RoundRect;
		clip.region = rect;
		clip.rx = radius.x;
		clip.ry = radius.y;
		state->clips.add_NoLock(clip);
	}

	void RenderCanvas::clipToEllipse(const Rectangle& rect)
	{
		RenderCanvasState* state = m_state.get();
		RenderCanvasClip clip;
		clip.type = RenderCanvasClipType::Ellipse;
		clip.region = rect;
		state->clips.add_NoLock(clip);
	}

	void RenderCanvas::concatMatrix(const Matrix3& matrix)
	{
		RenderCanvasState* state = m_state.get();
		state->matrix = matrix * state->matrix;
		ListElements<RenderCanvasClip> clips(state->clips);
		for (sl_size i = 0; i < clips.count; i++) {
			RenderCanvasClip& clip = clips[i];
			if (clip.flagTransform) {
				clip.transform = matrix * clip.transform;
			} else {
				clip.flagTransform = sl_true;
				clip.transform = matrix;
			}
		}
		if (state->flagClipRect) {
			RenderCanvasClip clip;
			clip.type = RenderCanvasClipType::Rectangle;
			clip.region = state->clipRect;
			clip.flagTransform = sl_true;
			clip.transform = matrix;
			state->clips.add_NoLock(clip);
			state->flagClipRect = sl_false;
		}
	}

	void RenderCanvas::translate(sl_real tx, sl_real ty)
	{
		RenderCanvasState* state = m_state.get();
		Transform2::preTranslate(state->matrix, tx, ty);
		if (state->flagClipRect) {
			state->clipRect.left -= tx;
			state->clipRect.top -= ty;
			state->clipRect.right -= tx;
			state->clipRect.bottom -= ty;
		}
		ListElements<RenderCanvasClip> clips(state->clips);
		for (sl_size i = 0; i < clips.count; i++) {
			RenderCanvasClip& clip = clips[i];
			if (clip.flagTransform) {
				Transform2::preTranslate(clip.transform, tx, ty);
			} else {
				clip.region.left -= tx;
				clip.region.top -= ty;
				clip.region.right -= tx;
				clip.region.bottom -= ty;
			}
		}
	}

	void RenderCanvas::translateFromSavedState(RenderCanvasState* savedState, sl_real tx, sl_real ty)
	{
		RenderCanvasState* state = m_state.get();
		state->matrix = savedState->matrix;
		Transform2::preTranslate(state->matrix, tx, ty);
		if (savedState->flagClipRect) {
			state->clipRect.left = savedState->clipRect.left - tx;
			state->clipRect.top = savedState->clipRect.top - ty;
			state->clipRect.right = savedState->clipRect.right - tx;
			state->clipRect.bottom = savedState->clipRect.bottom - ty;
		}
		ListElements<RenderCanvasClip> clips(state->clips);
		ListElements<RenderCanvasClip> savedClips(savedState->clips);
		sl_size n = Math::min(clips.count, savedClips.count);
		for (sl_size i = 0; i < n; i++) {
			RenderCanvasClip& clip = clips[i];
			RenderCanvasClip& savedClip = savedClips[i];
			if (savedClip.flagTransform) {
				clip.transform = savedClip.transform;
				Transform2::preTranslate(clip.transform, tx, ty);
			} else {
				clip.region.left = savedClip.region.left - tx;
				clip.region.top = savedClip.region.top - ty;
				clip.region.right = savedClip.region.right - tx;
				clip.region.bottom = savedClip.region.bottom - ty;
			}
		}
	}

	Size RenderCanvas::measureText(const Ref<Font>& font, const StringParam& text, sl_bool flagMultiLine)
	{
		return measureRenderingText(font, text, flagMultiLine);
	}

	Size RenderCanvas::measureRenderingText(const Ref<Font>& font, const StringParam& text, sl_bool flagMultiLine)
	{
		if (text.isEmpty()) {
			return Size::zero();
		}
		if (font.isNull()) {
			return Size::zero();
		}
		Ref<FontAtlas> fa = font->getSharedAtlas();
		if (fa.isNull()) {
			return Size::zero();
		}
		return fa->measureText(text, flagMultiLine);
	}

	void RenderCanvas::drawLine(const Point& pt1, const Point& pt2, const Ref<Pen>& pen)
	{
		if (pen.isNull()) {
			return;
		}

		sl_real penWidth = pen->getWidth();
		sl_real penWidthHalf = penWidth / 2;

		RenderCanvasState* state = m_state.get();
		if (penWidth < 1.0000001f && Vector2(state->matrix.m00, state->matrix.m10).getLength2p() <= 1.000001f && Vector2(state->matrix.m01, state->matrix.m11).getLength2p() <= 1.000001f) {
			if (Math::abs(pt1.x - pt2.x) < 0.0000001f) {
				_fillRectangle(Rectangle(pt1.x, pt1.y, pt1.x + 1, pt2.y), pen->getColor());
				return;
			}
			if (Math::abs(pt1.y - pt2.y) < 0.0000001f) {
				_fillRectangle(Rectangle(pt1.x, pt1.y, pt2.x, pt1.y + 1), pen->getColor());
				return;
			}
		} else {
			if (Math::abs(pt1.x - pt2.x) < 0.0000001f || Math::abs(pt1.y - pt2.y) < 0.0000001f) {
				_fillRectangle(Rectangle(pt1.x - penWidthHalf, pt1.y - penWidthHalf, pt2.x + penWidthHalf, pt2.y + penWidthHalf), pen->getColor());
				return;
			}
		}

		sl_real angle = Math::arctan((pt1.y - pt2.y) / (pt1.x - pt2.x));
		sl_real c = Math::cos(-angle);
		sl_real s = Math::sin(-angle);

		sl_real centerX = (pt1.x + pt2.x) / 2;
		sl_real centerY = (pt1.y + pt2.y) / 2;

		sl_real newX1 = centerX + (pt1.x - centerX) * c - (pt1.y - centerY) * s;
		sl_real newY1 = centerY + (pt1.x - centerX) * s + (pt1.y - centerY) * c;

		sl_real newX2 = centerX + (pt2.x - centerX) * c - (pt2.y - centerY) * s;
		sl_real newY2 = centerY + (pt2.x - centerX) * s + (pt2.y - centerY) * c;

		CanvasStateScope scope(this);
		rotate(centerX, centerY, angle);
		_fillRectangle(Rectangle(newX1 - penWidthHalf, newY1 - penWidthHalf, newX2 + penWidthHalf, newY2 + penWidthHalf), pen->getColor());

	}

	void RenderCanvas::drawLines(const Point* points, sl_uint32 countPoints, const Ref<Pen>& pen)
	{
		for (sl_uint32 i = 1; i < countPoints; i++) {
			drawLine(points[i - 1], points[i], pen);
		}
	}

	void RenderCanvas::drawArc(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen)
	{
	}


	void RenderCanvas::drawRectangle(const Rectangle& rect, const Ref<Pen>& pen, const Ref<Brush>& brush)
	{
		if (brush.isNotNull()) {
			drawRectangle(rect, pen, brush->getColor());
		} else {
			drawRectangle(rect, pen, Color::zero());
		}
	}

	void RenderCanvas::drawRectangle(const Rectangle& rect, const Ref<Pen>& pen, const Color& fillColor)
	{
		if (fillColor.a > 0) {
			if (pen.isNotNull()) {
				sl_real penWidth = pen->getWidth();
				_fillRectangle(Rectangle(rect.left + penWidth, rect.top + penWidth, rect.right - penWidth, rect.bottom - penWidth), fillColor);
			} else {
				_fillRectangle(rect, fillColor);
			}
		}
		if (pen.isNotNull()) {
			Color color = pen->getColor();
			sl_real penWidth = pen->getWidth();
			// top
			_fillRectangle(Rectangle(rect.left, rect.top, rect.right, rect.top + penWidth), color);
			// bottom
			_fillRectangle(Rectangle(rect.left, rect.bottom - penWidth, rect.right, rect.bottom), color);
			// left
			_fillRectangle(Rectangle(rect.left, rect.top + penWidth, rect.left + penWidth, rect.bottom - penWidth), color);
			// right
			_fillRectangle(Rectangle(rect.right - penWidth, rect.top + penWidth, rect.right, rect.bottom - penWidth), color);
		}
	}

	void RenderCanvas::_fillRectangle(const Rectangle& _rect, const Color& _color)
	{
		EngineContext* context = GetEngineContext(this);
		if (!context) {
			return;
		}

		RenderCanvasState* state = m_state.get();

		Rectangle rect = _rect;
		if (state->flagClipRect) {
			if (!(state->clipRect.intersectRectangle(rect, &rect))) {
				return;
			}
		}

		RenderCanvasProgramParam pp;
		pp.prepare(state, sl_true);

		RenderProgramScope<RenderCanvasProgramState> scope;
		if (scope.begin(m_engine.get(), context->getProgram(pp))) {
			Matrix3 mat;
			mat.m00 = rect.getWidth(); mat.m10 = 0; mat.m20 = rect.left;
			mat.m01 = 0; mat.m11 = rect.getHeight(); mat.m21 = rect.top;
			mat.m02 = 0; mat.m12 = 0; mat.m22 = 1;
			pp.applyToProgramState(scope.getState(), mat);
			mat *= state->matrix;
			mat *= m_matViewport;
			scope->setTransform(mat);
			Color4f color = _color;
			color.w *= getAlpha();
			scope->setColor(color);
			m_engine->drawPrimitive(4, context->vbRectangle, PrimitiveType::TriangleStrip);
		}

	}

	void RenderCanvas::drawRoundRect(const Rectangle& rect, const Size& radius, const Ref<Pen>& pen, const Ref<Brush>& brush)
	{
		drawRectangle(rect, pen, brush);
	}

	void RenderCanvas::drawEllipse(const Rectangle& rect, const Ref<Pen>& pen, const Ref<Brush>& brush)
	{
		EngineContext* context = GetEngineContext(this);
		if (!context) {
			return;
		}

		RenderCanvasState* state = m_state.get();

		if (brush.isNotNull()) {
			if (state->flagClipRect) {
				if (!(state->clipRect.intersectRectangle(rect, sl_null))) {
					return;
				}
			}

			RenderCanvasProgramParam pp;
			pp.prepare(state, state->flagClipRect && state->clipRect.containsRectangle(rect));
			RenderCanvasClip clip;
			clip.type = RenderCanvasClipType::Ellipse;
			clip.region = rect;
			pp.addFinalClip(&clip);

			RenderProgramScope<RenderCanvasProgramState> scope;
			if (scope.begin(m_engine.get(), context->getProgram(pp))) {
				Matrix3 mat;
				mat.m00 = rect.getWidth(); mat.m10 = 0; mat.m20 = rect.left;
				mat.m01 = 0; mat.m11 = rect.getHeight(); mat.m21 = rect.top;
				mat.m02 = 0; mat.m12 = 0; mat.m22 = 1;
				pp.applyToProgramState(scope.getState(), mat);
				mat *= state->matrix;
				mat *= m_matViewport;
				scope->setTransform(mat);
				Color4f color = brush->getColor();
				color.w *= getAlpha();
				scope->setColor(color);
				m_engine->drawPrimitive(4, context->vbRectangle, PrimitiveType::TriangleStrip);
			}
		}
	}

	void RenderCanvas::drawPolygon(const Point* points, sl_uint32 countPoints, const Ref<Pen>& pen, const Ref<Brush>& brush, FillMode fillMode)
	{
	}

	void RenderCanvas::drawPie(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Ref<Brush>& brush)
	{
	}

	void RenderCanvas::drawPath(const Ref<GraphicsPath>& path, const Ref<Pen>& pen, const Ref<Brush>& brush)
	{
	}

	void RenderCanvas::drawTexture(const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& _rectSrc, const DrawParam& param, const Color4f& color)
	{

		EngineContext* context = GetEngineContext(this);
		if (!context) {
			return;
		}

		RenderCanvasState* state = m_state.get();

		Rectangle rectSrc = _rectSrc;
		sl_real sw = (sl_real)(texture->getWidth());
		sl_real sh = (sl_real)(texture->getHeight());
		rectSrc.left /= sw;
		rectSrc.top /= sh;
		rectSrc.right /= sw;
		rectSrc.bottom /= sh;

		RenderCanvasProgramParam pp;
		pp.prepare(state, sl_false);
		pp.flagUseTexture = sl_true;
		if (param.useColorMatrix) {
			pp.flagUseColorFilter = sl_true;
		}

		RenderProgramScope<RenderCanvasProgramState> scope;
		if (scope.begin(m_engine.get(), context->getProgram(pp))) {
			pp.applyToProgramState(scope.getState(), transform);
			scope->setTexture(texture);
			scope->setTransform(transform * state->matrix * m_matViewport);
			scope->setRectSrc(Vector4(rectSrc.left, rectSrc.top, rectSrc.getWidth(), rectSrc.getHeight()));
			if (param.useColorMatrix) {
				scope->setColorFilterR(param.colorMatrix.red);
				scope->setColorFilterG(param.colorMatrix.green);
				scope->setColorFilterB(param.colorMatrix.blue);
				scope->setColorFilterA(param.colorMatrix.alpha);
				scope->setColorFilterC(param.colorMatrix.bias);
				scope->setColorFilterM(Vector4(param.colorMatrix.red.x, param.colorMatrix.green.y, param.colorMatrix.blue.z, param.colorMatrix.alpha.w));
			}
			if (param.useAlpha) {
				scope->setColor(Color4f(color.x, color.y, color.z, color.w * param.alpha * getAlpha()));
			} else {
				scope->setColor(Color4f(color.x, color.y, color.z, color.w * getAlpha()));
			}
			m_engine->drawPrimitive(4, context->vbRectangle, PrimitiveType::TriangleStrip);
		}

	}

	void RenderCanvas::drawTexture(const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, const DrawParam& param)
	{
		drawTexture(transform, texture, rectSrc, param, Color4f(1, 1, 1, 1));
	}

	void RenderCanvas::drawTexture(const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha)
	{
		drawTexture(transform, texture, rectSrc, DrawParam(), Color4f(1, 1, 1, alpha));
	}

	void RenderCanvas::drawTexture(const Matrix3& transform, const Ref<Texture>& texture, const DrawParam& param, const Color4f& color)
	{
		if (texture.isNotNull()) {
			drawTexture(transform, texture, Rectangle(0, 0, (sl_real)(texture->getWidth()), (sl_real)(texture->getHeight())), param, color);
		}
	}

	void RenderCanvas::drawTexture(const Matrix3& transform, const Ref<Texture>& texture, const DrawParam& param)
	{
		drawTexture(transform, texture, param, Color4f(1, 1, 1, 1));
	}

	void RenderCanvas::drawTexture(const Matrix3& transform, const Ref<Texture>& texture, sl_real alpha)
	{
		drawTexture(transform, texture, DrawParam(), Color4f(1, 1, 1, alpha));
	}

	void RenderCanvas::drawTexture(const Rectangle& _rectDst, const Ref<Texture>& texture, const Rectangle& _rectSrc, const DrawParam& param, const Color4f& color)
	{

		EngineContext* context = GetEngineContext(this);
		if (!context) {
			return;
		}

		RenderCanvasState* state = m_state.get();

		Rectangle rectDst = _rectDst;
		Rectangle rectSrc = _rectSrc;
		sl_real sw = (sl_real)(texture->getWidth());
		sl_real sh = (sl_real)(texture->getHeight());
		if (state->flagClipRect) {
			Rectangle rectIntersectClip;
			if (state->clipRect.intersectRectangle(rectDst, &rectIntersectClip)) {
				if (!(rectDst.isAlmostEqual(rectIntersectClip))) {
					rectSrc = GraphicsUtil::transformRectangle(rectSrc, rectDst, rectIntersectClip);
					rectDst = rectIntersectClip;
				}
			} else {
				return;
			}
		}
		rectSrc.left /= sw;
		rectSrc.top /= sh;
		rectSrc.right /= sw;
		rectSrc.bottom /= sh;

		RenderCanvasProgramParam pp;
		pp.prepare(state, sl_true);
		pp.flagUseTexture = sl_true;
		if (param.useColorMatrix) {
			pp.flagUseColorFilter = sl_true;
		}

		RenderProgramScope<RenderCanvasProgramState> scope;
		if (scope.begin(m_engine.get(), context->getProgram(pp))) {
			scope->setTexture(texture);
			Matrix3 mat;
			mat.m00 = rectDst.getWidth(); mat.m10 = 0; mat.m20 = rectDst.left;
			mat.m01 = 0; mat.m11 = rectDst.getHeight(); mat.m21 = rectDst.top;
			mat.m02 = 0; mat.m12 = 0; mat.m22 = 1;
			pp.applyToProgramState(scope.getState(), mat);
			mat *= state->matrix;
			mat *= m_matViewport;
			scope->setTransform(mat);
			scope->setRectSrc(Vector4(rectSrc.left, rectSrc.top, rectSrc.getWidth(), rectSrc.getHeight()));
			if (param.useColorMatrix) {
				scope->setColorFilterR(param.colorMatrix.red);
				scope->setColorFilterG(param.colorMatrix.green);
				scope->setColorFilterB(param.colorMatrix.blue);
				scope->setColorFilterA(param.colorMatrix.alpha);
				scope->setColorFilterC(param.colorMatrix.bias);
				scope->setColorFilterM(Vector4(param.colorMatrix.red.x, param.colorMatrix.green.y, param.colorMatrix.blue.z, param.colorMatrix.alpha.w));
			}
			if (param.useAlpha) {
				scope->setColor(Color4f(color.x, color.y, color.z, color.w * param.alpha * getAlpha()));
			} else {
				scope->setColor(Color4f(color.x, color.y, color.z, color.w * getAlpha()));
			}
			m_engine->drawPrimitive(4, context->vbRectangle, PrimitiveType::TriangleStrip);
		}

	}

	void RenderCanvas::drawTexture(const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, const DrawParam& param)
	{
		drawTexture(rectDst, texture, rectSrc, param, Color4f(1, 1, 1, 1));
	}

	void RenderCanvas::drawTexture(const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha)
	{
		drawTexture(rectDst, texture, rectSrc, DrawParam(), Color4f(1, 1, 1, alpha));
	}

	void RenderCanvas::drawTexture(const Rectangle& rectDst, const Ref<Texture>& texture, const DrawParam& param, const Color4f& color)
	{
		if (texture.isNotNull()) {
			drawTexture(rectDst, texture, Rectangle(0, 0, (sl_real)(texture->getWidth()), (sl_real)(texture->getHeight())), param, color);
		}
	}

	void RenderCanvas::drawTexture(const Rectangle& rectDst, const Ref<Texture>& texture, const DrawParam& param)
	{
		drawTexture(rectDst, texture, param, Color4f(1, 1, 1, 1));
	}

	void RenderCanvas::drawTexture(const Rectangle& rectDst, const Ref<Texture>& texture, sl_real alpha)
	{
		drawTexture(rectDst, texture, DrawParam(), Color4f(1, 1, 1, alpha));
	}

	Matrix3 RenderCanvas::getTransformMatrixForRectangle(const Rectangle& rect)
	{
		RenderCanvasState* canvasState = m_state.get();
		Matrix3 mat;
		mat.m00 = rect.getWidth(); mat.m10 = 0; mat.m20 = rect.left;
		mat.m01 = 0; mat.m11 = rect.getHeight(); mat.m21 = rect.top;
		mat.m02 = 0; mat.m12 = 0; mat.m22 = 1;
		mat *= canvasState->matrix;
		mat *= m_matViewport;
		return mat;
	}

	void RenderCanvas::drawRectangle(const Rectangle& rect, RenderProgramState2D_Position* programState, const DrawParam& param)
	{
		EngineContext* context = GetEngineContext(this);
		if (!context) {
			return;
		}

		RenderCanvasState* canvasState = m_state.get();

		Matrix3 mat;
		mat.m00 = rect.getWidth(); mat.m10 = 0; mat.m20 = rect.left;
		mat.m01 = 0; mat.m11 = rect.getHeight(); mat.m21 = rect.top;
		mat.m02 = 0; mat.m12 = 0; mat.m22 = 1;
		mat *= canvasState->matrix;
		mat *= m_matViewport;
		programState->setTransform(mat);

		Color4f color(1, 1, 1, param.alpha * getAlpha());
		programState->setColor(color);

		m_engine->drawPrimitive(4, context->vbRectangle, PrimitiveType::TriangleStrip);

	}

	void RenderCanvas::_drawBitmap(const Rectangle& rectDst, Bitmap* src, const Rectangle& rectSrc, const DrawParam& param)
	{
		Ref<Texture> texture = Texture::getBitmapRenderingCache(src);
		if (texture.isNull()) {
			return;
		}
		drawTexture(rectDst, texture, rectSrc, param, Color4f(1, 1, 1, 1));
	}

	void RenderCanvas::onDrawText(const StringParam& _text, sl_real x, sl_real y, const Ref<Font>& font, const DrawTextParam& param)
	{
		StringData16 text(_text);
		if (text.isEmpty()) {
			return;
		}

		sl_char16* arrChar = text.getData();
		sl_size len = text.getLength();
		sl_real fontHeight = font->getFontHeight();
		sl_bool fontItalic = font->isItalic();

		Ref<FontAtlas> fa = font->getSharedAtlas();
		if (fa.isNull()) {
			return;
		}

		EngineContext* context = GetEngineContext(this);
		if (!context) {
			return;
		}

		RenderCanvasState* state = m_state.get();
		if (state->flagClipRect) {
			if (state->clipRect.top >= y + fontHeight || state->clipRect.bottom <= y || state->clipRect.right <= x) {
				return;
			}
		}

		RenderCanvasProgramParam pp;
		pp.prepare(state, !fontItalic);
		pp.flagUseTexture = sl_true;

		RenderProgramScope<RenderCanvasProgramState> scope;
		sl_bool flagBeginScope = sl_false;
		Ref<Texture> textureBefore;

		FontAtlasChar fac;
		Color4f color = param.color;
		sl_real fx = x;

		for (sl_size i = 0; i < len; i++) {

			sl_char32 ch = arrChar[i];
			if (ch >= 0xD800 && ch < 0xE000) {
				if (i + 1 < len) {
					sl_uint32 ch1 = (sl_uint32)((sl_uint16)arrChar[++i]);
					if (ch < 0xDC00 && ch1 >= 0xDC00 && ch1 < 0xE000) {
						ch = (sl_char32)(((ch - 0xD800) << 10) | (ch1 - 0xDC00)) + 0x10000;
					} else {
						ch = 0;
					}
				} else {
					ch = 0;
				}
			}

			if (ch && fa->getChar(ch, fac)) {

				sl_real fw = fac.fontWidth;
				sl_real fh = fac.fontHeight;
				sl_real fxn = fx + fw;

				if (fac.bitmap.isNotNull()) {

					Rectangle rcDst;
					rcDst.left = fx;
					rcDst.right = fxn;
					rcDst.top = y + (fontHeight - fh);
					rcDst.bottom  = rcDst.top + fh;

					Rectangle rcClip;

					sl_bool flagIgnore = sl_false;
					sl_bool flagClip = sl_false;

					if (state->flagClipRect) {
						if (state->clipRect.right <= fx) {
							return;
						}
						if (state->clipRect.intersectRectangle(rcDst, &rcClip)) {
							if (!fontItalic) {
								if (!(state->clipRect.containsRectangle(rcDst))) {
									flagClip = sl_true;
								}
							}
						} else {
							flagIgnore = sl_true;
						}
					}
					if (!flagIgnore) {
						Ref<Texture> texture = Texture::getBitmapRenderingCache(fac.bitmap);
						if (texture.isNotNull()) {
							sl_real sw = (sl_real)(texture->getWidth());
							sl_real sh = (sl_real)(texture->getHeight());
							if (sw > SLIB_EPSILON && sh > SLIB_EPSILON) {
								if (!flagBeginScope) {
									Ref<RenderProgram> program = context->getProgram(pp);
									if (!(scope.begin(m_engine.get(), program))) {
										return;
									}
									scope->setColor(Color4f(color.x, color.y, color.z, color.w * getAlpha()));
									flagBeginScope = sl_true;
								}

								Rectangle rcSrc;
								rcSrc.left = (sl_real)(fac.region.left) / sw;
								rcSrc.top = (sl_real)(fac.region.top) / sh;
								rcSrc.right = (sl_real)(fac.region.right) / sw;
								rcSrc.bottom = (sl_real)(fac.region.bottom) / sh;
								if (flagClip) {
									rcSrc = GraphicsUtil::transformRectangle(rcSrc, rcDst, rcClip);
									rcDst = rcClip;
								}
								Matrix3 mat;
								if (fontItalic) {
									float ratio = 0.2f;
									mat.m00 = fw; mat.m10 = -ratio * fh; mat.m20 = ratio * fh + rcDst.left;
									mat.m01 = 0; mat.m11 = fh; mat.m21 = rcDst.top;
									mat.m02 = 0; mat.m12 = 0; mat.m22 = 1;
								} else {
									mat.m00 = rcDst.getWidth(); mat.m10 = 0; mat.m20 = rcDst.left;
									mat.m01 = 0; mat.m11 = rcDst.getHeight(); mat.m21 = rcDst.top;
									mat.m02 = 0; mat.m12 = 0; mat.m22 = 1;
								}
								pp.applyToProgramState(scope.getState(), mat);
								mat *= state->matrix;
								mat *= m_matViewport;
								scope->setTransform(mat);
								Ref<TextureInstance> textureInstance = m_engine->linkTexture(texture);
								if (textureBefore != texture || (textureInstance.isNotNull() && textureInstance->isUpdated())) {
									scope->setTexture(texture);
									textureBefore = texture;
								}
								scope->setRectSrc(Vector4(rcSrc.left, rcSrc.top, rcSrc.getWidth(), rcSrc.getHeight()));
								m_engine->drawPrimitive(4, context->vbRectangle, PrimitiveType::TriangleStrip);
							}
						}
					}
				}
				fx = fxn;
			}
		}

		if (font->isStrikeout() || font->isUnderline()) {
			Ref<Pen> pen = Pen::createSolidPen(1, param.color);
			FontMetrics fm;
			font->getFontMetrics(fm);
			if (font->isUnderline()) {
				sl_real yLine = y + fm.leading + fm.ascent;
				drawLine(Point(x, yLine), Point(fx, yLine), pen);
			}
			if (font->isStrikeout()) {
				sl_real yLine = y + fm.leading + fm.ascent / 2;
				drawLine(Point(x, yLine), Point(fx, yLine), pen);
			}
		}

	}

	void RenderCanvas::onDraw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc, const DrawParam& param)
	{
		if (src->isBitmap()) {
			_drawBitmap(rectDst, (Bitmap*)(src.get()), rectSrc, param);
		} else {
			CanvasExt::onDraw(rectDst, src, rectSrc, param);
		}
	}

	void RenderCanvas::onDrawAll(const Rectangle& rectDst, const Ref<Drawable>& src, const DrawParam& param)
	{
		if (src->isBitmap()) {
			_drawBitmap(rectDst, (Bitmap*)(src.get()), Rectangle(0, 0, src->getDrawableWidth(), src->getDrawableHeight()), param);
		} else {
			CanvasExt::onDrawAll(rectDst, src, param);
		}
	}

	void RenderCanvas::_setAlpha(sl_real alpha)
	{
	}

	void RenderCanvas::_setAntiAlias(sl_bool flag)
	{
	}

}
