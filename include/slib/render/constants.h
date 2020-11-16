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

#ifndef CHECKHEADER_SLIB_RENDER_CONSTANTS
#define CHECKHEADER_SLIB_RENDER_CONSTANTS

#include "../graphics/constants.h"

namespace slib
{

	enum class RenderBlendingOperation
	{
		Add = 0,
		Subtract = 1,
		ReverseSubtract = 2
	};

	enum class RenderBlendingFactor
	{
		One = 0,
		Zero = 1,
		SrcAlpha = 2,
		OneMinusSrcAlpha = 3,
		DstAlpha = 4,
		OneMinusDstAlpha = 5,
		SrcColor = 6,
		OneMinusSrcColor = 7,
		DstColor = 8,
		OneMinusDstColor = 9,
		SrcAlphaSaturate = 10, // f = min(As, 1 - Ad)
		Constant = 11,
		OneMinusConstant = 12
	};

	enum class RenderFunctionOperation
	{
		Never = 0,
		Always = 1,
		Equal = 2,
		NotEqual = 3,
		Less = 4,
		LessEqual = 5,
		Greater = 6,
		GreaterEqual = 7,
	};

	enum class TextureFilterMode
	{
		Point,
		Linear
	};

	typedef TileMode TextureWrapMode;

	enum class RenderUniformType
	{
		Float = 0,
		Float2 = 1,
		Float3 = 2,
		Float4 = 4,
		Int = 0x10,
		Int2 = 0x11,
		Int3 = 0x12,
		Int4 = 0x13,
		Matrix3 = 0x21,
		Matrix4 = 0x22,
		Sampler = 0x30
	};

	enum class RenderInputType
	{
		Float = 0,
		Float2 = 1,
		Float3 = 2,
		Float4 = 3,
		UByte4 = 0x10,
		Short2 = 0x11,
		Short4 = 0x12
	};

	enum class RenderInputSemanticName
	{
		Undefined = 255,
		Position = 0,
		BlendWeight = 1,
		BlendIndices = 2,
		Normal = 3,
		PSize = 4,
		TexCoord = 5,
		Tangent = 6,
		BiNormal = 7,
		TessFactor = 8,
		PositionT = 9,
		Color = 10,
		Fog = 11,
		Depth = 12
	};

	enum class RenderShaderType
	{
		Undefined = 0,
		Vertex = 1,
		Pixel = 2
	};

	SLIB_INLINE constexpr RenderShaderType operator|(RenderShaderType v1, RenderShaderType v2)
	{
		return (RenderShaderType)(((int)v1) | ((int)v2));
	}

	SLIB_INLINE constexpr int operator&(RenderShaderType v1, RenderShaderType v2)
	{
		return ((int)v1) & ((int)v2);
	}

}
#endif
