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

#ifndef CHECKHEADER_SLIB_CORE_JSON_CONV
#define CHECKHEADER_SLIB_CORE_JSON_CONV

#include "core.h"

namespace slib
{

	void FromJson(const Json& json, Json& _out);
	void ToJson(Json& json, const Json& _in);

	void FromJson(const Json& json, Variant& _out);
	void ToJson(Json& json, const Variant& _in);

	void FromJson(const Json& json, signed char& _out);
	void FromJson(const Json& json, signed char& _out, signed char def);
	void ToJson(Json& json, signed char _in);

	void FromJson(const Json& json, unsigned char& _out);
	void FromJson(const Json& json, unsigned char& _out, unsigned char def);
	void ToJson(Json& json, unsigned char _in);

	void FromJson(const Json& json, short& _out);
	void FromJson(const Json& json, short& _out, short def);
	void ToJson(Json& json, short _in);

	void FromJson(const Json& json, unsigned short& _out);
	void FromJson(const Json& json, unsigned short& _out, unsigned short def);
	void ToJson(Json& json, unsigned short _in);

	void FromJson(const Json& json, int& _out);
	void FromJson(const Json& json, int& _out, int def);
	void ToJson(Json& json, int _in);

	void FromJson(const Json& json, unsigned int& _out);
	void FromJson(const Json& json, unsigned int& _out, unsigned int def);
	void ToJson(Json& json, unsigned int _in);

	void FromJson(const Json& json, long& _out);
	void FromJson(const Json& json, long& _out, long def);
	void ToJson(Json& json, long _in);

	void FromJson(const Json& json, unsigned long& _out);
	void FromJson(const Json& json, unsigned long& _out, unsigned long def);
	void ToJson(Json& json, unsigned long _in);

	void FromJson(const Json& json, sl_int64& _out);
	void FromJson(const Json& json, sl_int64& _out, sl_int64 def);
	void ToJson(Json& json, sl_int64 _in);

	void FromJson(const Json& json, sl_uint64& _out);
	void FromJson(const Json& json, sl_uint64& _out, sl_uint64 def);
	void ToJson(Json& json, sl_uint64 _in);

	void FromJson(const Json& json, float& _out);
	void FromJson(const Json& json, float& _out, float def);
	void ToJson(Json& json, float _in);

	void FromJson(const Json& json, double& _out);
	void FromJson(const Json& json, double& _out, double def);
	void ToJson(Json& json, double _in);

	void FromJson(const Json& json, bool& _out);
	void FromJson(const Json& json, bool& _out, bool def);
	void ToJson(Json& json, bool _in);

	void FromJson(const Json& json, String& _out);
	void FromJson(const Json& json, String& _out, const String& def);
	void ToJson(Json& json, const String& _in);
	void ToJson(Json& json, const StringView& _in);

	void FromJson(const Json& json, String16& _out);
	void FromJson(const Json& json, String16& _out, const String16& def);
	void ToJson(Json& json, const String16& _in);
	void ToJson(Json& json, const StringView16& _in);

	void ToJson(Json& json, const sl_char8* sz8);
	void ToJson(Json& json, const sl_char16* sz16);

	void FromJson(const Json& json, StringParam& _out);
	void ToJson(Json& json, const StringParam& _in);

#ifdef SLIB_SUPPORT_STD_TYPES
	void FromJson(const Json& json, std::string& _out);
	void ToJson(Json& json, const std::string& _in);

	void FromJson(const Json& json, std::u16string& _out);
	void ToJson(Json& json, const std::u16string& _in);
#endif

	void FromJson(const Json& json, Time& _out);
	void FromJson(const Json& json, Time& _out, const Time& def);
	void ToJson(Json& json, const Time& _in);

	void FromJson(const Json& json, Memory& _out);
	void ToJson(Json& json, const Memory& _in);

	void FromJson(const Json& json, VariantList& _out);
	void ToJson(Json& json, const VariantList& _in);

	void FromJson(const Json& json, VariantMap& _out);
	void ToJson(Json& json, const VariantMap& _in);

	void ToJson(Json& json, const List<VariantMap>& _in);

	void FromJson(const Json& json, JsonList& _out);
	void ToJson(Json& json, const JsonList& _in);

	void FromJson(const Json& json, JsonMap& _out);
	void ToJson(Json& json, const JsonMap& _in);

	void ToJson(Json& json, const List<JsonMap>& _in);

}

#endif
