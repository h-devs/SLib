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

#ifndef CHECKHEADER_SLIB_CORE_DEFAULT_MEMBERS
#define CHECKHEADER_SLIB_CORE_DEFAULT_MEMBERS

#define SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CLASS) \
	public: \
		~CLASS(); \
		CLASS(CLASS const& other); \
		CLASS(CLASS&& other); \
		CLASS& operator=(CLASS const& other); \
		CLASS& operator=(CLASS&& other);

#define SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CLASS) \
	CLASS::~CLASS() {} \
	CLASS::CLASS(CLASS const& other) = default; \
	CLASS::CLASS(CLASS&& other) = default; \
	CLASS& CLASS::operator=(CLASS const& other) = default; \
	CLASS& CLASS::operator=(CLASS&& other) = default;

#define SLIB_DEFINE_MEMBER_CLASS_DEFAULT_MEMBERS(PARENT, CLASS) \
	PARENT::CLASS::~CLASS() {} \
	PARENT::CLASS::CLASS(PARENT::CLASS const& other) = default; \
	PARENT::CLASS::CLASS(PARENT::CLASS&& other) = default; \
	PARENT::CLASS& PARENT::CLASS::operator=(PARENT::CLASS const& other) = default; \
	PARENT::CLASS& PARENT::CLASS::operator=(PARENT::CLASS&& other) = default;

#define SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(CLASS) \
	public: \
		CLASS(CLASS const& other) noexcept = default; \
		CLASS(CLASS&& other) noexcept = default; \
		CLASS& operator=(CLASS const& other) noexcept = default; \
		CLASS& operator=(CLASS&& other) noexcept = default;

#define SLIB_DELETE_CLASS_DEFAULT_MEMBERS(CLASS) \
	public: \
		CLASS(CLASS const& other) = delete; \
		CLASS(CLASS&& other) = delete; \
		CLASS& operator=(CLASS const& other) = delete; \
		CLASS& operator=(CLASS&& other) = delete;

#endif
