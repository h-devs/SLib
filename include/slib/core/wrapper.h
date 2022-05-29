/*
*   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_WRAPPER
#define CHECKHEADER_SLIB_CORE_WRAPPER

#include "default_members.h"

#define SLIB_DECLARE_WRAPPER_DEFAULT_MEMBERS(CLASS, BASE_CLASS, BASE_MEMBER) \
		BASE_CLASS BASE_MEMBER; \
	public: \
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CLASS) \
		CLASS(const BASE_CLASS& other); \
		CLASS(BASE_CLASS&& other); \
		CLASS& operator=(const BASE_CLASS& other); \
		CLASS& operator=(BASE_CLASS&& other);

#define SLIB_DEFINE_WRAPPER_DEFAULT_MEMBERS(CLASS, BASE_CLASS, BASE_MEMBER) \
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CLASS) \
	CLASS::CLASS(const BASE_CLASS& other): BASE_MEMBER(other) {} \
	CLASS::CLASS(BASE_CLASS&& other): BASE_MEMBER(Move(other)) {} \
	CLASS& CLASS::operator=(const BASE_CLASS& other) { BASE_MEMBER = other; } \
	CLASS& CLASS::operator=(BASE_CLASS&& other) { BASE_MEMBER = Move(other); return *this; }

#define SLIB_DEFINE_WRAPPER_DEFAULT_MEMBERS_INLINE(CLASS, BASE_CLASS, BASE_MEMBER) \
		BASE_CLASS BASE_MEMBER; \
	public: \
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(CLASS) \
		CLASS(const BASE_CLASS& other): BASE_MEMBER(other) {} \
		CLASS(BASE_CLASS&& other): BASE_MEMBER(Move(other)) {} \
		CLASS& operator=(const BASE_CLASS& other) { BASE_MEMBER = other; } \
		CLASS& operator=(BASE_CLASS&& other) { BASE_MEMBER = Move(other); return *this; }

#endif
