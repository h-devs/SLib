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

#ifndef CHECKHEADER_SLIB_GRAPHICS_CONSTANTS
#define CHECKHEADER_SLIB_GRAPHICS_CONSTANTS

#include "definition.h"

#include "../core/flags.h"
#include "../doc/file_type.h"

namespace slib
{

	enum class PenStyle
	{
		Solid = 0,
		Dot = 1,
		Dash = 2,
		DashDot = 3,
		DashDotDot = 4,
		Default = 0x80
	};

	enum class LineCap
	{
		Flat = 0,
		Round = 1,
		Square = 2,
		Default = 0x80
	};

	enum class LineJoin
	{
		Miter = 0,
		Round = 1,
		Bevel = 2,
		Default = 0x80
	};

	enum class BrushStyle
	{
		Solid = 0,
		LinearGradient = 1,
		RadialGradient = 2,
		Texture = 3,
		Hatch = 4
	};

	enum class HatchStyle
	{
		Solid = 0,
		Horizontal = 1,
		Vertical = 2,
		ForwardDiagonal = 3,
		BackwardDiagonal = 4,
		Cross = 5,
		DiagonalCross = 6,
		Dots = 7,
		Count = 8
	};

	enum class FillMode
	{
		Winding = 0,
		Alternate = 1
	};

	SLIB_DEFINE_FLAGS(Alignment, {
		HorizontalMask = 3,
		VerticalMask = 12,

		Default = 0,
		Left = 1,
		Right = 2,
		Center = 3,

		Top = 4,
		Bottom = 8,
		Middle = 12,

		TopLeft = 5,
		TopRight = 6,
		TopCenter = 7,
		BottomLeft = 9,
		BottomRight = 10,
		BottomCenter = 11,
		MiddleLeft = 13,
		MiddleRight = 14,
		MiddleCenter = 15
	})

	enum class StretchMode
	{
		Nearest = 0,
		Linear = 1,
		Box = 2,

		Default = Box
	};

	enum class BlendMode
	{
		Copy = 0,
		Over = 1
	};

	enum class RotationMode
	{
		Rotate0 = 0,
		Rotate90 = 90,
		Rotate180 = 180,
		Rotate270 = 270
	};

	constexpr RotationMode operator+(RotationMode a, RotationMode b)
	{
		return (RotationMode)(((int)a + (int)b) % 360);
	}

	constexpr RotationMode operator-(RotationMode a, RotationMode b)
	{
		return (RotationMode)(((int)a + 360 - (int)b) % 360);
	}

	constexpr RotationMode operator-(RotationMode a)
	{
		return (RotationMode)((360 - (int)a) % 360);
	}

	enum class FlipMode
	{
		None = 0,
		Horizontal = 1,
		Vertical = 2,
		Both = 3 // Same effect with `RotationMode::Rotate180`
	};

	constexpr FlipMode operator*(FlipMode a, FlipMode b)
	{
		return (FlipMode)((int)a ^ (int)b);
	};

	// Convert Flip::Both to 180 rotation, and then Convert 180 rotation to flip, and then Convert Flip::Vertical to Flip::Horizontal on 90/270 rotation
	SLIB_INLINE void NormalizeRotateAndFlip(RotationMode& rotation, FlipMode& flip)
	{
		if (flip == FlipMode::Both) {
			rotation = rotation + RotationMode::Rotate180;
			flip = FlipMode::None;
			return;
		}
		if (flip == FlipMode::None) {
			return;
		}
		if (rotation == RotationMode::Rotate0) {
			return;
		}
		if (rotation == RotationMode::Rotate180) {
			if (flip == FlipMode::Horizontal) {
				flip = FlipMode::Vertical;
			} else { // always FlipMode::Vertical;
				flip = FlipMode::Horizontal;
			}
			rotation = RotationMode::Rotate0;
			return;
		}
		if (flip == FlipMode::Vertical) {
			rotation = -rotation;
			flip = FlipMode::Horizontal;
		}
	}

	template <class T>
	SLIB_INLINE void RotatePoint(T& x, T& y, T w, T h, RotationMode rotation)
	{
		T sx = x;
		T sy = y;
		switch (rotation) {
			case RotationMode::Rotate90:
				x = h - sy;
				y = sx;
				break;
			case RotationMode::Rotate180:
				x = w - sx;
				y = h - sy;
				break;
			case RotationMode::Rotate270:
				x = sy;
				y = w - sx;
				break;
			default:
				break;
		}
	}

	template <class T>
	SLIB_INLINE void FlipPoint(T& x, T& y, T w, T h, FlipMode flip)
	{
		switch (flip) {
			case FlipMode::Horizontal:
				x = w - x;
				break;
			case FlipMode::Vertical:
				y = h - y;
				break;
			case FlipMode::Both:
				x = w - x;
				y = h - y;
				break;
			default:
				break;
		}
	}

	enum class ScaleMode
	{
		None = 0,
		Stretch = 1,
		Contain = 2,
		Cover = 3
	};

	enum class TileMode
	{
		Repeat,
		Mirror,
		Clamp
	};

	enum class ColorSpace
	{
		None = 0,
		RGB = 1,
		YUV = 2,
		CMYK = 3,
		HLS = 4,
		HSV = 5
	};

	enum class MultiLineMode
	{
		Single, // Single line
		Multiple, // Break only at CR/LF
		WordWrap, // Break at TAB, Spaces, CR/LF
		BreakWord, // Break at any position
		LatinWrap // Break at non-latin characters
	};

	SLIB_INLINE SLIB_CONSTEXPR sl_bool IsWrappingMultiLineMode(MultiLineMode mode)
	{
		return mode == MultiLineMode::WordWrap || mode == MultiLineMode::BreakWord || mode == MultiLineMode::LatinWrap;
	}

	enum class EllipsizeMode
	{
		None = 0,
		End = 1,
		Start = 2,
		Middle = 3
	};

	enum class BoundShape
	{
		None = 0,
		Rectangle = 1,
		Ellipse = 2,
		RoundRect = 3,
		Path = 10
	};

	enum class AntiAliasMode
	{
		False = 0,
		True = 1,
		Inherit = 2
	};

	typedef FileType ImageFileType;

}

#endif
