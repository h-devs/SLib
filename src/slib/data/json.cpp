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

#define SLIB_SUPPORT_STD_TYPES

#include "slib/data/json.h"

#include "slib/io/file.h"
#include "slib/core/stringx.h"
#include "slib/core/log.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(JsonParseParam)

	JsonParseParam::JsonParseParam()
	{
		flagLogError = sl_false;
		flagSupportComments = sl_true;

		flagError = sl_false;
		errorLine = 0;
		errorColumn = 0;
		errorPosition = 0;
	}

	String JsonParseParam::getErrorText()
	{
		if (flagError) {
			return "(" + String::fromSize(errorLine) + ":" + String::fromSize(errorColumn) + ") " + errorMessage;
		}
		return sl_null;
	}


	Json::Json()
	{
	}

	Json::Json(const Json& other): Variant(other)
	{
	}

	Json::Json(Json&& other): Variant(Move(other))
	{
	}

	Json::Json(const Atomic<Json>& other)
	{
		other._retain_construct(this);
	}

	Json::Json(const Variant& other): Variant(other)
	{
	}

	Json::Json(Variant&& other): Variant(Move(other))
	{
	}

	Json::Json(const Atomic<Variant>& other): Variant(other)
	{
	}

	Json::~Json()
	{
	}

	Json::Json(sl_null_t): Variant(sl_null)
	{
	}

	Json::Json(signed char value): Variant(value)
	{
	}

	Json::Json(unsigned char value): Variant(value)
	{
	}

	Json::Json(short value): Variant(value)
	{
	}

	Json::Json(unsigned short value): Variant(value)
	{
	}

	Json::Json(int value): Variant(value)
	{
	}

	Json::Json(unsigned int value): Variant(value)
	{
	}

	Json::Json(long value): Variant(value)
	{
	}

	Json::Json(unsigned long value): Variant(value)
	{
	}

	Json::Json(sl_int64 value): Variant(value)
	{
	}

	Json::Json(sl_uint64 value): Variant(value)
	{
	}

	Json::Json(float value): Variant(value)
	{
	}

	Json::Json(double value): Variant(value)
	{
	}

	Json::Json(sl_bool value): Variant(value)
	{
	}

	Json::Json(const String& value): Variant(value)
	{
	}

	Json::Json(String&& value): Variant(Move(value))
	{
	}

	Json::Json(const String16& value): Variant(value)
	{
	}

	Json::Json(String16&& value): Variant(Move(value))
	{
	}

	Json::Json(const String32& value): Variant(value)
	{
	}

	Json::Json(String32&& value): Variant(Move(value))
	{
	}

	Json::Json(const StringView& value): Variant(value)
	{
	}

	Json::Json(const StringView16& value): Variant(value)
	{
	}

	Json::Json(const StringView32& value): Variant(value)
	{
	}

	Json::Json(const sl_char8* sz8): Variant(sz8)
	{
	}

	Json::Json(const sl_char16* sz16): Variant(sz16)
	{
	}

	Json::Json(const sl_char32* sz32): Variant(sz32)
	{
	}

	Json::Json(const StringParam& param): Variant(param)
	{
	}

	Json::Json(const std::string& str): Variant(str)
	{
	}

	Json::Json(const std::u16string& str): Variant(str)
	{
	}

	Json::Json(const std::u32string& str) : Variant(str)
	{
	}

	Json::Json(const Time& value): Variant(value)
	{
	}

	Json::Json(const Memory& value): Variant(value)
	{
	}

	Json::Json(Memory&& value): Variant(Move(value))
	{
	}

	Json::Json(const ObjectId& value): Variant(value)
	{
	}

	Json::Json(const JsonList& list): Variant(list)
	{
	}

	Json::Json(JsonList&& list): Variant(Move(list))
	{
	}

	Json::Json(const JsonMap& map): Variant(map)
	{
	}

	Json::Json(JsonMap&& map): Variant(Move(map))
	{
	}

	Json::Json(const VariantList& list): Variant(list)
	{
	}

	Json::Json(VariantList&& list): Variant(Move(list))
	{
	}

	Json::Json(const VariantMap& map): Variant(map)
	{
	}

	Json::Json(VariantMap&& map): Variant(Move(map))
	{
	}

	Json::Json(const std::initializer_list<JsonItem>& pairs): Variant(JsonMap(*(reinterpret_cast<const std::initializer_list< Pair<String, Json> >*>(&pairs))))
	{
	}

	Json Json::createList()
	{
		return JsonList::create();
	}

	Json Json::createMap()
	{
		return JsonMap::create();
	}

	Json& Json::operator=(const Json& json)
	{
		_assign(json);
		return *this;
	}

	Json& Json::operator=(Json&& json)
	{
		_assignMove(json);
		return *this;
	}

	Json& Json::operator=(const Atomic<Json>& json)
	{
		return *this = Json(json);
	}

	Json& Json::operator=(const Variant& variant)
	{
		_assign(variant);
		return *this;
	}

	Json& Json::operator=(Variant&& variant)
	{
		_assignMove(variant);
		return *this;
	}

	Json& Json::operator=(const Atomic<Variant>& variant)
	{
		return *this = Variant(variant);
	}

	Json& Json::operator=(sl_null_t)
	{
		setNull();
		return *this;
	}

	Json& Json::operator=(const std::initializer_list<JsonItem>& pairs)
	{
		return *this = Json(pairs);
	}

	Json Json::operator[](sl_size list_index) const
	{
		return getElement(list_index);
	}

	Json Json::operator[](const String& key) const
	{
		return getItem(key);
	}

	Json Json::getElement(sl_size index) const
	{
		return Variant::getElement(index);
	}

	sl_bool Json::setElement(sl_uint64 index, const Json& value)
	{
		return Variant::setElement(index, value);
	}

	sl_bool Json::addElement(const Json& value)
	{
		return Variant::addElement(value);
	}

	Json Json::getItem(const String& key) const
	{
		return Variant::getItem(key);
	}

	sl_bool Json::putItem(const String& key, const Json& value)
	{
		return Variant::putItem(key, value);
	}

	String Json::toString() const
	{
		return Variant::toString();
	}

	namespace {

		// https://github.com/mongodb/specifications/blob/master/source/extended-json.rst
		static Variant ParseExtendedJson(const JsonMap& map)
		{
			ObjectId oid;
			if (oid.setJson(map)) {
				return oid;
			}
			SLIB_STATIC_STRING(strNumberInt, "$numberInt")
			sl_int32 n32;
			if (map.getValue(strNumberInt).getInt32(&n32)) {
				return n32;
			}
			SLIB_STATIC_STRING(strNumberLong, "$numberLong")
			sl_int64 n64;
			if (map.getValue(strNumberLong).getInt64(&n64)) {
				return n64;
			}
			SLIB_STATIC_STRING(strNumberDouble, "$numberDouble")
			double nDouble;
			if (map.getValue(strNumberDouble).getDouble(&nDouble)) {
				return nDouble;
			}
			SLIB_STATIC_STRING(strNumberDate, "$date")
			Time nTime;
			Variant nTimeValue = map.getValue(strNumberDate);
			if (nTimeValue.getTime(&nTime)) {
				return nTime;
			}
			nTimeValue = nTimeValue.getItem(strNumberLong);
			if (nTimeValue.isIntegerType()) {
				return Time::withMilliseconds(nTimeValue.getInt64());
			}
			SLIB_STATIC_STRING(strUndefined, "$undefined")
			if (map.getValue(strUndefined).isTrue()) {
				return Variant();
			}
			sl_uint32 subType = 0;
			Memory mem = Memory::createFromExtendedJson(map, &subType);
			if (mem.isNotNull()) {
				if (subType) {
					return map;
				} else {
					return mem;
				}
			}
			return map;
		}

		template <class CHAR>
		class Parser
		{
		public:
			typedef typename StringTypeFromCharType<CHAR>::Type StringType;
			typedef typename StringViewTypeFromCharType<CHAR>::Type StringViewType;
			const CHAR* buf = sl_null;
			sl_size len = 0;
			sl_bool flagSupportComments = sl_false;

			sl_size pos = 0;

			sl_bool flagError = sl_false;
			String errorMessage;

			StringType strUndefined;
			StringType strNull;
			StringType strTrue;
			StringType strFalse;

		public:
			Parser();

		public:
			void escapeSpaceAndComments();

			Json parse();

			static Json parse(const CHAR* buf, sl_size len, JsonParseParam& param);

		};

		template <>
		Parser<sl_char8>::Parser()
		{
			SLIB_STATIC_STRING(_undefined, "undefined");
			strUndefined = _undefined;
			SLIB_STATIC_STRING(_null, "null");
			strNull = _null;
			SLIB_STATIC_STRING(_true, "true");
			strTrue = _true;
			SLIB_STATIC_STRING(_false, "false");
			strFalse = _false;
		}

		template <>
		Parser<sl_char16>::Parser()
		{
			SLIB_STATIC_STRING16(_undefined, "undefined");
			strUndefined = _undefined;
			SLIB_STATIC_STRING16(_null, "null");
			strNull = _null;
			SLIB_STATIC_STRING16(_true, "true");
			strTrue = _true;
			SLIB_STATIC_STRING16(_false, "false");
			strFalse = _false;
		}

		template <>
		Parser<sl_char32>::Parser()
		{
			SLIB_STATIC_STRING32(_undefined, "undefined");
			strUndefined = _undefined;
			SLIB_STATIC_STRING32(_null, "null");
			strNull = _null;
			SLIB_STATIC_STRING32(_true, "true");
			strTrue = _true;
			SLIB_STATIC_STRING32(_false, "false");
			strFalse = _false;
		}

		template <class CHAR>
		void Parser<CHAR>::escapeSpaceAndComments()
		{
			sl_bool flagLineComment = sl_false;
			sl_bool flagBlockComment = sl_false;
			while (pos < len) {
				sl_bool flagEscape = sl_false;
				CHAR ch = buf[pos];
				if (flagSupportComments) {
					if (flagLineComment) {
						flagEscape = sl_true;
						if (ch == '\r' || ch == '\n') {
							flagLineComment = sl_false;
						}
					} else {
						if (flagBlockComment) {
							flagEscape = sl_true;
							if (pos >= 2 && ch == '/' && buf[pos-1] == '*') {
								flagBlockComment = sl_false;
							}
						} else {
							if (pos + 2 <= len && ch == '/') {
								if (buf[pos + 1] == '/') {
									flagLineComment = sl_true;
									flagEscape = sl_true;
									pos++;
								} else if (buf[pos + 1] == '*') {
									flagBlockComment = sl_true;
									flagEscape = sl_true;
									pos++;
								}
							}
						}
					}
				}
				if (!flagEscape && !SLIB_CHAR_IS_WHITE_SPACE(ch)) {
					break;
				}
				pos++;
			}
		}

		template <class CHAR>
		Json Parser<CHAR>::parse()
		{
			escapeSpaceAndComments();
			if (pos == len) {
				return Json();
			}

			CHAR first = buf[pos];

			// string
			if (first == '"' || first == '\'') {
				sl_size m = 0;
				sl_bool f = sl_false;
				StringType str = StringType::from(Stringx::parseBackslashEscapes(StringViewType(buf + pos, len - pos), &m, &f));
				pos += m;
				if (f) {
					flagError = sl_true;
					errorMessage = "String: Missing character  \" or ' ";
					return Json();
				}
				return str;
			}

			// array
			if (first == '[') {
				pos++;
				escapeSpaceAndComments();
				if (pos == len) {
					flagError = sl_true;
					errorMessage = "Array: Missing character ] ";
					return Json();
				}
				if (buf[pos] == ']') {
					pos++;
					return Json::createList();
				}
				JsonList list = JsonList::create();
				while (pos < len) {
					CHAR ch = buf[pos];
					if (ch == ']' || ch == ',') {
						list.add_NoLock(Json::null());
					} else {
						Json item = parse();
						if (flagError) {
							return Json();
						}
						list.add_NoLock(item);
						escapeSpaceAndComments();
						if (pos == len) {
							flagError = sl_true;
							errorMessage = "Array: Missing character ] ";
							return Json();
						}
						ch = buf[pos];
					}
					if (ch == ']') {
						pos++;
						return list;
					} else if (ch == ',') {
						pos++;
					} else {
						flagError = sl_true;
						errorMessage = "Array: Missing character ] ";
						return Json();
					}
					escapeSpaceAndComments();
					if (pos == len) {
						flagError = sl_true;
						errorMessage = "Array: Missing character ] ";
						return Json();
					}
				}
				flagError = sl_true;
				errorMessage = "Array: Missing character ] ";
				return Json();
			}

			// object
			if (first == '{') {
				pos++;
				if (pos == len) {
					flagError = sl_true;
					errorMessage = "Object: Missing character } ";
					return Json();
				}
				JsonMap map = JsonMap::create();
				sl_bool flagFirst = sl_true;
				sl_bool flagFoundExtendedJsonField = sl_false;
				while (pos < len) {
					escapeSpaceAndComments();
					if (pos == len) {
						flagError = sl_true;
						errorMessage = "Object: Missing character } ";
						return Json();
					}
					CHAR ch = buf[pos];
					if (ch == '}') {
						pos++;
						if (flagFoundExtendedJsonField) {
							return ParseExtendedJson(map);
						} else {
							return map;
						}
					}
					if (!flagFirst) {
						if (ch == ',') {
							pos++;
						} else {
							flagError = sl_true;
							errorMessage = "Object: Missing character , ";
							return Json();
						}
					}
					escapeSpaceAndComments();
					if (pos == len) {
						flagError = sl_true;
						errorMessage = "Object: Missing character } ";
						return Json();
					}
					StringType key;
					ch = buf[pos];
					if (ch == '}') {
						pos++;
						if (flagFoundExtendedJsonField) {
							return ParseExtendedJson(map);
						} else {
							return map;
						}
					} else if (ch == '"' || ch == '\'') {
						sl_size m = 0;
						sl_bool f = sl_false;
						key = StringType::from(Stringx::parseBackslashEscapes(StringViewType(buf + pos, len - pos), &m, &f));
						if (key.startsWith('$')) {
							flagFoundExtendedJsonField = sl_true;
						}
						pos += m;
						if (f) {
							flagError = sl_true;
							errorMessage = "Object Item Name: Missing terminating character \" or ' ";
							return Json();
						}
					} else {
						sl_size s = pos;
						while (pos < len) {
							CHAR ch = buf[pos];
							if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' || (pos != s && ch >= '0' && ch <= '9')) {
								pos++;
							} else {
								break;
							}
						}
						if (pos == len) {
							flagError = sl_true;
							errorMessage = "Object: Missing character : ";
							return Json();
						}
						key = StringType(buf + s, pos - s);
					}
					escapeSpaceAndComments();
					if (pos == len) {
						flagError = sl_true;
						errorMessage = "Object: Missing character : ";
						return Json();
					}
					if (buf[pos] == ':') {
						pos++;
					} else {
						flagError = sl_true;
						errorMessage = "Object: Missing character : ";
						return Json();
					}
					escapeSpaceAndComments();
					if (pos == len) {
						flagError = sl_true;
						errorMessage = "Object: Missing Item value";
						return Json();
					}
					if (buf[pos] == '}' || buf[pos] == ',') {
						map.put_NoLock(String::from(key), Json::null());
					} else {
						Json item = parse();
						if (flagError) {
							return Json();
						}
						if (item.isNotUndefined()) {
							map.put_NoLock(String::from(key), item);
						}
					}
					flagFirst = sl_false;
				}
				flagError = sl_true;
				errorMessage = "Object: Missing character } ";
				return Json();
			}
			{
				sl_size s = pos;
				while (pos < len) {
					CHAR ch = buf[pos];
					if (ch == '\r' || ch == '\n' || ch == ' ' || ch == '\t' || ch == '/' || ch == ']' || ch == '}' || ch == ',') {
						break;
					} else {
						pos++;
					}
				}
				if (pos == s) {
					flagError = sl_true;
					errorMessage = "Invalid token";
					return Json();
				}
				StringType str(buf + s, pos - s);
				if (str == strUndefined) {
					return Json();
				}
				if (str == strNull) {
					return sl_null;
				}
				if (str == strTrue) {
					return Json(sl_true);
				}
				if (str == strFalse) {
					return Json(sl_false);
				}
				sl_int64 vi64;
				if (str.parseInt64(0, &vi64)) {
					if (vi64 >= SLIB_INT64(-0x80000000) && vi64 < SLIB_INT64(0x7fffffff)) {
						return (sl_int32)vi64;
					} else {
						return vi64;
					}
				}
				double vf;
				if (str.parseDouble(&vf)) {
					return vf;
				}
			}
			flagError = sl_true;
			errorMessage = "Invalid token";
			return Json();
		}

		template <class CHAR>
		Json Parser<CHAR>::parse(const CHAR* buf, sl_size len, JsonParseParam& param)
		{
			if (!len) {
				return Json();
			}

			param.flagError = sl_false;

			Parser<CHAR> parser;
			parser.buf = buf;
			parser.len = len;
			parser.flagSupportComments = param.flagSupportComments;

			parser.pos = 0;
			parser.flagError = sl_false;

			Json var = parser.parse();
			if (!(parser.flagError)) {
				parser.escapeSpaceAndComments();
				if (parser.pos != len) {
					parser.flagError = sl_true;
					parser.errorMessage = "Invalid token";
				}
				if (!(parser.flagError)) {
					return var;
				}
			}

			param.flagError = sl_true;
			param.errorPosition = parser.pos;
			param.errorMessage = parser.errorMessage;
			param.errorLine = Stringx::countLineNumber(StringViewType(buf, parser.pos), &(param.errorColumn));

			if (param.flagLogError) {
				LogError("Json", param.getErrorText());
			}

			return Json();

		}

	}

	Json Json::parse(const sl_char8* str, sl_size len, JsonParseParam& param)
	{
		return Parser<sl_char8>::parse(str, len, param);
	}

	Json Json::parse(const sl_char8* str, sl_size len)
	{
		if (!len) {
			return Json();
		}
		JsonParseParam param;
		return parse(str, len, param);
	}

	Json Json::parse(const sl_char16* str, sl_size len, JsonParseParam& param)
	{
		return Parser<sl_char16>::parse(str, len, param);
	}

	Json Json::parse(const sl_char16* str, sl_size len)
	{
		if (!len) {
			return Json();
		}
		JsonParseParam param;
		return parse(str, len, param);
	}

	Json Json::parse(const sl_char32* str, sl_size len, JsonParseParam& param)
	{
		return Parser<sl_char32>::parse(str, len, param);
	}

	Json Json::parse(const sl_char32* str, sl_size len)
	{
		if (!len) {
			return Json();
		}
		JsonParseParam param;
		return parse(str, len, param);
	}

	Json Json::parse(const StringParam& _str, JsonParseParam& param)
	{
		if (_str.isEmpty()) {
			return Json();
		}
		if (_str.is8BitsStringType()) {
			StringData str(_str);
			return Parser<sl_char8>::parse(str.getData(), str.getLength(), param);
		} else if (_str.is16BitsStringType()) {
			StringData16 str(_str);
			return Parser<sl_char16>::parse(str.getData(), str.getLength(), param);
		} else {
			StringData32 str(_str);
			return Parser<sl_char32>::parse(str.getData(), str.getLength(), param);
		}
	}

	Json Json::parse(const StringParam& str)
	{
		if (str.isEmpty()) {
			return Json();
		}
		JsonParseParam param;
		return parse(str, param);
	}

	Json Json::parse(const MemoryView& utf, JsonParseParam& param)
	{
		if (!(utf.size)) {
			return Json();
		}
		return parse(StringParam::fromUtf(utf), param);
	}

	Json Json::parse(const MemoryView& utf)
	{
		if (!(utf.size)) {
			return Json();
		}
		JsonParseParam param;
		return parse(utf, param);
	}

	Json Json::parseTextFile(const StringParam& filePath, JsonParseParam& param)
	{
		return parse(File::readAllText(filePath), param);
	}

	Json Json::parseTextFile(const StringParam& filePath)
	{
		JsonParseParam param;
		return parseTextFile(filePath, param);
	}


	void FromJson(const Json& json, Json& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out = json;
	}

	void ToJson(Json& json, const Json& _in)
	{
		json = _in;
	}

	void FromJson(const Json& json, Variant& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out = json;
	}

	void ToJson(Json& json, const Variant& _in)
	{
		json = _in;
	}

	void FromJson(const Json& json, signed char& _out)
	{
		_out = (char)(json.getInt32((sl_int32)_out));
	}

	void FromJson(const Json& json, signed char& _out, signed char def)
	{
		_out = (char)(json.getInt32((sl_int32)def));
	}

	void ToJson(Json& json, signed char _in)
	{
		json.setInt32((sl_int32)_in);
	}

	void FromJson(const Json& json, unsigned char& _out)
	{
		_out = (unsigned char)(json.getUint32((sl_uint32)_out));
	}

	void FromJson(const Json& json, unsigned char& _out, unsigned char def)
	{
		_out = (unsigned char)(json.getUint32((sl_uint32)def));
	}

	void ToJson(Json& json, unsigned char _in)
	{
		json.setUint32((sl_uint32)_in);
	}

	void FromJson(const Json& json, short& _out)
	{
		_out = (short)(json.getInt32((sl_int32)_out));
	}

	void FromJson(const Json& json, short& _out, short def)
	{
		_out = (short)(json.getInt32((sl_int32)def));
	}

	void ToJson(Json& json, short _in)
	{
		json.setInt32((sl_int32)_in);
	}

	void FromJson(const Json& json, unsigned short& _out)
	{
		_out = (unsigned short)(json.getUint32(_out));
	}

	void FromJson(const Json& json, unsigned short& _out, unsigned short def)
	{
		_out = (unsigned short)(json.getUint32((sl_uint32)def));
	}

	void ToJson(Json& json, unsigned short _in)
	{
		json.setUint32((sl_uint32)_in);
	}

	void FromJson(const Json& json, int& _out)
	{
		_out = (int)(json.getInt32((sl_int32)_out));
	}

	void FromJson(const Json& json, int& _out, int def)
	{
		_out = (int)(json.getInt32((sl_int32)def));
	}

	void ToJson(Json& json, int _in)
	{
		json.setInt32((sl_int32)_in);
	}

	void FromJson(const Json& json, unsigned int& _out)
	{
		_out = (unsigned int)(json.getUint32((sl_uint32)_out));
	}

	void FromJson(const Json& json, unsigned int& _out, unsigned int def)
	{
		_out = (unsigned int)(json.getUint32((sl_uint32)def));
	}

	void ToJson(Json& json, unsigned int _in)
	{
		json.setUint32((sl_uint32)_in);
	}

	void FromJson(const Json& json, long& _out)
	{
		_out = (long)(json.getInt32((sl_int32)_out));
	}

	void FromJson(const Json& json, long& _out, long def)
	{
		_out = (long)(json.getInt32((sl_int32)def));
	}

	void ToJson(Json& json, long _in)
	{
		json.setInt32((sl_int32)_in);
	}

	void FromJson(const Json& json, unsigned long& _out)
	{
		_out = (unsigned long)(json.getUint32((sl_uint32)_out));
	}

	void FromJson(const Json& json, unsigned long& _out, unsigned long def)
	{
		_out = (unsigned long)(json.getUint32((sl_uint32)def));
	}

	void ToJson(Json& json, unsigned long _in)
	{
		json.setUint32((sl_uint32)_in);
	}

	void FromJson(const Json& json, sl_int64& _out)
	{
		_out = json.getInt64(_out);
	}

	void FromJson(const Json& json, sl_int64& _out, sl_int64 def)
	{
		_out = json.getInt64(def);
	}

	void ToJson(Json& json, sl_int64 _in)
	{
		json.setInt64(_in);
	}

	void FromJson(const Json& json, sl_uint64& _out)
	{
		_out = json.getUint64(_out);
	}

	void FromJson(const Json& json, sl_uint64& _out, sl_uint64 def)
	{
		_out = json.getUint64(def);
	}

	void ToJson(Json& json, sl_uint64 _in)
	{
		json.setUint64(_in);
	}

	void FromJson(const Json& json, float& _out)
	{
		_out = json.getFloat(_out);
	}

	void FromJson(const Json& json, float& _out, float def)
	{
		_out = json.getFloat(def);
	}

	void ToJson(Json& json, float _in)
	{
		json.setFloat(_in);
	}

	void FromJson(const Json& json, double& _out)
	{
		_out = json.getDouble(_out);
	}

	void FromJson(const Json& json, double& _out, double def)
	{
		_out = json.getDouble(def);
	}

	void ToJson(Json& json, double _in)
	{
		json.setDouble(_in);
	}

	void FromJson(const Json& json, bool& _out)
	{
		_out = json.getBoolean(_out);
	}

	void FromJson(const Json& json, bool& _out, bool def)
	{
		_out = json.getBoolean(def);
	}

	void ToJson(Json& json, bool _in)
	{
		json.setBoolean(_in);
	}

	void FromJson(const Json& json, String& _out)
	{
		_out = json.getString(_out);
	}

	void FromJson(const Json& json, String& _out, const String& def)
	{
		_out = json.getString(def);
	}

	void ToJson(Json& json, const String& _in)
	{
		json.setString(_in);
	}

	void ToJson(Json& json, const StringView& _in)
	{
		json.setString(_in);
	}

	void FromJson(const Json& json, String16& _out)
	{
		_out = json.getString16(_out);
	}

	void FromJson(const Json& json, String16& _out, const String16& def)
	{
		_out = json.getString16(def);
	}

	void ToJson(Json& json, const String16& _in)
	{
		json.setString(_in);
	}

	void ToJson(Json& json, const StringView16& _in)
	{
		json.setString(_in);
	}

	void ToJson(Json& json, const sl_char8* _in)
	{
		json.setString(_in);
	}

	void ToJson(Json& json, const sl_char16* _in)
	{
		json.setString(_in);
	}

	void FromJson(const Json& json, StringParam& _out)
	{
		_out = json.getStringParam(_out);
	}

	void ToJson(Json& json, const StringParam& _in)
	{
		json.setString(_in);
	}

	void FromJson(const Json& json, std::string& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out = json.getStdString();
	}

	void ToJson(Json& json, const std::string& _in)
	{
		json.setString(_in);
	}

	void FromJson(const Json& json, std::u16string& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out = json.getStdString16();
	}

	void ToJson(Json& json, const std::u16string& _in)
	{
		json.setString(_in);
	}

	void FromJson(const Json& json, Time& _out)
	{
		_out = json.getTime(_out);
	}

	void FromJson(const Json& json, Time& _out, const Time& def)
	{
		_out = json.getTime(def);
	}

	void ToJson(Json& json, const Time& _in)
	{
		json.setTime(_in);
	}

	void FromJson(const Json& json, Memory& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out = json.getMemory();
	}

	void ToJson(Json& json, const Memory& _in)
	{
		json.setMemory(_in);
	}

	void FromJson(const Json& json, VariantList& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out = json.getVariantList();
	}

	void ToJson(Json& json, const VariantList& _in)
	{
		json.setVariantList(_in);
	}

	void FromJson(const Json& json, VariantMap& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out = json.getVariantMap();
	}

	void ToJson(Json& json, const VariantMap& _in)
	{
		json.setVariantMap(_in);
	}

	void ToJson(Json& json, const List<VariantMap>& _in)
	{
		json.Variant::set(_in);
	}

	void FromJson(const Json& json, JsonList& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out = json.getJsonList();
	}

	void ToJson(Json& json, const JsonList& _in)
	{
		json.setJsonList(_in);
	}

	void FromJson(const Json& json, JsonMap& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out = json.getJsonMap();
	}

	void ToJson(Json& json, const JsonMap& _in)
	{
		json.setJsonMap(_in);
	}

	void ToJson(Json& json, const List<JsonMap>& _in)
	{
		json.Variant::set(_in);
	}


	Json ObjectId::toJson() const noexcept
	{
		JsonMap ret;
		SLIB_STATIC_STRING(oid, "$oid");
		if (ret.put_NoLock(oid, toString())) {
			return ret;
		}
		return Json();
	}

	sl_bool ObjectId::setJson(const Json& json) noexcept
	{
		return json.getObjectId(this);
	}

}
