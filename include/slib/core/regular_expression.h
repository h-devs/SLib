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

#ifndef CHECKHEADER_SLIB_CORE_REGULAR_EXPRESSION
#define CHECKHEADER_SLIB_CORE_REGULAR_EXPRESSION

#include "string.h"
#include "flags.h"
#include "handle_container.h"

namespace slib
{

	SLIB_DEFINE_FLAGS(RegularExpressionFlags, {
		Default = 0,
		Icase = 0x0001,
		Nosubs = 0x0002,
		Optimize = 0x0004,
		Collate = 0x0008,
		ECMAScript = 0x0010,
		Basic = 0x0020,
		Extended = 0x0040,
		Awk = 0x0080,
		Grep = 0x0100,
		Egrep = 0x0200
	})

	SLIB_DEFINE_FLAGS(RegularExpressionMatchFlags, {
		Default = 0,
		NotBol = 0x0001,
		NotEol = 0x0002,
		NotBow = 0x0004,
		NotEow = 0x0008,
		Any = 0x0010,
		NotNull = 0x0020,
		Continuous = 0x0040,
		PrevAvail = 0x0080,
		FormatSed = 0x0100,
		FormatNoCopy = 0x0200,
		FormatFirstOnly = 0x0400
	})

	typedef struct HRegEx_ *HRegEx;

	class RegularExpression
	{
		SLIB_DECLARE_HANDLE_CONTAINER_MEMBERS(RegularExpression, HRegEx, m_handle, sl_null)

	public:
		RegularExpression(const StringParam& pattern) noexcept;
		
		RegularExpression(const StringParam& pattern, const RegularExpressionFlags& flags) noexcept;

	public:
		sl_bool match(const StringParam& str, const RegularExpressionMatchFlags& flags = RegularExpressionMatchFlags::Default) noexcept;
		
		static sl_bool matchEmail(const StringParam& str) noexcept;

	};
	
}

#endif
