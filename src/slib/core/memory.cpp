/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/memory.h"
#include "slib/core/memory_buffer.h"
#include "slib/core/memory_queue.h"

#include "slib/core/string_buffer.h"
#include "slib/core/stringx.h"
#include "slib/data/base64.h"
#include "slib/data/json.h"
#include "slib/data/serialize/memory.h"
#include "slib/data/serialize/string.h"

namespace slib
{

	namespace
	{

		static CMemory* Create(sl_size size) noexcept
		{
			if (size) {
				sl_uint8* mem = new sl_uint8[sizeof(CMemory) + size];
				if (mem) {
					CMemory* ret = (CMemory*)mem;
					new (ret) CMemory(mem + sizeof(CMemory), size);
					return ret;
				}
			}
			return sl_null;
		}

		static CMemory* Create(const void* data, sl_size size) noexcept
		{
			CMemory* ret = Create(size);
			if (ret) {
				if (data) {
					Base::copyMemory(ret->data, data, size);
				}
				return ret;
			}
			return sl_null;
		}

		class ResizableMemory : public CMemory
		{
		public:
			ResizableMemory(const void* _data, sl_size _size) noexcept: CMemory(_data, _size) {}

			~ResizableMemory() noexcept
			{
				Base::freeMemory(data);
			}

		public:
			sl_bool isResizable() noexcept override
			{
				return sl_true;
			}

			sl_bool setSize(sl_size sizeNew) noexcept override
			{
				void* p = data;
				if (p && sizeNew) {
					p = Base::reallocMemory(p, sizeNew);
					if (p) {
						data = p;
						size = sizeNew;
						return sl_true;
					}
				}
				return sl_false;
			}

		};

		static CMemory* CreateResizable(sl_size size) noexcept
		{
			if (size) {
				void* mem = Base::createMemory(size);
				if (mem) {
					CMemory* ret = new ResizableMemory(mem, size);
					if (ret) {
						return ret;
					}
					Base::freeMemory(mem);
				}
			}
			return sl_null;
		}

		static CMemory* CreateResizable(const void* data, sl_size size) noexcept
		{
			CMemory* ret = CreateResizable(size);
			if (ret) {
				if (data) {
					Base::copyMemory(ret->data, data, size);
				}
				return ret;
			}
			return sl_null;
		}

		static CMemory* CreateNoCopy(const void* data, sl_size size) noexcept
		{
			if (data && size) {
				CMemory* ret = new ResizableMemory((void*)data, size);
				if (ret) {
					return ret;
				}
			}
			return sl_null;
		}

		class StaticMemory : public CMemory
		{
		public:
			StaticMemory(const void* _data, sl_size _size) noexcept: CMemory(_data, _size) {}

		public:
			CRef* getRef() noexcept override
			{
				return sl_null;
			}

		};

		static CMemory* CreateStatic(const void* data, sl_size size) noexcept
		{
			if (data && size) {
				return new StaticMemory(data, size);
			}
			return sl_null;
		}

		class MemoryWithRef : public CMemory
		{
		public:
			Ref<CRef> ref;

		public:
			template <class REF>
			MemoryWithRef(const void* _data, sl_size _size, REF&& _ref) noexcept: CMemory(_data, _size), ref(Forward<REF>(_ref)) {}

		public:
			CRef* getRef() noexcept override
			{
				return ref.get();
			}

		};

		template <class REF>
		static CMemory* CreateStatic(const void* data, sl_size size, REF&& ref) noexcept
		{
			if (data && size) {
				return new MemoryWithRef(data, size, Forward<REF>(ref));
			}
			return sl_null;
		}

		template <class STRING>
		class MemoryWithString : public CMemory
		{
		public:
			STRING str;

		public:
			template <class TYPE>
			MemoryWithString(const void* _data, sl_size _size, TYPE&& _str) noexcept: CMemory(_data, _size), str(Forward<TYPE>(_str)) {}

		};

		template <class STRING>
		static CMemory* CreateWithString(STRING&& str) noexcept
		{
			auto data = str.getData();
			if (data) {
				sl_size size = str.getLength() * sizeof(decltype(*data));
				if (size) {
					return new MemoryWithString<typename RemoveConstReference<STRING>::Type>(data, size, Forward<STRING>(str));
				}
			}
			return sl_null;
		}

		static CMemory* Concat(const MemoryView& m1, const MemoryView& m2)
		{
			CMemory* ret = Create(m1.size + m2.size);
			if (ret) {
				sl_uint8* data = (sl_uint8*)(ret->data);
				Base::copyMemory(data, m1.data, m1.size);
				Base::copyMemory(data + m1.size, m2.data, m2.size);
				return ret;
			}
			return sl_null;
		}

	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MemoryData)

	MemoryData::MemoryData(const Memory& memory) noexcept
	{
		CMemory* p = memory.ref.get();
		if (p) {
			data = p->data;
			size = p->size;
			new (&ref) Ref<CRef>(p->getRef());
		} else {
			data = sl_null;
			size = 0;
		}
	}

	MemoryData::MemoryData(Memory&& memory) noexcept
	{
		CMemory* p = memory.ref.get();
		if (p) {
			data = p->data;
			size = p->size;
			CRef* r = p->getRef();
			if (r == p) {
				new (&ref) Ref<CRef>(Move(memory.ref));
			} else {
				new (&ref) Ref<CRef>(p->getRef());
			}
		} else {
			data = sl_null;
			size = 0;
		}
	}

	MemoryData& MemoryData::operator=(const Memory& memory) noexcept
	{
		setMemory(memory);
		return *this;
	}

	MemoryData& MemoryData::operator=(Memory&& memory) noexcept
	{
		setMemory(Move(memory));
		return *this;
	}

	void MemoryData::setMemory(const Memory& memory) noexcept
	{
		CMemory* p = memory.ref.get();
		if (p) {
			data = p->data;
			size = p->size;
			ref = p->getRef();
		} else {
			data = sl_null;
			size = 0;
			ref.setNull();
		}
	}

	void MemoryData::setMemory(Memory&& memory) noexcept
	{
		CMemory* p = memory.ref.get();
		if (p) {
			data = p->data;
			size = p->size;
			CRef* r = p->getRef();
			if (r == p) {
				ref = Move(memory.ref);
			} else {
				ref = p->getRef();
			}
		} else {
			data = sl_null;
			size = 0;
			ref.setNull();
		}
	}

	Memory MemoryData::getMemory() const noexcept
	{
		if (CMemory* mem = CastInstance<CMemory>(ref.ptr)) {
			if (mem->data == data && mem->size == size) {
				return mem;
			}
		}
		return Memory::createStatic(data, size, ref.ptr);
	}

	Memory MemoryData::sub(sl_size offset, sl_size sizeSub) const noexcept
	{
		if (offset >= size) {
			return sl_null;
		}
		sl_size limit = size - offset;
		if (sizeSub > limit) {
			sizeSub = limit;
		}
		if (sizeSub == size) {
			return getMemory();
		}
		return Memory::createStatic((sl_uint8*)data + offset, sizeSub, ref.ptr);
	}


	SLIB_DEFINE_ROOT_OBJECT(CMemory)

	CMemory::CMemory(const void* _data, sl_size _size) noexcept: data((void*)_data), size(_size)
	{
	}

	CMemory::~CMemory() noexcept
	{
	}

	sl_bool CMemory::isResizable() noexcept
	{
		return sl_false;
	}

	sl_bool CMemory::setSize(sl_size size) noexcept
	{
		return sl_false;
	}

	CRef* CMemory::getRef() noexcept
	{
		return this;
	}

	String CMemory::getString() noexcept
	{
		sl_size len = size;
		if (len) {
			sl_char8* str = (sl_char8*)data;
			if (!(str[len - 1])) {
				len--;
			}
			CRef* ref = getRef();
			if (ref) {
				return String::fromRef(ref, str, len);
			} else {
				return String::fromStatic(str, len);
			}
		}
		return sl_null;
	}

	String16 CMemory::getString16() noexcept
	{
		sl_size len = size >> 1;
		if (len) {
			sl_char16* str = (sl_char16*)data;
			if (!(str[len - 1])) {
				len--;
			}
			CRef* ref = getRef();
			if (ref) {
				return String16::fromRef(ref, str, len);
			} else {
				return String16::fromStatic(str, len);
			}
		}
		return sl_null;
	}

	String32 CMemory::getString32() noexcept
	{
		sl_size len = size >> 2;
		if (len) {
			sl_char32* str = (sl_char32*)data;
			if (!(str[len - 1])) {
				len--;
			}
			CRef* ref = getRef();
			if (ref) {
				return String32::fromRef(ref, str, len);
			} else {
				return String32::fromStatic(str, len);
			}
		}
		return sl_null;
	}

	String CMemory::toString()
	{
		return getString();
	}

	sl_bool CMemory::toJsonString(StringBuffer& buf)
	{
		Json binary;
		SLIB_STATIC_STRING(strBase64, "base64")
		binary.putItem(strBase64, Base64::encode(data, size));
		SLIB_STATIC_STRING(strSubType, "subType")
		binary.putItem(strSubType, "00");
		Json json;
		SLIB_STATIC_STRING(strBinary, "$binary")
		json.putItem(strBinary, binary);
		return buf.add(json.toJsonString());
	}

	sl_bool CMemory::toJsonBinary(MemoryBuffer& buf)
	{
		if (!(SerializeByte(&buf, (sl_uint8)(VariantType::Memory)))) {
			return sl_false;
		}
		return serialize(&buf);
	}

	CMemory* CMemory::sub(sl_size offset, sl_size sizeSub) noexcept
	{
		sl_size sizeParent = size;
		if (offset < sizeParent) {
			sl_size limit = sizeParent - offset;
			if (sizeSub > limit) {
				sizeSub = limit;
			}
			if (sizeSub) {
				if (sizeParent == sizeSub) {
					return this;
				} else {
					return CreateStatic((sl_uint8*)data + offset, sizeSub, getRef());
				}
			}
		}
		return sl_null;
	}

	sl_size CMemory::read(sl_size offset, void* dst, sl_size sizeRead) noexcept
	{
		const sl_uint8* pSrc = (const sl_uint8*)data;
		sl_uint8* pDst = (sl_uint8*)dst;
		if (pDst && pSrc) {
			sl_size sizeSrc = size;
			if (offset < sizeSrc) {
				sl_size n = sizeSrc - offset;
				if (sizeRead > n) {
					sizeRead = n;
				}
				if (sizeRead) {
					Base::copyMemory(pDst, pSrc + offset, sizeRead);
					return sizeRead;
				}
			}
		}
		return 0;
	}

	sl_size CMemory::write(sl_size offset, const void* src, sl_size sizeWrite) noexcept
	{
		sl_uint8* pDst = (sl_uint8*)data;
		const sl_uint8* pSrc = (const sl_uint8*)src;
		if (pSrc && pDst && sizeWrite) {
			sl_size sizeTarget = size;
			if (offset < sizeTarget) {
				sl_size n = sizeTarget - offset;
				if (sizeWrite > n) {
					sizeWrite = n;
				}
				if (sizeWrite) {
					Base::copyMemory(pDst + offset, pSrc, sizeWrite);
					return sizeWrite;
				}
			}
		}
		return 0;
	}

	sl_size CMemory::write(sl_size offset, const MemoryView& src) noexcept
	{
		return write(offset, src.data, src.size);
	}

	CMemory* CMemory::duplicate() noexcept
	{
		return Create(data, size);
	}

	sl_bool CMemory::serialize(MemoryBuffer* output) noexcept
	{
		if (!(CVLI::serialize(output, size))) {
			return sl_false;
		}
		if (size) {
			return output->add(data, size, getRef());
		} else {
			return sl_true;
		}
	}


	Memory Memory::create(sl_size size) noexcept
	{
		return Create(size);
	}

	Memory Memory::create(const void* buf, sl_size size) noexcept
	{
		return Create(buf, size);
	}

	Memory Memory::createResizable(sl_size size) noexcept
	{
		return CreateResizable(size);
	}

	Memory Memory::createResizable(const void* buf, sl_size size) noexcept
	{
		return CreateResizable(buf, size);
	}

	Memory Memory::createNoCopy(const void* buf, sl_size size) noexcept
	{
		return CreateNoCopy(buf, size);
	}

	Memory Memory::createStatic(const void* buf, sl_size size) noexcept
	{
		return CreateStatic(buf, size);
	}

	Memory Memory::_createStatic(const void* buf, sl_size size, CRef* ref) noexcept
	{
		if (ref) {
			return CreateStatic(buf, size, ref);
		} else {
			return CreateStatic(buf, size);
		}
	}

	Memory Memory::_createStaticMove(const void* buf, sl_size size, void* pRef) noexcept
	{
		return CreateStatic(buf, size, Move(*((Ref<CRef>*)pRef)));
	}

	Memory Memory::createFromString(const String& str) noexcept
	{
		return CreateWithString(str);
	}

	Memory Memory::createFromString(String&& str) noexcept
	{
		return CreateWithString(Move(str));
	}

	Memory Memory::createFromString(const String16& str) noexcept
	{
		return CreateWithString(str);
	}

	Memory Memory::createFromString(String16&& str) noexcept
	{
		return CreateWithString(Move(str));
	}

	Memory Memory::createFromString(const String32& str) noexcept
	{
		return CreateWithString(str);
	}

	Memory Memory::createFromString(String32&& str) noexcept
	{
		return CreateWithString(Move(str));
	}

	Memory Memory::createFromExtendedJson(const Json& json, sl_uint32* pOutSubType)
	{
		SLIB_STATIC_STRING(strBinary, "$binary")
		Json binary = json.getItem(strBinary);
		if (!(binary.isJsonMap())) {
			return sl_null;
		}
		if (pOutSubType) {
			SLIB_STATIC_STRING(strSubType, "subType")
			if (!(binary.getItem(strSubType).getString().parseUint32(16, pOutSubType))) {
				return sl_null;
			}
		}
		SLIB_STATIC_STRING(strBase64, "base64")
		String base64 = binary.getItem(strBase64).getString();
		if (base64.isNotEmpty()) {
			return Base64::decode(base64);
		}
		return sl_null;
	}

	void* Memory::getData() const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->data;
		}
		return sl_null;
	}

	sl_size Memory::getSize() const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->size;
		}
		return 0;
	}

	sl_bool Memory::setSize(sl_size size) noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			if (size) {
				return obj->setSize(size);
			} else {
				ref.setNull();
				return sl_true;
			}
		} else {
			if (size) {
				ref = CreateResizable(size);
				return ref.isNotNull();
			} else {
				return sl_true;
			}
		}
	}

	CRef* Memory::getRef() const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->getRef();
		}
		return sl_null;
	}

	sl_bool Memory::isResizable() const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->isResizable();
		}
		return sl_false;
	}

	Memory Memory::sub(sl_size offset, sl_size size) const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->sub(offset, size);
		}
		return sl_null;
	}

	sl_size Memory::read(sl_size offset, void* dst, sl_size size) const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->read(offset, dst, size);
		}
		return 0;
	}

	sl_size Memory::write(sl_size offset, const void* src, sl_size size) const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->write(offset, src, size);
		}
		return 0;
	}

	sl_size Memory::write(sl_size offset, const MemoryView& src) const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->write(offset, src);
		}
		return 0;
	}

	Memory Memory::duplicate() const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->duplicate();
		}
		return sl_null;
	}

	sl_bool Memory::getData(MemoryData& data) const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			data.data = obj->data;
			data.size = obj->size;
			data.ref = obj->getRef();
			return sl_true;
		} else {
			data.data = sl_null;
			data.size = 0;
			data.ref.setNull();
			return sl_false;
		}
	}

	namespace
	{
		static sl_compare_result CompareMemory(const MemoryView& m1, const MemoryView& m2) noexcept
		{
			if (m1.size == m2.size) {
				if (!(m1.size)) {
					return 0;
				}
				return Base::compareMemory((sl_uint8*)(m1.data), (sl_uint8*)(m2.data), m1.size);
			} else if (m1.size > m2.size) {
				if (!(m2.size)) {
					return 1;
				}
				sl_compare_result result = Base::compareMemory((sl_uint8*)(m1.data), (sl_uint8*)(m2.data), m2.size);
				if (!result) {
					return 1;
				}
				return result;
			} else {
				if (!(m1.size)) {
					return -1;
				}
				sl_compare_result result = Base::compareMemory((sl_uint8*)(m1.data), (sl_uint8*)(m2.data), m1.size);
				if (!result) {
					return -1;
				}
				return result;
			}
		}

		static sl_bool EqualsMemory(const MemoryView m1, const MemoryView& m2) noexcept
		{
			if (m1.size == m2.size) {
				if (!(m1.size)) {
					return sl_true;
				}
				return Base::equalsMemory(m1.data, m2.data, m1.size);
			} else {
				return sl_false;
			}
		}
	}

	sl_compare_result Memory::compare(const Memory& other) const noexcept
	{
		return CompareMemory(*this, other);
	}

	sl_bool Memory::equals(const Memory& other) const noexcept
	{
		return EqualsMemory(*this, other);
	}

	sl_compare_result Memory::compare(const MemoryView& other) const noexcept
	{
		return CompareMemory(*this, other);
	}

	sl_bool Memory::equals(const MemoryView& other) const noexcept
	{
		return EqualsMemory(*this, other);
	}

	sl_size Memory::getHashCode() const noexcept
	{
		sl_size size = getSize();
		if (size) {
			return HashBytes(getData(), size);
		}
		return 0;
	}

	Memory Memory::operator+(const MemoryView& other) const& noexcept
	{
		if (other.size) {
			return Concat(*this, other);
		} else {
			return *this;
		}
	}

	Memory Memory::operator+(const MemoryView& other) && noexcept
	{
		Memory ret;
		if (other.size) {
			ret = Concat(*this, other);
		} else {
			ret = Move(*this);
		}
		return ret;
	}

	Memory Memory::operator+(const Memory& other) const& noexcept
	{
		if (isNull()) {
			return other;
		}
		if (other.isNull()) {
			return *this;
		}
		return Concat(*this, other);
	}

	Memory Memory::operator+(const Memory& other) && noexcept
	{
		Memory ret;
		if (isNull()) {
			ret = other;
		} else if (other.isNull()) {
			ret = Move(*this);
		} else {
			ret = Concat(*this, other);
		}
		return ret;
	}

	Memory Memory::operator+(Memory&& other) const& noexcept
	{
		Memory ret;
		if (isNull()) {
			ret = Move(other);
		} else if (other.isNull()) {
			ret = *this;
		} else {
			ret = Concat(*this, other);
		}
		return ret;
	}

	Memory Memory::operator+(Memory&& other) && noexcept
	{
		Memory ret;
		if (isNull()) {
			ret = Move(other);
		} else if (other.isNull()) {
			ret = Move(*this);
		} else {
			ret = Concat(*this, other);
		}
		return ret;
	}

	sl_bool Memory::serialize(MemoryBuffer* output) const
	{
		CMemory* mem = ref.get();
		if (mem) {
			return mem->serialize(output);
		} else {
			return SerializeStatic(output, "", 1);
		}
	}

	sl_bool Memory::deserialize(SerializeBuffer* input)
	{
		sl_size size;
		if (!(CVLI::deserialize(input, size))) {
			return sl_false;
		}
		if (size) {
			if (input->current + size <= input->end) {
				if (input->ref.isNotNull()) {
					*this = Memory::createStatic(input->current, size, input->ref);
				} else {
					*this = Memory::create(input->current, size);
				}
				if (isNotNull()) {
					input->current += size;
					return sl_true;
				}
			}
			return sl_false;
		} else {
			setNull();
			return sl_true;
		}
	}


	sl_bool Deserialize(SerializeBuffer* input, String& _out)
	{
		sl_size size;
		if (!(CVLI::deserialize(input, size))) {
			return sl_false;
		}
		if (!size) {
			_out.setEmpty();
			return sl_true;
		}
		if (input->current + size > input->end) {
			return sl_false;
		}
		if (input->ref.isNotNull()) {
			_out = String::fromRef(input->ref, (sl_char8*)(input->current), size);
		} else {
			_out = String((sl_char8*)(input->current), size);
		}
		if (_out.isNull()) {
			return sl_false;
		}
		input->current += size;
		return sl_true;
	}


	MemoryView::MemoryView(const Memory& mem) noexcept: data(mem.getData()), size(mem.getSize())
	{
	}

	MemoryView MemoryView::sub(sl_size offset, sl_size sizeSub) const noexcept
	{
		if (offset >= size) {
			return MemoryView();
		}
		sl_size limit = size - offset;
		if (sizeSub > limit) {
			sizeSub = limit;
		}
		return MemoryView((sl_uint8*)data + offset, sizeSub);
	}

	MemoryView& MemoryView::operator=(const Memory& mem) noexcept
	{
		data = mem.getData();
		size = mem.getSize();
		return *this;
	}

	Memory MemoryView::operator+(const MemoryView& other) const noexcept
	{
		return Concat(*this, other);
	}

	Memory MemoryView::operator+(const Memory& other) const noexcept
	{
		if (!size) {
			return other;
		}
		return Concat(*this, other);
	}

	Memory MemoryView::operator+(Memory&& other) const noexcept
	{
		Memory ret;
		if (!size) {
			ret = Move(other);
		} else {
			ret = Concat(*this, other);
		}
		return ret;
	}

	sl_compare_result MemoryView::compare(const MemoryView& other) const noexcept
	{
		return CompareMemory(*this, other);
	}

	sl_bool MemoryView::equals(const MemoryView& other) const noexcept
	{
		return EqualsMemory(*this, other);
	}


	MemoryBuffer::MemoryBuffer(): m_size(0)
	{
	}

	MemoryBuffer::~MemoryBuffer()
	{
	}

	sl_size MemoryBuffer::getSize() const
	{
		return m_size;
	}

	sl_bool MemoryBuffer::add(const MemoryData& mem)
	{
		if (!(mem.size)) {
			return sl_true;
		}
		if (mem.data) {
			if (m_queue.push_NoLock(mem)) {
				m_size += mem.size;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool MemoryBuffer::add(MemoryData&& mem)
	{
		if (!(mem.size)) {
			return sl_true;
		}
		if (mem.data) {
			if (m_queue.push_NoLock(Move(mem))) {
				m_size += mem.size;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool MemoryBuffer::add(const Memory& mem)
	{
		return add(MemoryData(mem));
	}

	sl_bool MemoryBuffer::add(Memory&& mem)
	{
		return add(MemoryData(Move(mem)));
	}

	sl_bool MemoryBuffer::addNew(const void* buf, sl_size size)
	{
		if (!size) {
			return sl_true;
		}
		if (buf) {
			Memory mem = Memory::create(buf, size);
			if (mem.isNotNull()) {
				if (m_queue.push_NoLock(Move(mem))) {
					m_size += size;
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool MemoryBuffer::addNew(const MemoryView& mem)
	{
		return addNew(mem.data, mem.size);
	}

	sl_bool MemoryBuffer::addStatic(const void* buf, sl_size size)
	{
		if (!size) {
			return sl_true;
		}
		if (buf) {
			return add(MemoryData(buf, size));
		}
		return sl_false;
	}

	sl_bool MemoryBuffer::addStatic(const MemoryView& mem)
	{
		return addStatic(mem.data, mem.size);
	}

	sl_bool MemoryBuffer::pop(MemoryData& data)
	{
		return m_queue.popFront_NoLock(&data);
	}

	sl_bool MemoryBuffer::pushFront(const MemoryData& data)
	{
		return m_queue.pushFront_NoLock(data);
	}

	void MemoryBuffer::link(MemoryBuffer& buf)
	{
		m_size += buf.m_size;
		buf.m_size = 0;
		m_queue.merge_NoLock(&(buf.m_queue));
	}

	void MemoryBuffer::clear()
	{
		if (!m_size) {
			return;
		}
		m_queue.removeAll_NoLock();
		m_size = 0;
	}

	Memory MemoryBuffer::merge() const
	{
		if (m_queue.getCount() == 0) {
			return sl_null;
		}
		Link<MemoryData>* front = m_queue.getFront();
		if (m_queue.getCount() == 1) {
			return front->value.getMemory();
		}
		sl_size total = m_size;
		Memory ret = Memory::create(total);
		if (ret.isNotNull()) {
			char* buf = (char*)(ret.getData());
			sl_size offset = 0;
			Link<MemoryData>* item = front;
			while (item) {
				MemoryData& m = item->value;
				sl_size t = m.size;
				Base::copyMemory(buf + offset, m.data, t);
				offset += t;
				item = item->next;
			}
		}
		return ret;
	}

	MemoryData* MemoryBuffer::getLastData() const
	{
		Link<MemoryData>* link = m_queue.getBack();
		if (link) {
			return &(link->value);
		}
		return sl_null;
	}


	MemoryQueue::MemoryQueue()
	{
		m_size = 0;
		m_memCurrent.data = sl_null;
		m_memCurrent.size = 0;
		m_posCurrent = 0;
	}

	MemoryQueue::~MemoryQueue()
	{
	}

	sl_size MemoryQueue::getSize() const
	{
		return m_size;
	}

	sl_bool MemoryQueue::add_NoLock(const MemoryData& mem)
	{
		if (!(mem.size)) {
			return sl_true;
		}
		if (mem.data) {
			if (m_queue.push_NoLock(mem)) {
				m_size += mem.size;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool MemoryQueue::add_NoLock(MemoryData&& mem)
	{
		if (!(mem.size)) {
			return sl_true;
		}
		if (mem.data) {
			if (m_queue.push_NoLock(Move(mem))) {
				m_size += mem.size;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool MemoryQueue::add(const MemoryData& mem)
	{
		if (!(mem.size)) {
			return sl_true;
		}
		if (mem.data) {
			ObjectLocker lock(this);
			if (m_queue.push_NoLock(mem)) {
				m_size += mem.size;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool MemoryQueue::add(MemoryData&& mem)
	{
		if (!(mem.size)) {
			return sl_true;
		}
		if (mem.data) {
			ObjectLocker lock(this);
			if (m_queue.push_NoLock(Move(mem))) {
				m_size += mem.size;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool MemoryQueue::add_NoLock(const Memory& mem)
	{
		return add_NoLock(MemoryData(mem));
	}

	sl_bool MemoryQueue::add_NoLock(Memory&& mem)
	{
		return add_NoLock(MemoryData(Move(mem)));
	}

	sl_bool MemoryQueue::add(const Memory& mem)
	{
		return add(MemoryData(mem));
	}

	sl_bool MemoryQueue::add(Memory&& mem)
	{
		return add(MemoryData(Move(mem)));
	}

	sl_bool MemoryQueue::addNew_NoLock(const void* buf, sl_size size)
	{
		if (!size) {
			return sl_true;
		}
		if (buf) {
			Memory mem = Memory::create(buf, size);
			if (mem.isNotNull()) {
				if (m_queue.push_NoLock(Move(mem))) {
					m_size += size;
					return sl_true;
				}
			}

		}
		return sl_false;
	}

	sl_bool MemoryQueue::addNew(const void* buf, sl_size size)
	{
		if (!size) {
			return sl_true;
		}
		if (buf) {
			Memory mem = Memory::create(buf, size);
			if (mem.isNotNull()) {
				ObjectLocker lock(this);
				if (m_queue.push_NoLock(Move(mem))) {
					m_size += size;
					return sl_true;
				}
			}

		}
		return sl_false;
	}

	sl_bool MemoryQueue::addStatic_NoLock(const void* buf, sl_size size)
	{
		if (!size) {
			return sl_true;
		}
		if (buf) {
			MemoryData data;
			data.data = (void*)buf;
			data.size = size;
			return add_NoLock(data);
		}
		return sl_false;
	}

	sl_bool MemoryQueue::addStatic(const void* buf, sl_size size)
	{
		if (!size) {
			return sl_true;
		}
		if (buf) {
			MemoryData data;
			data.data = (void*)buf;
			data.size = size;
			return add(data);
		}
		return sl_false;
	}

	void MemoryQueue::link_NoLock(MemoryQueue& buf)
	{
		m_size += buf.m_size;
		buf.m_size = 0;
		m_queue.merge_NoLock(&(buf.m_queue));
	}

	void MemoryQueue::link(MemoryQueue& buf)
	{
		MultipleObjectsLocker lock(this, &buf);
		m_size += buf.m_size;
		buf.m_size = 0;
		m_queue.merge_NoLock(&(buf.m_queue));
	}

	void MemoryQueue::clear_NoLock()
	{
		m_queue.removeAll_NoLock();
		m_size = 0;
		m_memCurrent.data = sl_null;
		m_memCurrent.size = 0;
		m_posCurrent = 0;
	}

	void MemoryQueue::clear()
	{
		ObjectLocker lock(this);
		clear_NoLock();
	}

	sl_bool MemoryQueue::pop_NoLock(MemoryData& data)
	{
		MemoryData mem = Move(m_memCurrent);
		if (mem.size > 0) {
			sl_size pos = m_posCurrent;
			m_memCurrent.size = 0;
			m_posCurrent = 0;
			if (pos < mem.size) {
				data.data = (sl_uint8*)(mem.data) + pos;
				data.size = mem.size - pos;
				data.ref = Move(mem.ref);
				m_size -= data.size;
				return sl_true;
			}
		} else {
			if (m_queue.pop_NoLock(&data)) {
				if (data.size > 0) {
					m_size -= data.size;
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool MemoryQueue::pop(MemoryData& data)
	{
		ObjectLocker lock(this);
		return pop_NoLock(data);
	}

	sl_size MemoryQueue::pop_NoLock(void* _buf, sl_size size)
	{
		char* buf = (char*)_buf;
		sl_size nRead = 0;
		while (nRead < size) {
			MemoryData mem = m_memCurrent;
			sl_size pos = m_posCurrent;
			m_memCurrent.size = 0;
			m_posCurrent = 0;
			if (mem.size == 0) {
				m_queue.pop_NoLock(&mem);
				pos = 0;
			}
			if (mem.size == 0) {
				break;
			}
			sl_size n = size - nRead;
			sl_size m = mem.size;
			if (pos > m) {
				pos = m;
			}
			m -= pos;
			if (n >= m) {
				Base::copyMemory(buf + nRead, (char*)(mem.data) + pos, m);
				nRead += m;
			} else {
				Base::copyMemory(buf + nRead, (char*)(mem.data) + pos, n);
				nRead += n;
				m_posCurrent = pos + n;
				m_memCurrent = mem;
			}
		}
		m_size -= nRead;
		return nRead;
	}

	sl_size MemoryQueue::pop(void* buf, sl_size size)
	{
		ObjectLocker lock(this);
		return pop_NoLock(buf, size);
	}

	Memory MemoryQueue::merge_NoLock() const
	{
		if (m_queue.getCount() == 0) {
			return m_memCurrent.sub(m_posCurrent);
		}
		Link<MemoryData>* front = m_queue.getFront();
		if (m_queue.getCount() == 1 && m_memCurrent.size == 0) {
			return front->value.getMemory();
		}
		sl_size total = m_size;
		Memory ret = Memory::create(total);
		if (ret.isNotNull()) {
			char* buf = (char*)(ret.getData());
			sl_size offset = 0;
			if (m_memCurrent.size > 0) {
				sl_size p = m_posCurrent;
				sl_size s = m_memCurrent.size;
				if (p < s) {
					s -= p;
					Base::copyMemory(buf, (char*)(m_memCurrent.data) + p, s);
					offset = s;
				}
			}
			Link<MemoryData>* item = front;
			while (item) {
				MemoryData& m = item->value;
				sl_size t = m.size;
				Base::copyMemory(buf + offset, m.data, t);
				offset += t;
				item = item->next;
			}
		}
		return ret;
	}

	Memory MemoryQueue::merge() const
	{
		ObjectLocker lock(this);
		return merge_NoLock();
	}

}
