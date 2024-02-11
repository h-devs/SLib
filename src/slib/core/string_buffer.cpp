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

#include "slib/core/string_buffer.h"

#include "slib/core/memory.h"
#include "slib/core/memory_traits.h"

namespace slib
{

	namespace {

		template <class STRING>
		class StringGetter;

		template <>
		class StringGetter<String>
		{
		public:
			static String& get(StringStorage& storage)
			{
				return storage.string8;
			}
		};

		template <>
		class StringGetter<String16>
		{
		public:
			static String16& get(StringStorage& storage)
			{
				return storage.string16;
			}
		};

		template <>
		class StringGetter<String32>
		{
		public:
			static String32& get(StringStorage& storage)
			{
				return storage.string32;
			}
		};

	}

#define DEFINE_STRING_BUFFER_MEMBERS(BUFFER) \
	BUFFER::BUFFER() noexcept: m_len(0) {} \
	\
	BUFFER::~BUFFER() noexcept \
	{ \
	} \
	\
	sl_size BUFFER::getLength() const noexcept \
	{ \
		return m_len; \
	} \
	\
	sl_bool BUFFER::isEmpty() const noexcept \
	{ \
		return !m_len; \
	} \
	\
	sl_bool BUFFER::isNotEmpty() const noexcept \
	{ \
		return m_len != 0; \
	} \
	\
	typename BUFFER::Char BUFFER::getFirstChar() const noexcept \
	{ \
		Link<StringStorage>* link = m_queue.getFront(); \
		if (link) { \
			StringStorage& storage = link->value; \
			return *((typename BUFFER::Char*)(storage.data)); \
		} \
		return 0; \
	} \
	\
	typename BUFFER::Char BUFFER::getLastChar() const noexcept \
	{ \
		Link<StringStorage>* link = m_queue.getBack(); \
		if (link) { \
			StringStorage& storage = link->value; \
			return ((typename BUFFER::Char*)(storage.data))[storage.length - 1]; \
		} \
		return 0; \
	} \
	\
	typename BUFFER::Char BUFFER::getCharAt(sl_size index) const noexcept \
	{ \
		Link<StringStorage>* link = m_queue.getFront(); \
		while (link) { \
			StringStorage& storage = link->value; \
			if (index < storage.length) { \
				return ((typename BUFFER::Char*)(storage.data))[index]; \
			} \
			index -= storage.length; \
			link = link->next; \
		} \
		return 0; \
	} \
	\
	sl_bool BUFFER::add(typename BUFFER::StringType const& str) noexcept \
	{ \
		sl_size len = str.getLength(); \
		if (!len) { \
			return sl_true; \
		} \
		if (m_queue.push_NoLock(str)) { \
			m_len += len; \
			return sl_true; \
		} \
		return sl_false; \
	} \
	\
	sl_bool BUFFER::add(typename BUFFER::StringType&& str) noexcept \
	{ \
		sl_size len = str.getLength(); \
		if (!len) { \
			return sl_true; \
		} \
		if (m_queue.push_NoLock(Move(str))) { \
			m_len += len; \
			return sl_true; \
		} \
		return sl_false; \
	} \
	\
	sl_bool BUFFER::add(const StringStorage& data) noexcept \
	{ \
		sl_size len = data.length; \
		if (!len) { \
			return sl_true; \
		} \
		if (data.data) { \
			if (m_queue.push_NoLock(data)) { \
				m_len += len; \
				return sl_true; \
			} \
		} \
		return sl_false; \
	} \
	\
	sl_bool BUFFER::addStatic(typename BUFFER::Char const* buf, sl_size length) noexcept \
	{ \
		if (!length) { \
			return sl_true; \
		} \
		StringStorage data; \
		data.data = (typename BUFFER::Char*)buf; \
		data.length = length; \
		data.charSize = sizeof(typename BUFFER::Char); \
		return add(data); \
	} \
	\
	void BUFFER::link(BUFFER& buf) noexcept \
	{ \
		m_len += buf.m_len; \
		buf.m_len = 0; \
		m_queue.merge_NoLock(&(buf.m_queue)); \
	} \
	\
	void BUFFER::clear() noexcept \
	{ \
		m_queue.removeAll_NoLock(); \
		m_len = 0; \
	} \
	\
	typename BUFFER::StringType BUFFER::merge() const noexcept \
	{ \
		sl_size total = m_len; \
		if (!total) { \
			return StringType::getEmpty(); \
		} \
		Link<StringStorage>* front = m_queue.getFront(); \
		sl_size nQueueCount = m_queue.getCount(); \
		if (!nQueueCount) { \
			return StringType::getEmpty(); \
		} \
		if (nQueueCount == 1) { \
			StringType& s = StringGetter<StringType>::get(front->value); \
			if (s.isNotNull()) { \
				return s; \
			} \
		} \
		StringType ret = StringType::allocate(total); \
		if (ret.isNotEmpty()) { \
			Char* buf = (Char*)(ret.getData()); \
			sl_size offset = 0; \
			Link<StringStorage>* item = front; \
			while (item) { \
				StringStorage& s = item->value; \
				sl_size t = s.length; \
				if (offset + t > total) { \
					MemoryTraits<Char>::copy(buf + offset, (Char*)(s.data), total - offset); \
					return ret; \
				} \
				MemoryTraits<Char>::copy(buf + offset, (Char*)(s.data), t); \
				offset += t; \
				item = item->next; \
			} \
		} \
		return ret; \
	} \
	\
	Memory BUFFER::mergeToMemory() const noexcept \
	{ \
		if (!(m_queue.getCount())) { \
			return sl_null; \
		} \
		Link<StringStorage>* front = m_queue.getFront(); \
		sl_size total = m_len; \
		Memory ret = Memory::create(total); \
		if (ret.isNotNull()) { \
			Char* buf = (Char*)(ret.getData()); \
			sl_size offset = 0; \
			Link<StringStorage>* item = front; \
			while (item) { \
				StringStorage& s = item->value; \
				sl_size t = s.length; \
				MemoryTraits<Char>::copy(buf + offset, (Char*)(s.data), t); \
				offset += t; \
				item = item->next; \
			} \
		} \
		return ret; \
	} \

	DEFINE_STRING_BUFFER_MEMBERS(StringBuffer)
	DEFINE_STRING_BUFFER_MEMBERS(StringBuffer16)
	DEFINE_STRING_BUFFER_MEMBERS(StringBuffer32)

}
