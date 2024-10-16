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

#ifndef CHECKHEADER_SLIB_RENDER_PROGRAM_EXT
#define CHECKHEADER_SLIB_RENDER_PROGRAM_EXT

#include "program.h"

namespace slib
{

	namespace render2d
	{
		namespace vertex
		{
			struct SLIB_EXPORT PositionTexture
			{
				Vector2 position;
				Vector2 texCoord;
			};
		}
		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(PositionTexture, vertex::PositionTexture)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(Transform, u_Transform, RenderShaderStage::Vertex, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(TextureTransform, u_TextureTransform, RenderShaderStage::Vertex, 3)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(Texture, u_Texture, RenderShaderStage::Pixel, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color, RenderShaderStage::Pixel, 0)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position, RenderInputSemanticName::Position)
				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoord, a_TexCoord, RenderInputSemanticName::TexCoord)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT PositionTexture : public RenderProgramT<state::PositionTexture>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};

			class SLIB_EXPORT PositionTextureYUV : public PositionTexture
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};

			class SLIB_EXPORT PositionTextureOES : public PositionTexture
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};
		}

		namespace vertex
		{
			struct SLIB_EXPORT PositionColor
			{
				Vector2 position;
				Color4F color;
			};
		}
		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(PositionColor, vertex::PositionColor)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(Transform, u_Transform, RenderShaderStage::Vertex, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color, RenderShaderStage::Vertex, 3)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position, RenderInputSemanticName::Position)
				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT4(color, a_Color, RenderInputSemanticName::Color)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT PositionColor : public RenderProgramT<state::PositionColor>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};
		}

		namespace vertex
		{
			struct SLIB_EXPORT Position
			{
				Vector2 position;
			};
		}
		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(Position, vertex::Position)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(Transform, u_Transform, RenderShaderStage::Vertex, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color, RenderShaderStage::Pixel, 0)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position, RenderInputSemanticName::Position)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT Position : public RenderProgramT<state::Position>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};
		}

		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(HatchFill, vertex::Position)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(Transform, u_Transform, RenderShaderStage::Vertex, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(HatchTransform, u_HatchTransform, RenderShaderStage::Vertex, 3)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(ForeColor, u_ForeColor, RenderShaderStage::Pixel, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(BackColor, u_BackColor, RenderShaderStage::Pixel, 1)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(LineWidth, hatchLineWidth, RenderShaderStage::Pixel, 2)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(SmoothWidth, hatchSmoothWidth, RenderShaderStage::Pixel, 3)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position, RenderInputSemanticName::Position)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT HatchFill : public RenderProgramT<state::HatchFill>
			{
			public:
				HatchFill(HatchStyle style): m_style(style) {}

			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
				static String getShader(RenderShaderType type, HatchStyle style);

			public:
				// Input Variable: `hatch`, `hatchLineWidth`, `hatchSmoothWidth`
				// Output variables: `hatchFactor`
				static String getShaderSnippet(RenderShaderLanguage lang, HatchStyle style);

			protected:
				HatchStyle m_style;
			};
		}
	}

	namespace render3d
	{
		namespace vertex
		{
			struct SLIB_EXPORT PositionNormalColor
			{
				Vector3 position;
				Vector3 normal;
				Color4F color;
			};
		}
		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(PositionNormalColor, vertex::PositionNormalColor)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(MatrixModelViewIT, u_MatrixModelViewIT)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DirectionalLight, u_DirectionalLight)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DiffuseColor, u_DiffuseColor)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(AmbientColor, u_AmbientColor)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(Alpha, u_Alpha)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(normal, a_Normal)
				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT4(color, a_Color)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT PositionNormalColor : public RenderProgramT<state::PositionNormalColor>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};
		}

		namespace vertex
		{
			struct SLIB_EXPORT PositionColor
			{
				Vector3 position;
				Color4F color;
			};
		}
		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(PositionColor, vertex::PositionColor)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT4(color, a_Color)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT PositionColor : public RenderProgramT<state::PositionColor>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};
		}

		namespace vertex
		{
			struct SLIB_EXPORT PositionNormalTexture
			{
				Vector3 position;
				Vector3 normal;
				Vector2 texCoord;
			};
		}
		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(PositionNormalTexture, vertex::PositionNormalTexture)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(MatrixModelViewIT, u_MatrixModelViewIT)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DirectionalLight, u_DirectionalLight)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DiffuseColor, u_DiffuseColor)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(AmbientColor, u_AmbientColor)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(Alpha, u_Alpha)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(Texture, u_Texture, RenderShaderStage::Pixel, 0)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(normal, a_Normal)
				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoord, a_TexCoord)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT PositionNormalTexture : public RenderProgramT<state::PositionNormalTexture>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};
		}

		namespace vertex
		{
			struct SLIB_EXPORT PositionTexture
			{
				Vector3 position;
				Vector2 texCoord;
			};
		}
		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(PositionTexture, vertex::PositionTexture)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DiffuseColor, u_Color)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(Texture, u_Texture, RenderShaderStage::Pixel, 0)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoord, a_TexCoord)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT PositionTexture : public RenderProgramT<state::PositionTexture>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};
		}

		namespace vertex
		{
			struct SLIB_EXPORT PositionNormal
			{
				Vector3 position;
				Vector3 normal;
			};
		}
		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(PositionNormal, vertex::PositionNormal)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(MatrixModelViewIT, u_MatrixModelViewIT)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DirectionalLight, u_DirectionalLight)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DiffuseColor, u_DiffuseColor)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(AmbientColor, u_AmbientColor)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(Alpha, u_Alpha)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(normal, a_Normal)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT PositionNormal : public RenderProgramT<state::PositionNormal>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};
		}

		namespace vertex
		{
			struct SLIB_EXPORT Position
			{
				Vector3 position;
			};
		}
		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(Position, vertex::Position)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform, RenderShaderStage::Vertex, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color, RenderShaderStage::Pixel, 0)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position, RenderInputSemanticName::Position)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT Position : public RenderProgramT<state::Position>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
				static String getShader(RenderShaderType type);
			};
		}

		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(Position2D, render2d::vertex::Position)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform, RenderShaderStage::Vertex, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color, RenderShaderStage::Pixel, 0)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position, RenderInputSemanticName::Position)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT Position2D : public RenderProgramT<state::Position2D>
			{
			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;
			};
		}

		namespace state
		{
			SLIB_RENDER_PROGRAM_STATE_BEGIN(HatchFill2D, render2d::vertex::Position)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform, RenderShaderStage::Vertex, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(HatchTransform, u_HatchTransform, RenderShaderStage::Vertex, 4)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(ForeColor, u_ForeColor, RenderShaderStage::Pixel, 0)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(BackColor, u_BackColor, RenderShaderStage::Pixel, 1)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(LineWidth, hatchLineWidth, RenderShaderStage::Pixel, 2)
				SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(SmoothWidth, hatchSmoothWidth, RenderShaderStage::Pixel, 3)

				SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position, RenderInputSemanticName::Position)
			SLIB_RENDER_PROGRAM_STATE_END
		}
		namespace program
		{
			class SLIB_EXPORT HatchFill2D : public RenderProgramT<state::HatchFill2D>
			{
			public:
				HatchFill2D(HatchStyle style): m_style(style) {}

			public:
				String getShader(RenderEngine* engine, RenderShaderType type) override;

			protected:
				HatchStyle m_style;
			};
		}
	}

}

#endif

