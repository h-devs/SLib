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

#ifndef CHECKHEADER_SLIB_CORE_SERIALIZE_STRING
#define CHECKHEADER_SLIB_CORE_SERIALIZE_STRING

#include "primitive.h"
#include "variable_length_integer.h"

#include "../string.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <string>
#endif

namespace slib
{

	template <class OUTPUT>
	static sl_bool SerializeString(OUTPUT* output, const sl_char8* str, sl_size len)
	{
		if (!(CVLI::serialize(output, len))) {
			return sl_false;
		}
		if (len) {
			return SerializeRaw(output, str, len);
		} else {
			return sl_true;
		}
	}

	sl_bool Serialize(MemoryBuffer* output, const String& _in);

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const String& _in)
	{
		sl_size len = _in.getLength();
		if (!(CVLI::serialize(output, len))) {
			return sl_false;
		}
		if (len) {
			return SerializeRaw(output, _in.getData(), len);
		} else {
			return sl_true;
		}
	}

	sl_bool Deserialize(SerializeBuffer* input, String& _out);

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, String& _out)
	{
		sl_size len;
		if (!(CVLI::deserialize(input, len))) {
			return sl_false;
		}
		if (len) {
			String ret = String::allocate(len);
			if (ret.isNull()) {
				return sl_false;
			}
			if (DeserializeRaw(input, ret.getData(), len)) {
				_out = Move(ret);
				return sl_true;
			}
			return sl_false;
		} else {
			_out.setNull();
			return sl_true;
		}
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const String16& _in)
	{
		return Serialize(output, String::from(_in));
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, String16& _out)
	{
		String s;
		if (Deserialize(input, s)) {
			if (s.isNotNull()) {
				_out = String16::from(s);
				return _out.isNotNull();
			} else {
				_out.setNull();
				return sl_true;
			}
		} else {
			return sl_false;
		}
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const StringView& _in)
	{
		return SerializeString(output, _in.getData(), _in.getLength());
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const StringView16& _in)
	{
		return Serialize(output, String::from(_in));
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const sl_char8* _in)
	{
		return Serialize(output, StringView(_in));
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const sl_char16* _in)
	{
		return Serialize(output, String::from(_in));
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const StringParam& _in)
	{
		return Serialize(output, _in.toString());
	}

#ifdef SLIB_SUPPORT_STD_TYPES
	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const std::string& _in)
	{
		return Serialize(output, String::from(_in));
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, std::string& _out)
	{
		String s;
		if (Deserialize(input, s)) {
			if (s.isNotNull()) {
				_out = s.toStd();
				return _out.size() != 0;
			} else {
				_out.clear();
				return sl_true;
			}
		} else {
			return sl_false;
		}
	}
	
	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const std::u16string& _in)
	{
		return Serialize(output, String::from(_in));
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, std::u16string& _out)
	{
		String16 s;
		if (Deserialize(input, s)) {
			if (s.isNotNull()) {
				_out = s.toStd();
				return _out.size() != 0;
			} else {
				_out.clear();
				return sl_true;
			}
		} else {
			return sl_false;
		}
	}
#endif

}

#endif
