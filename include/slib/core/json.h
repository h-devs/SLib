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

#ifndef CHECKHEADER_SLIB_CORE_JSON
#define CHECKHEADER_SLIB_CORE_JSON

#include "variant.h"
#include "macro_arg.h"
#include "array_collection.h"
#include "list_collection.h"
#include "map_object.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <initializer_list>
#include <string>
#endif

namespace slib
{

	typedef Pair<String, Json> JsonItem;
	
	class SLIB_EXPORT JsonParseParam
	{
	public:
		// in
		sl_bool flagSupportComments;
		// in
		sl_bool flagLogError;

		// out
		sl_bool flagError;
		// out
		sl_size errorPosition;
		// out
		sl_size errorLine;
		// out
		sl_size errorColumn;
		// out
		String errorMessage;

	public:
		JsonParseParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JsonParseParam)

	public:
		String getErrorText();

	};
	
	class SLIB_EXPORT Json : public Variant
	{
	public:
		Json();
		
		Json(const Json& other);
		
		Json(Json&& other);

		Json(const Atomic<Json>& other);

		Json(const Variant& other);

		Json(Variant&& other);

		Json(const Atomic<Variant>& other);
		
		~Json();
		
	public:
		Json(sl_null_t);
		
		Json(signed char value);
		
		Json(unsigned char value);
		
		Json(short value);
		
		Json(unsigned short value);
		
		Json(int value);
		
		Json(unsigned int value);
		
		Json(long value);
		
		Json(unsigned long value);
		
		Json(sl_int64 value);
		
		Json(sl_uint64 value);
		
		Json(float value);
		
		Json(double value);
		
		Json(sl_bool value);
		
		Json(const String& value);

		Json(String&& value);

		Json(const String16& value);

		Json(String16&& value);

		Json(const sl_char8* sz8);
		
		Json(const sl_char16* sz16);
		
		Json(const StringParam& param);
		
#ifdef SLIB_SUPPORT_STD_TYPES
		Json(const std::string& value);

		Json(const std::u16string& value);
#endif
		
		Json(const Time& value);

		Json(const Memory& value);

		Json(Memory&& mem);

		Json(const JsonList& list);

		Json(JsonList&& list);

		Json(const JsonMap& map);

		Json(JsonMap&& map);

		Json(const VariantList& list);

		Json(VariantList&& list);

		Json(const VariantMap& map);

		Json(VariantMap&& map);

#ifdef SLIB_SUPPORT_STD_TYPES
		Json(const std::initializer_list<JsonItem>& pairs);
		
		Json(const std::initializer_list<Json>& elements);
#endif

		template <class T>
		Json(const Atomic<T>& t): Json(T(t)) {}

		template <class T>
		Json(const T& value)
		{
			ToJson(*this, value);
		}

	public:
		static const Json& undefined()
		{
			return *(reinterpret_cast<Json const*>(&(priv::variant::g_undefined)));
		}

		static const Json& null()
		{
			return *(reinterpret_cast<Json const*>(&(priv::variant::g_null)));
		}
		
		static Json createList();
		
		static Json createMap();
		
	public:
		Json& operator=(const Json& json);
		
		Json& operator=(Json&& json);

		Json& operator=(const Atomic<Json>& json);

		Json& operator=(const Variant& variant);

		Json& operator=(Variant&& variant);

		Json& operator=(const Atomic<Variant>& variant);
		
		Json& operator=(sl_null_t);
		
#ifdef SLIB_SUPPORT_STD_TYPES
		Json& operator=(const std::initializer_list<JsonItem>& pairs);
		
		Json& operator=(const std::initializer_list<Json>& elements);
#endif

		template <class T>
		Json& operator=(T&& value) noexcept
		{
			set(Forward<T>(value));
			return *this;
		}

		Json operator[](sl_size index) const;

		Json operator[](const StringParam& key) const;

	public:
		template <class T>
		void get(T& value) const
		{
			FromJson(*this, value);
		}

		template <class T>
		void get(T& value, const T& defaultValue) const
		{
			FromJson(*this, value, defaultValue);
		}

		template <class T>
		void set(T&& t)
		{
			_free(_type, _value);
			new (this) Json(Forward<T>(t));
		}

		Json getElement_NoLock(sl_size index) const;

		template <class T>
		void getElement_NoLock(sl_size index, T& _out) const
		{
			FromJson(getElement_NoLock(index), _out);
		}

		Json getElement(sl_size index) const;

		template <class T>
		void getElement(sl_size index, T& _out) const
		{
			FromJson(getElement(index), _out);
		}

		sl_bool setElement_NoLock(sl_uint64 index, const Json& value);

		sl_bool setElement(sl_uint64 index, const Json& value);

		sl_bool addElement_NoLock(const Json& value);

		sl_bool addElement(const Json& value);

		Json getItem_NoLock(const StringParam& key) const;

		template <class T>
		void getItem_NoLock(const StringParam& key, T& _out) const
		{
			FromJson(getItem_NoLock(key), _out);
		}

		Json getItem(const StringParam& key) const;
		
		template <class T>
		void getItem(const StringParam& key, T& _out) const
		{
			FromJson(getItem(key), _out);
		}

		sl_bool putItem_NoLock(const StringParam& key, const Json& value);

		sl_bool putItem(const StringParam& key, const Json& value);

	public:
		static Json parseJson(const sl_char8* sz, sl_size len, JsonParseParam& param);

		static Json parseJson(const sl_char8* sz, sl_size len);

		static Json parseJson(const sl_char16* sz, sl_size len, JsonParseParam& param);

		static Json parseJson(const sl_char16* sz, sl_size len);

		static Json parseJson(const StringParam& str, JsonParseParam& param);

		static Json parseJson(const StringParam& str);

		static Json parseJsonFromTextFile(const StringParam& filePath, JsonParseParam& param);

		static Json parseJsonFromTextFile(const StringParam& filePath);

	protected:
		String toString() const;
		
	};
	

	SLIB_INLINE JsonItem operator<<=(const String& str, const Json& v)
	{
		return JsonItem(str, v);
	}

	SLIB_INLINE JsonItem operator<<=(const String& str, Json&& v)
	{
		return JsonItem(str, Move(v));
	}

	SLIB_INLINE JsonItem operator>>=(const String& str, const Json& v)
	{
		return JsonItem(str, v);
	}

	SLIB_INLINE JsonItem operator>>=(const String& str, Json&& v)
	{
		return JsonItem(str, Move(v));
	}


	namespace priv
	{
		namespace json
		{
			class SLIB_EXPORT JsonFieldContainer : public StringContainer
			{
			public:
				JsonFieldContainer(sl_char8* _sz, sl_size _len)
				{
					sz = _sz;
					len = _len;
					hash = 0;
					type = 0;
					ref = -1;
				}
			};

		}
	}

}

#include "json_conv.h"

#define SLIB_JSON \
public: \
	slib::Json toJson() const \
	{ \
		slib::Json json = slib::Json::createMap(); \
		slib::RemoveConstPointerVariable(this)->doJson(json, sl_false); \
		return json; \
	} \
	void fromJson(const slib::Json& json) \
	{ \
		if (json.isUndefined()) { \
			return; \
		} \
		doJson(*((slib::Json*)&json), sl_true); \
	} \
	void doJson(slib::Json& json, sl_bool isFromJson)

#define SLIB_JSON_ADD_MEMBER(MEMBER_NAME, JSON_NAME) \
	{ \
		static sl_char8 _strJsonField_buf[] = JSON_NAME; \
		static slib::priv::json::JsonFieldContainer _strJsonField_container(_strJsonField_buf, sizeof(_strJsonField_buf)-1); \
		static slib::StringContainer* _strJsonField_str = &_strJsonField_container; \
		static const slib::String& _strJsonField = *(reinterpret_cast<slib::String*>(&_strJsonField_str)); \
		if (isFromJson) { \
			slib::FromJson(json.getItem(_strJsonField), MEMBER_NAME); \
		} else { \
			json.putItem(_strJsonField, MEMBER_NAME); \
		} \
	}

#define SLIB_JSON_ADD_MEMBER_FROM(MEMBER_NAME, JSON_NAME) \
	{ \
		if (isFromJson) { \
			static sl_char8 _strJsonField_buf[] = JSON_NAME; \
			static slib::priv::json::JsonFieldContainer _strJsonField_container(_strJsonField_buf, sizeof(_strJsonField_buf)-1); \
			static slib::StringContainer* _strJsonField_str = &_strJsonField_container; \
			static const slib::String& _strJsonField = *(reinterpret_cast<slib::String*>(&_strJsonField_str)); \
			slib::FromJson(json.getItem(_strJsonField), MEMBER_NAME); \
		} \
	}

#define SLIB_JSON_ADD_MEMBER_TO(MEMBER_NAME, JSON_NAME) \
	{ \
		if (!isFromJson) { \
			static sl_char8 _strJsonField_buf[] = JSON_NAME; \
			static slib::priv::json::JsonFieldContainer _strJsonField_container(_strJsonField_buf, sizeof(_strJsonField_buf)-1); \
			static slib::StringContainer* _strJsonField_str = &_strJsonField_container; \
			static const slib::String& _strJsonField = *(reinterpret_cast<slib::String*>(&_strJsonField_str)); \
			json.putItem(_strJsonField, MEMBER_NAME); \
		} \
	}

#define PRIV_SLIB_JSON_ADD_MEMBERS0
#define PRIV_SLIB_JSON_ADD_MEMBERS1(NAME) SLIB_JSON_ADD_MEMBER(NAME, #NAME)
#define PRIV_SLIB_JSON_ADD_MEMBERS2(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS1(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS3(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS2(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS4(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS3(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS5(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS4(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS6(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS5(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS7(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS6(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS8(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS7(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS9(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS8(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS10(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS9(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS11(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS10(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS12(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS11(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS13(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS12(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS14(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS13(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS15(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS14(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS16(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS15(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS17(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS16(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS18(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS17(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS19(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS18(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS20(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS19(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS21(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS20(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS22(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS21(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS23(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS22(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS24(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS23(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS25(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS24(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS26(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS25(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS27(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS26(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS28(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS27(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS29(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS28(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS30(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS29(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS31(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS30(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS32(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS31(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS33(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS32(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS34(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS33(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS35(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS34(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS36(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS35(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS37(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS36(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS38(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS37(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS39(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS38(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS40(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS39(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS41(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS40(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS42(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS41(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS43(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS42(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS44(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS43(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS45(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS44(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS46(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS45(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS47(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS46(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS48(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS47(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS49(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS48(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS50(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS49(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS51(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS50(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS52(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS51(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS53(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS52(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS54(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS53(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS55(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS54(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS56(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS55(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS57(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS56(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS58(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS57(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS59(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS58(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS60(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS59(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS61(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS60(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS62(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS61(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS63(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS62(__VA_ARGS__),)
#define PRIV_SLIB_JSON_ADD_MEMBERS64(NAME, ...) SLIB_JSON_ADD_MEMBER(NAME, #NAME) SLIB_MACRO_CONCAT(PRIV_SLIB_JSON_ADD_MEMBERS63(__VA_ARGS__),)

#define SLIB_JSON_ADD_MEMBERS(...) SLIB_MACRO_CONCAT(SLIB_MACRO_CONCAT_COUNT_ARGUMENTS(PRIV_SLIB_JSON_ADD_MEMBERS, __VA_ARGS__)(__VA_ARGS__),)

#define SLIB_JSON_MEMBERS(...) \
	SLIB_JSON \
	{ \
		SLIB_JSON_ADD_MEMBERS(__VA_ARGS__) \
	}


#define SLIB_DECLARE_JSON \
public: \
	slib::Json toJson() const; \
	void fromJson(const slib::Json& json); \
	void doJson(slib::Json& json, sl_bool isFromJson);

#define SLIB_DEFINE_JSON(CLASS) \
	slib::Json CLASS::toJson() const \
	{ \
		slib::Json json = slib::Json::createMap(); \
		slib::RemoveConstPointerVariable(this)->doJson(json, sl_false); \
		return json; \
	} \
	void CLASS::fromJson(const slib::Json& json) \
	{ \
		if (json.isUndefined()) { \
			return; \
		} \
		doJson(*((slib::Json*)&json), sl_true); \
	} \
	void CLASS::doJson(slib::Json& json, sl_bool isFromJson)

#define SLIB_DEFINE_JSON_MEMBERS(CLASS, ...) \
	SLIB_DEFINE_JSON(CLASS) \
	{ \
		SLIB_JSON_ADD_MEMBERS(__VA_ARGS__) \
	}
	

#endif
