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

#ifndef CHECKHEADER_SLIB_CORE_MAP_OBJECT
#define CHECKHEADER_SLIB_CORE_MAP_OBJECT

#include "variant.h"
#include "map_iterator.h"
#include "string_cast.h"
#include "string_buffer.h"
#include "parse_util.h"

#include "serialize/variant.h"

namespace slib
{

	template <class CMAP>
	class MapObject : public Object
	{
	public:
		MapObject(CMAP* map) : m_map(map) {}

	public:
		Variant getProperty(const String& name) override
		{
			return m_map->getValue(Cast<String, typename CMAP::KEY_TYPE>()(name));
		}

		sl_bool setProperty(const String& name, const Variant& value) override
		{
			typename CMAP::VALUE_TYPE v;
			value.get(v);
			return m_map->put(Cast<String, typename CMAP::KEY_TYPE>()(name), Move(v));
		}

		sl_bool clearProperty(const String& name) override
		{
			return m_map->remove(Cast<String, typename CMAP::KEY_TYPE>()(name));
		}

		PropertyIterator getPropertyIterator() override
		{
			return new MapIterator<CMAP, String, Variant>(m_map);
		}

		String toString() override
		{
			StringBuffer buf;
			if (toJsonString(buf)) {
				return buf.merge();
			}
			return sl_null;
		}

		sl_bool toJsonString(StringBuffer& buf) override
		{
			ObjectLocker lock(m_map);
			if (!(buf.addStatic("{"))) {
				return sl_false;
			}
			sl_bool flagFirst = sl_true;
			auto node = m_map->getFirstNode();
			while (node) {
				Variant v(node->value);
				if (v.isNotUndefined()) {
					if (!flagFirst) {
						if (!(buf.addStatic(", "))) {
							return sl_false;
						}
					}
					if (!(buf.add(ParseUtil::applyBackslashEscapes(Cast<typename CMAP::KEY_TYPE, String>()(node->key))))) {
						return sl_false;
					}
					if (!(buf.addStatic(": "))) {
						return sl_false;
					}
					if (!(v.toJsonString(buf))) {
						return sl_false;
					}
					flagFirst = sl_false;
				}
				node = node->getNext();
			}
			if (!(buf.addStatic("}"))) {
				return sl_false;
			}
			return sl_true;
		}

		sl_bool toJsonBinary(MemoryBuffer& buf) override
		{
			ObjectLocker lock(m_map);
			if (!(SerializeByte(&buf, (sl_uint8)(VariantType::Object)))) {
				return sl_false;
			}
			sl_size n = m_map->getCount();
			if (!(CVLI::serialize(&buf, n))) {
				return sl_false;
			}
			auto node = m_map->getFirstNode();
			while (node) {
				Variant v(node->value);
				if (v.isNotUndefined()) {
					if (!(Serialize(&buf, Cast<typename CMAP::KEY_TYPE, String>()(node->key)))) {
						return sl_false;
					}
					if (!(Serialize(&buf, v))) {
						return sl_false;
					}
				}
				node = node->getNext();
			}
			return sl_true;
		}

	protected:
		Ref<CMAP> m_map;

	};

	template <class KT, class VT, class KEY_COMPARE>
	Ref<Object> CMap<KT, VT, KEY_COMPARE>::toObject() noexcept
	{
		return new MapObject< CMap<KT, VT, KEY_COMPARE> >(this);
	}

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	Ref<Object> CHashMap<KT, VT, HASH, KEY_COMPARE>::toObject() noexcept
	{
		return new MapObject< CHashMap<KT, VT, HASH, KEY_COMPARE> >(this);
	}


	template <class KT, class VT, class KEY_COMPARE>
	Map<KT, VT, KEY_COMPARE> Map<KT, VT, KEY_COMPARE>::create(Object* object)
	{
		Map<KT, VT, KEY_COMPARE> ret;
		priv::variant::BuildMapFromObject(ret, object);
		return ret;
	}

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	HashMap<KT, VT, HASH, KEY_COMPARE> HashMap<KT, VT, HASH, KEY_COMPARE>::create(Object* object)
	{
		HashMap<KT, VT, HASH, KEY_COMPARE> ret;
		priv::variant::BuildMapFromObject(ret, object);
		return ret;
	}


	template <class KT, class VT, class KEY_COMPARE>
	Variant::Variant(const Map<KT, VT, KEY_COMPARE>& map)
	{
		Ref<Object> object(map.toObject());
		_constructorMoveRef(&object, VariantType::Object);
	}

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	Variant::Variant(const HashMap<KT, VT, HASH, KEY_COMPARE>& map)
	{
		Ref<Object> object(map.toObject());
		_constructorMoveRef(&object, VariantType::Object);
	}

}

#endif
