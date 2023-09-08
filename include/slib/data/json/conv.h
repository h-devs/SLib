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

#ifndef CHECKHEADER_SLIB_DATA_JSON_CONV
#define CHECKHEADER_SLIB_DATA_JSON_CONV

#include "core.h"

namespace slib
{

	void FromJson(const Json& json, Json& _out);

	void FromJson(const Json& json, Variant& _out);

	void FromJson(const Json& json, signed char& _out);
	void FromJson(const Json& json, signed char& _out, signed char def);

	void FromJson(const Json& json, unsigned char& _out);
	void FromJson(const Json& json, unsigned char& _out, unsigned char def);

	void FromJson(const Json& json, char& _out);
	void FromJson(const Json& json, char& _out, char def);

	void FromJson(const Json& json, short& _out);
	void FromJson(const Json& json, short& _out, short def);

	void FromJson(const Json& json, unsigned short& _out);
	void FromJson(const Json& json, unsigned short& _out, unsigned short def);

	void FromJson(const Json& json, int& _out);
	void FromJson(const Json& json, int& _out, int def);

	void FromJson(const Json& json, unsigned int& _out);
	void FromJson(const Json& json, unsigned int& _out, unsigned int def);

	void FromJson(const Json& json, long& _out);
	void FromJson(const Json& json, long& _out, long def);

	void FromJson(const Json& json, unsigned long& _out);
	void FromJson(const Json& json, unsigned long& _out, unsigned long def);

	void FromJson(const Json& json, sl_int64& _out);
	void FromJson(const Json& json, sl_int64& _out, sl_int64 def);

	void FromJson(const Json& json, sl_uint64& _out);
	void FromJson(const Json& json, sl_uint64& _out, sl_uint64 def);

	void FromJson(const Json& json, sl_char16& _out);
	void FromJson(const Json& json, sl_char16& _out, sl_char16 def);

	void FromJson(const Json& json, sl_char32& _out);
	void FromJson(const Json& json, sl_char32& _out, sl_char32 def);

	void FromJson(const Json& json, float& _out);
	void FromJson(const Json& json, float& _out, float def);

	void FromJson(const Json& json, double& _out);
	void FromJson(const Json& json, double& _out, double def);

	void FromJson(const Json& json, bool& _out);
	void FromJson(const Json& json, bool& _out, bool def);

	void FromJson(const Json& json, String& _out);
	void FromJson(const Json& json, String& _out, const String& def);

	void FromJson(const Json& json, String16& _out);
	void FromJson(const Json& json, String16& _out, const String16& def);

	void FromJson(const Json& json, StringParam& _out);

#ifdef SLIB_SUPPORT_STD_TYPES
	void FromJson(const Json& json, std::string& _out);
	void FromJson(const Json& json, std::u16string& _out);
#endif

	void FromJson(const Json& json, Time& _out);
	void FromJson(const Json& json, Time& _out, const Time& def);

	void FromJson(const Json& json, Memory& _out);

	void FromJson(const Json& json, VariantList& _out);

	void FromJson(const Json& json, VariantMap& _out);

	void FromJson(const Json& json, JsonList& _out);

	void FromJson(const Json& json, JsonMap& _out);

}

#endif
