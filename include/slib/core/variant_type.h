/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_VARIANT_TYPE
#define CHECKHEADER_SLIB_CORE_VARIANT_TYPE

#include "definition.h"

namespace slib
{

	class VariantType
	{
	public:
		enum Types : sl_uint8
		{
			Null = 0,
			Int32 = 1,
			Uint32 = 2,
			Int64 = 3,
			Uint64 = 4,
			Float = 5,
			Double = 6,
			Boolean = 7,
			String8 = 8,
			String16 = 9,
			String32 = 10,
			Sz8 = 11,
			Sz16 = 12,
			Sz32 = 13,
			StringData8 = 14,
			StringData16 = 15,
			StringData32 = 16,
			Time = 17,
			Pointer = 18,
			ObjectId = 19,
			Referable = 64,
			Weak = Referable + 1,
			Object = Referable + 2,
			Collection = Referable + 3,
			Map = Referable + 4,
			List = Referable + 5,
			Memory = Referable + 6,
			BigInt = Referable + 7,
			Promise = Referable + 8,
			Function = Referable + 9
		};

	};

	constexpr static sl_bool operator==(sl_uint8 v1, VariantType::Types v2)
	{
		return v1 == (sl_uint8)v2;
	}

	constexpr static sl_bool operator==(VariantType::Types v2, sl_uint8 v1)
	{
		return v1 == (sl_uint8)v2;
	}

	constexpr static sl_bool operator!=(sl_uint8 v1, VariantType::Types v2)
	{
		return v1 != (sl_uint8)v2;
	}

	constexpr static sl_bool operator!=(VariantType::Types v2, sl_uint8 v1)
	{
		return v1 != (sl_uint8)v2;
	}

}

#endif
