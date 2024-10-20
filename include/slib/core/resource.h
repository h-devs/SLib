/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_RESOURCE
#define CHECKHEADER_SLIB_CORE_RESOURCE

#include "locale.h"
#include "hash_map.h"
#include "memory.h"
#include "string.h"
#include "safe_static.h"

namespace slib
{

	class Resources
	{
	public:
		static Locale getCurrentLocale();

		static void setCurrentLocale(const Locale& locale);

		static String makeResourceName(const String& path);
	};

	namespace priv
	{
		Memory CompressRawResource(const void* data, sl_size size);

		Memory DecompressRawResource(const void* data, sl_size size);
	}

}

#define SLIB_DECLARE_STRING_RESOURCE(NAME) \
	namespace NAME { \
		slib::String get(const slib::Locale& locale); \
		slib::String get(); \
	} \

#define PRIV_SLIB_DEFINE_STRING_RESOURCE_BEGIN_PREFIX \
			slib::Locale localeSource; \
			SLIB_UNUSED(localeSource) \
			slib::Locale localeLang(locale.getLanguage()); \
			SLIB_UNUSED(localeLang) \
			slib::Locale localeLangCountry(locale.getLanguage(), locale.getCountry()); \
			SLIB_UNUSED(localeLangCountry) \
			slib::Locale localeDetail(locale.getLanguage(), locale.getScript(), slib::Country::Unknown); \
			SLIB_UNUSED(localeDetail)

#define SLIB_DEFINE_STRING_RESOURCE_BEGIN(NAME, DEFAULT) \
	namespace NAME { \
		SLIB_STATIC_STRING(def, DEFAULT) \
		static slib::String _get(const slib::Locale& locale, const slib::String& _def) { \
			PRIV_SLIB_DEFINE_STRING_RESOURCE_BEGIN_PREFIX

#define SLIB_DEFINE_STRING_RESOURCE_VALUE(LOCALE, VALUE) \
			localeSource = slib::Locale(#LOCALE); \
			if (locale == localeSource || localeLang == localeSource || localeLangCountry == localeSource || localeDetail == localeSource) { \
				SLIB_RETURN_STRING(VALUE); \
			}

#define SLIB_DEFINE_STRING_RESOURCE_END \
			return _def; \
		} \
		slib::String get(const slib::Locale& locale) { \
			return _get(locale, def); \
		} \
		slib::String get() { return get(slib::Resources::getCurrentLocale()); } \
	} \

#define SLIB_DEFINE_STRING_RESOURCE_SIMPLE(NAME, VALUE) \
	namespace NAME { \
		SLIB_STATIC_STRING(def, VALUE) \
		slib::String _get(const slib::Locale& locale, const slib::String& _def) { return _def; } \
		slib::String get(const slib::Locale& locale) { return def; } \
		slib::String get() { return def; } \
	}

#define SLIB_DECLARE_STRING_VARIANT(NAME, VARIANT) \
	namespace NAME { \
		SLIB_DECLARE_STRING_RESOURCE(VARIANT) \
	}

#define SLIB_DEFINE_STRING_VARIANT_BEGIN(NAME, VARIANT, DEFAULT) \
	namespace NAME { \
		namespace VARIANT { \
			SLIB_STATIC_STRING(def, DEFAULT) \
			slib::String get(const slib::Locale& locale) { \
				PRIV_SLIB_DEFINE_STRING_RESOURCE_BEGIN_PREFIX

#define SLIB_DEFINE_STRING_VARIANT_BEGIN_NODEF(NAME, VARIANT) \
	namespace NAME { \
		namespace VARIANT { \
			slib::String get(const slib::Locale& locale) { \
				PRIV_SLIB_DEFINE_STRING_RESOURCE_BEGIN_PREFIX

#define SLIB_DEFINE_STRING_VARIANT_END \
				return _get(locale, def); \
			} \
			slib::String get() { return get(slib::Resources::getCurrentLocale()); } \
		} \
	} \

#define SLIB_DEFINE_STRING_VARIANT_SIMPLE(NAME, VARIANT, VALUE) \
	namespace NAME { \
		namespace VARIANT { \
			SLIB_STATIC_STRING(def, VALUE) \
			slib::String get(const slib::Locale& locale) { return _get(locale, def); } \
			slib::String get() { return get(slib::Resources::getCurrentLocale()); } \
		} \
	}


#define SLIB_DECLARE_RAW_RESOURCE(NAME) \
	namespace NAME { \
		extern const sl_uint8 bytes[]; \
		extern const sl_size size; \
		slib::Memory get(); \
	} \

#define SLIB_DEFINE_RAW_RESOURCE(NAME, SIZE) \
	namespace NAME { \
		const sl_size size = SIZE; \
		slib::Memory get() { \
			return slib::Memory::createStatic(bytes, SIZE); \
		} \
	}

#define SLIB_DECLARE_COMPRESSED_RAW_RESOURCE(NAME) \
	namespace NAME { \
		slib::Memory get(); \
	} \

#define SLIB_DEFINE_COMPRESSED_RAW_RESOURCE(NAME) \
	namespace NAME { \
		extern const sl_uint8 compressed_bytes[]; \
		extern const sl_size compressed_size; \
		struct Context { \
			slib::Memory memory; \
			Context() { \
				memory = slib::priv::DecompressRawResource(compressed_bytes, compressed_size); \
			} \
		}; \
		SLIB_SAFE_STATIC_GETTER(Context, GetContext); \
		slib::Memory get() { \
			Context* context = GetContext(); \
			if (context) { \
				return context->memory; \
			} \
			return sl_null; \
		} \
	}

#define SLIB_DECLARE_RESOURCE_MAP(TYPE) \
	TYPE get(const slib::String& name); \
	slib::List<slib::String> getAllNames();

#define SLIB_DEFINE_RESOURCE_MAP_BEGIN(TYPE) \
	typedef TYPE (*GETTER_TYPE)(); \
	class ResourceMap { \
	public: \
		slib::CHashMap< slib::String, GETTER_TYPE > map; \
	public: \
		 ResourceMap() {

#define SLIB_DEFINE_RESOURCE_MAP_ITEM(NAME) \
			{ \
				SLIB_STATIC_STRING(_key, #NAME) \
				GETTER_TYPE f = &(NAME::get); \
				map.put_NoLock(_key, f); \
			}

#define SLIB_DEFINE_RESOURCE_MAP_END(TYPE, DEFAULT_VALUE) \
		} \
	}; \
	SLIB_SAFE_STATIC_GETTER(ResourceMap, getResourceMap); \
	TYPE get(const slib::String& name) { \
		 ResourceMap* map = getResourceMap(); \
		if (map) { \
			GETTER_TYPE getter; \
			if (map->map.get_NoLock(name, &getter)) { \
				return getter(); \
			} \
		} \
		return DEFAULT_VALUE; \
	} \
	slib::List<slib::String> getAllNames() { \
		ResourceMap* map = getResourceMap(); \
		if (map) { \
			return map->map.getAllKeys_NoLock(); \
		} \
		return sl_null; \
	}

#define SLIB_DECLARE_LOCALIZED_RESOURCE_MAP(TYPE) \
	TYPE get(const slib::String& name, const slib::Locale& locale); \
	TYPE get(const slib::String& name); \
	slib::List<slib::String> getAllNames();

#define SLIB_DEFINE_LOCALIZED_RESOURCE_MAP_BEGIN(TYPE) \
	typedef TYPE (*GETTER_TYPE)(); \
	typedef TYPE (*GETTER_LOCALE_TYPE)(const slib::Locale& locale); \
	class  ResourceMap { \
	public: \
		slib::CHashMap< slib::String, GETTER_TYPE > map; \
		slib::CHashMap< slib::String, GETTER_LOCALE_TYPE > map_locale; \
	public: \
		ResourceMap() {

#define SLIB_DEFINE_LOCALIZED_RESOURCE_MAP_ITEM(NAME) \
			{ \
				SLIB_STATIC_STRING(_key, #NAME) \
				GETTER_TYPE f1 = &(NAME::get); \
				map.put_NoLock(_key, f1); \
				GETTER_LOCALE_TYPE f2 = &(NAME::get); \
				map_locale.put_NoLock(_key, f2); \
			}

#define SLIB_DEFINE_LOCALIZED_RESOURCE_MAP_END(TYPE, DEFAULT_VALUE) \
		} \
	}; \
	SLIB_SAFE_STATIC_GETTER(ResourceMap, getResourceMap); \
	TYPE get(const slib::String& name, const slib::Locale& locale) { \
		ResourceMap* map = getResourceMap(); \
		if (map) { \
			GETTER_LOCALE_TYPE getter; \
			if (map->map_locale.get_NoLock(name, &getter)) { \
				return getter(locale); \
			} \
		} \
		return DEFAULT_VALUE; \
	} \
	TYPE get(const slib::String& name) { \
		ResourceMap* map = getResourceMap(); \
		if (map) { \
			GETTER_TYPE getter; \
			if (map->map.get_NoLock(name, &getter)) { \
				return getter(); \
			} \
		} \
		return DEFAULT_VALUE; \
	} \
	slib::List<slib::String> getAllNames() { \
		ResourceMap* map = getResourceMap(); \
		if (map) { \
			return map->map.getAllKeys_NoLock(); \
		} \
		return sl_null; \
	}

#define SLIB_DECLARE_STRING_RESOURCE_MAP SLIB_DECLARE_LOCALIZED_RESOURCE_MAP(slib::String)
#define SLIB_DEFINE_STRING_RESOURCE_MAP_BEGIN SLIB_DEFINE_LOCALIZED_RESOURCE_MAP_BEGIN(slib::String)
#define SLIB_DEFINE_STRING_RESOURCE_MAP_ITEM(NAME) SLIB_DEFINE_LOCALIZED_RESOURCE_MAP_ITEM(NAME)
#define SLIB_DEFINE_STRING_RESOURCE_MAP_END SLIB_DEFINE_LOCALIZED_RESOURCE_MAP_END(slib::String, sl_null)
#define SLIB_DEFINE_STRING_VARIANT_MAP_ITEM(NAME, VARIANT) \
			{ \
				SLIB_STATIC_STRING(_key, #NAME "/" #VARIANT) \
				GETTER_TYPE f1 = &(NAME::VARIANT::get); \
				map.put_NoLock(_key, f1); \
				GETTER_LOCALE_TYPE f2 = &(NAME::VARIANT::get); \
				map_locale.put_NoLock(_key, f2); \
			}

#define SLIB_DECLARE_RAW_RESOURCE_MAP SLIB_DECLARE_RESOURCE_MAP(slib::Memory)
#define SLIB_DEFINE_RAW_RESOURCE_MAP_BEGIN SLIB_DEFINE_RESOURCE_MAP_BEGIN(slib::Memory)
#define SLIB_DEFINE_RAW_RESOURCE_MAP_ITEM(NAME) SLIB_DEFINE_RESOURCE_MAP_ITEM(NAME)
#define SLIB_DEFINE_RAW_RESOURCE_MAP_PATH(PATH, NAMESPACE_AND_NAME) \
		{ \
			SLIB_STATIC_STRING(_key, PATH) \
			GETTER_TYPE f = &(NAMESPACE_AND_NAME::get); \
			map.put_NoLock(_key, f); \
		}

#define SLIB_DEFINE_RAW_RESOURCE_MAP_END SLIB_DEFINE_RESOURCE_MAP_END(slib::Memory, sl_null)

#endif
