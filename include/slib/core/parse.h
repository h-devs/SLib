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

#ifndef CHECKHEADER_SLIB_CORE_PARSE
#define CHECKHEADER_SLIB_CORE_PARSE

#include "definition.h"

#define SLIB_PARSE_ERROR (-1)

#define SLIB_DECLARE_CLASS_PARSE_MEMBERS(CLASS) \
	static sl_reg parse(CLASS* _out, const sl_char8* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX); \
	static sl_reg parse(CLASS* _out, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX); \
	static sl_reg parse(CLASS* _out, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX); \
	sl_bool parse(const slib::StringParam& str);

#define SLIB_DECLARE_CLASS_PARSE2_MEMBERS(CLASS, ARG_TYPE, ARG_NAME) \
	static sl_reg parse(CLASS* _out, ARG_TYPE ARG_NAME, const sl_char8* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX); \
	static sl_reg parse(CLASS* _out, ARG_TYPE ARG_NAME, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX); \
	static sl_reg parse(CLASS* _out, ARG_TYPE ARG_NAME, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX); \
	sl_bool parse(const slib::StringParam& str, ARG_TYPE ARG_NAME);

#define SLIB_DECLARE_CLASS_PARSE_INT_MEMBERS(CLASS) \
	static sl_reg parse(CLASS* _out, sl_uint32 radix, const sl_char8* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX); \
	static sl_reg parse(CLASS* _out, sl_uint32 radix, const sl_char16* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX); \
	static sl_reg parse(CLASS* _out, sl_uint32 radix, const sl_char32* str, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX); \
	sl_bool parse(const slib::StringParam& str, sl_uint32 radix = 10);

#define SLIB_DEFINE_CLASS_PARSE_MEMBERS(CLASS, BASE_PARSER_FUNCTION) \
	sl_reg CLASS::parse(CLASS* _out, const sl_char8* str, sl_size posBegin, sl_size posEnd) \
	{ \
		return BASE_PARSER_FUNCTION(_out, str, posBegin, posEnd); \
	} \
	sl_reg CLASS::parse(CLASS* _out, const sl_char16* str, sl_size posBegin, sl_size posEnd) \
	{ \
		return BASE_PARSER_FUNCTION(_out, str, posBegin, posEnd); \
	} \
	sl_reg CLASS::parse(CLASS* _out, const sl_char32* str, sl_size posBegin, sl_size posEnd) \
	{ \
		return BASE_PARSER_FUNCTION(_out, str, posBegin, posEnd); \
	} \
	sl_bool CLASS::parse(const slib::StringParam& _str) \
	{ \
		if (_str.isNotNull()) { \
			StringRawData str; \
			_str.getData(str); \
			if (str.charSize == 1) { \
				if (str.length < 0) { \
					sl_reg ret = BASE_PARSER_FUNCTION(this, str.data8, 0, SLIB_SIZE_MAX); \
					return ret != SLIB_PARSE_ERROR && !((str.data8)[ret]); \
				} else { \
					return BASE_PARSER_FUNCTION(this, str.data8, 0, str.length) == (sl_reg)(str.length); \
				} \
			} else if (str.charSize == 2) { \
				if (str.length < 0) { \
					sl_reg ret = BASE_PARSER_FUNCTION(this, str.data16, 0, SLIB_SIZE_MAX); \
					return ret != SLIB_PARSE_ERROR && !((str.data16)[ret]); \
				} else { \
					return BASE_PARSER_FUNCTION(this, str.data16, 0, str.length) == (sl_reg)(str.length); \
				} \
			} else { \
				if (str.length < 0) { \
					sl_reg ret = BASE_PARSER_FUNCTION(this, str.data32, 0, SLIB_SIZE_MAX); \
					return ret != SLIB_PARSE_ERROR && !((str.data32)[ret]); \
				} else { \
					return BASE_PARSER_FUNCTION(this, str.data32, 0, str.length) == (sl_reg)(str.length); \
				} \
			} \
		} \
		return sl_false; \
	}

#define SLIB_DEFINE_CLASS_PARSE2_MEMBERS(CLASS, ARG_TYPE, ARG_NAME, BASE_PARSER_FUNCTION) \
	sl_reg CLASS::parse(CLASS* _out, ARG_TYPE ARG_NAME, const sl_char8* str, sl_size posBegin, sl_size posEnd) \
	{ \
		return BASE_PARSER_FUNCTION(_out, ARG_NAME, str, posBegin, posEnd); \
	} \
	sl_reg CLASS::parse(CLASS* _out, ARG_TYPE ARG_NAME, const sl_char16* str, sl_size posBegin, sl_size posEnd) \
	{ \
		return BASE_PARSER_FUNCTION(_out, ARG_NAME, str, posBegin, posEnd); \
	} \
	sl_bool CLASS::parse(const slib::StringParam& _str, ARG_TYPE ARG_NAME) \
	{ \
		if (_str.isNotNull()) { \
			StringRawData str; \
			_str.getData(str); \
			if (str.charSize == 1) { \
				if (str.length < 0) { \
					sl_reg ret = BASE_PARSER_FUNCTION(this, ARG_NAME, str.data8, 0, SLIB_SIZE_MAX); \
					return ret != SLIB_PARSE_ERROR && !((str.data8)[ret]); \
				} else { \
					return BASE_PARSER_FUNCTION(this, ARG_NAME, str.data8, 0, str.length) == (sl_reg)(str.length); \
				} \
			} else if (str.charSize == 2) { \
				if (str.length < 0) { \
					sl_reg ret = BASE_PARSER_FUNCTION(this, ARG_NAME, str.data16, 0, SLIB_SIZE_MAX); \
					return ret != SLIB_PARSE_ERROR && !((str.data16)[ret]); \
				} else { \
					return BASE_PARSER_FUNCTION(this, ARG_NAME, str.data16, 0, str.length) == (sl_reg)(str.length); \
				} \
			} else { \
				if (str.length < 0) { \
					sl_reg ret = BASE_PARSER_FUNCTION(this, ARG_NAME, str.data32, 0, SLIB_SIZE_MAX); \
					return ret != SLIB_PARSE_ERROR && !((str.data32)[ret]); \
				} else { \
					return BASE_PARSER_FUNCTION(this, ARG_NAME, str.data32, 0, str.length) == (sl_reg)(str.length); \
				} \
			} \
		} \
		return sl_false; \
	}

#define SLIB_DEFINE_CLASS_PARSE_INT_MEMBERS(CLASS, BASE_PARSER_FUNCTION) SLIB_DEFINE_CLASS_PARSE2_MEMBERS(CLASS, sl_uint32, radix, BASE_PARSER_FUNCTION)

namespace slib
{

	class StringParam;

}

#endif
