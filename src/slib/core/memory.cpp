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

#include "slib/core/memory.h"
#include "slib/core/memory_buffer.h"
#include "slib/core/memory_queue.h"

#include "slib/core/string.h"
#include "slib/core/string_buffer.h"
#include "slib/core/parse_util.h"
#include "slib/core/json.h"
#include "slib/core/serialize/memory.h"
#include "slib/core/serialize/string.h"

#include "slib/crypto/base64.h"

namespace slib
{

	namespace priv
	{
		namespace memory
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
					if (data) {
						Base::freeMemory(data);
					}
				}

			public:
				sl_bool isResizable() noexcept override
				{
					return sl_true;
				}

				sl_bool setSize(sl_size sizeNew) noexcept override
				{
					void* p = data;
					if (p) {
						if (sizeNew) {
							p = Base::reallocMemory(p, sizeNew);
							if (p) {
								data = p;
								size = sizeNew;
								return sl_true;
							}
						} else {
							Base::freeMemory(p);
							data = sl_null;
							size = 0;
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

			class HeapMemory : public CMemory
			{
			public:
				HeapMemory(const void* _data, sl_size _size) noexcept: CMemory(_data, _size) {}

				~HeapMemory() noexcept
				{
					Base::freeMemory(data);
				}

			};

			static CMemory* CreateNoCopy(const void* data, sl_size size) noexcept
			{
				if (data && size) {
					CMemory* ret = new HeapMemory((void*)data, size);
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
				Referable * getRef() noexcept override
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
				Ref<Referable> ref;

			public:
				template <class REF>
				MemoryWithRef(const void* _data, sl_size _size, REF&& _ref) noexcept: CMemory(_data, _size), ref(Forward<REF>(_ref)) {}

			public:
				Referable* getRef() noexcept override
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

			class MemoryWithString : public CMemory
			{
			public:
				String str;

			public:
				template <class STRING>
				MemoryWithString(const void* _data, sl_size _size, STRING&& _str) noexcept : CMemory(_data, _size), str(Forward<STRING>(_str)) {}

			public:
				String getString() noexcept override
				{
					return str;
				}

			};

			class MemoryWithString16 : public CMemory
			{
			public:
				String16 str;

			public:
				template <class STRING>
				MemoryWithString16(const void* _data, sl_size _size, STRING&& _str) noexcept : CMemory(_data, _size), str(Forward<STRING>(_str)) {}

			public:
				String16 getString16() noexcept override
				{
					return str;
				}

			};

		}
	}

	using namespace priv::memory;

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MemoryData)

	MemoryData::MemoryData() noexcept : data(sl_null), size(0)
	{
	}

	MemoryData::MemoryData(const void* _data, sl_size _size) noexcept: data((void*)_data), size(_size)
	{
	}

	MemoryData::MemoryData(const Memory& memory) noexcept
	{
		CMemory* p = memory.ref.get();
		if (p) {
			data = p->data;
			size = p->size;
			new (&ref) Ref<Referable>(p->getRef());
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
			Referable* r = p->getRef();
			if (r == p) {
				new (&ref) Ref<Referable>(Move(memory.ref));
			} else {
				new (&ref) Ref<Referable>(p->getRef());
			}
		} else {
			data = sl_null;
			size = 0;
		}
	}

	MemoryData& MemoryData::operator=(const Memory& memory) noexcept
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
		return *this;
	}

	MemoryData& MemoryData::operator=(Memory&& memory) noexcept
	{
		CMemory* p = memory.ref.get();
		if (p) {
			data = p->data;
			size = p->size;
			Referable* r = p->getRef();
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
		return *this;
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

	Referable* CMemory::getRef() noexcept
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
			Referable* ref = getRef();
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
			Referable* ref = getRef();
			if (ref) {
				return String16::fromRef(ref, str, len);
			} else {
				return String16::fromStatic(str, len);
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
		binary.putItem_NoLock(strBase64, Base64::encode(data, size));
		SLIB_STATIC_STRING(strSubType, "subType")
		binary.putItem_NoLock(strSubType, "00");
		Json json;
		SLIB_STATIC_STRING(strBinary, "$binary")
		json.putItem_NoLock(strBinary, binary);
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

	sl_size CMemory::read(sl_size offsetSource, sl_size sizeRead, void* dst) noexcept
	{
		sl_uint8* pSrc = (sl_uint8*)data;
		sl_uint8* pDst = (sl_uint8*)dst;
		if (pDst && pSrc) {
			sl_size sizeSrc = size;
			if (offsetSource < sizeSrc) {
				sl_size n = sizeSrc - offsetSource;
				if (sizeRead > n) {
					sizeRead = n;
				}
				if (sizeRead) {
					Base::copyMemory(pDst, pSrc + offsetSource, sizeRead);
					return sizeRead;
				}
			}
		}
		return 0;
	}

	sl_size CMemory::write(sl_size offsetTarget, sl_size sizeWrite, const void* src) noexcept
	{
		sl_uint8* pDst = (sl_uint8*)data;
		sl_uint8* pSrc = (sl_uint8*)src;
		if (pSrc && pDst && sizeWrite) {
			sl_size sizeTarget = size;
			if (offsetTarget < sizeTarget) {
				sl_size n = sizeTarget - offsetTarget;
				if (sizeWrite > n) {
					sizeWrite = n;
				}
				if (sizeWrite) {
					Base::copyMemory(pDst + offsetTarget, pSrc, sizeWrite);
					return sizeWrite;
				}
			}
		}
		return 0;
	}

	sl_size CMemory::copy(sl_size offsetTarget, const CMemory* source, sl_size offsetSource, sl_size sizeCopy) noexcept
	{
		if (source) {
			sl_uint8* pSrc = (sl_uint8*)(source->data);
			if (pSrc) {
				sl_size sizeSrc = source->size;
				if (offsetSource < sizeSrc) {
					sl_size n = sizeSrc - offsetSource;
					if (sizeCopy > n) {
						sizeCopy = n;
					}
					return write(offsetSource, sizeCopy, pSrc + offsetSource);
				}
			}
		}
		return 0;
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

	Memory Memory::_createStatic(const void* buf, sl_size size, Referable* ref) noexcept
	{
		if (ref) {
			return CreateStatic(buf, size, ref);
		} else {
			return CreateStatic(buf, size);
		}
	}

	Memory Memory::_createStaticMove(const void* buf, sl_size size, void* pRef) noexcept
	{
		return CreateStatic(buf, size, Move(*((Ref<Referable>*)pRef)));
	}

	Memory Memory::createFromString(const String& str) noexcept
	{
		sl_char8* data = str.getData();
		sl_size size = str.getLength();
		if (data && size) {
			return new MemoryWithString(data, size, str);
		} else {
			return sl_null;
		}
	}

	Memory Memory::createFromString(String&& str) noexcept
	{
		sl_char8* data = str.getData();
		sl_size size = str.getLength();
		if (data && size) {
			return new MemoryWithString(data, size, Move(str));
		} else {
			return sl_null;
		}
	}

	Memory Memory::createFromString16(const String16& str) noexcept
	{
		sl_char16* data = str.getData();
		sl_size size = str.getLength() << 1;
		if (data && size) {
			return new MemoryWithString16(data, size, str);
		} else {
			return sl_null;
		}
	}

	Memory Memory::createFromString16(String16&& str) noexcept
	{
		sl_char16* data = str.getData();
		sl_size size = str.getLength() << 1;
		if (data && size) {
			return new MemoryWithString16(data, size, Move(str));
		} else {
			return sl_null;
		}
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

	Referable* Memory::getRef() const noexcept
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

	sl_size Memory::read(sl_size offsetSource, sl_size size, void* bufDst) const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->read(offsetSource, size, (sl_uint8*)bufDst);
		}
		return 0;
	}

	sl_size Memory::write(sl_size offsetTarget, sl_size size, const void* bufSrc) const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->write(offsetTarget, size, (const sl_uint8*)bufSrc);
		}
		return 0;
	}

	sl_size Memory::copy(sl_size offsetTarget, const Memory& source, sl_size offsetSource, sl_size size) const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->copy(offsetTarget, source.ref.ptr, offsetSource, size);
		}
		return 0;
	}

	sl_size Memory::copy(const Memory& source, sl_size offset, sl_size size) const noexcept
	{
		return copy(0, source, offset, size);
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

	sl_compare_result Memory::compare(const Memory& other) const noexcept
	{
		sl_size size1 = getSize();
		sl_size size2 = other.getSize();
		if (size1 == size2) {
			if (!size1) {
				return 0;
			}
			return Base::compareMemory((sl_uint8*)(getData()), (sl_uint8*)(other.getData()), size1);
		} else if (size1 > size2) {
			if (!size2) {
				return 1;
			}
			sl_compare_result result = Base::compareMemory((sl_uint8*)(getData()), (sl_uint8*)(other.getData()), size2);
			if (result == 0) {
				return 1;
			}
			return result;
		} else {
			if (!size1) {
				return -1;
			}
			sl_compare_result result = Base::compareMemory((sl_uint8*)(getData()), (sl_uint8*)(other.getData()), size1);
			if (result == 0) {
				return -1;
			}
			return result;
		}
	}
	
	sl_bool Memory::equals(const Memory& other) const noexcept
	{
		sl_size size1 = getSize();
		sl_size size2 = other.getSize();
		if (size1 == size2) {
			if (!size1) {
				return sl_true;
			}
			return Base::equalsMemory(getData(), other.getData(), size1);
		} else {
			return sl_false;
		}
	}
	
	sl_size Memory::getHashCode() const noexcept
	{
		sl_size size = getSize();
		if (size) {
			return HashBytes(getData(), size);
		}
		return 0;
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

	sl_bool Memory::deserialize(DeserializeBuffer* input)
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


	sl_size Atomic<Memory>::getSize() const noexcept
	{
		Ref<CMemory> obj(ref);
		if (obj.isNotNull()) {
			return obj->size;
		}
		return 0;
	}

	Memory Atomic<Memory>::sub(sl_size offset, sl_size size) const noexcept
	{
		Ref<CMemory> obj(ref);
		if (obj.isNotNull()) {
			return obj->sub(offset, size);
		}
		return sl_null;
	}

	sl_size Atomic<Memory>::read(sl_size offsetSource, sl_size size, void* bufDst) const noexcept
	{
		Ref<CMemory> obj(ref);
		if (obj.isNotNull()) {
			return obj->read(offsetSource, size, (sl_uint8*)bufDst);
		}
		return 0;
	}

	sl_size Atomic<Memory>::write(sl_size offsetTarget, sl_size size, const void* bufSrc) const noexcept
	{
		Ref<CMemory> obj(ref);
		if (obj.isNotNull()) {
			return obj->write(offsetTarget, size, (const sl_uint8*)bufSrc);
		}
		return 0;
	}

	sl_size Atomic<Memory>::copy(sl_size offsetTarget, const Memory& source, sl_size offsetSource, sl_size size) const noexcept
	{
		Ref<CMemory> obj(ref);
		if (obj.isNotNull()) {
			return obj->copy(offsetTarget, source.ref.ptr, offsetSource, size);
		}
		return 0;
	}

	sl_size Atomic<Memory>::copy(const Memory& source, sl_size offsetSource, sl_size size) const noexcept
	{
		Ref<CMemory> obj(ref);
		if (obj.isNotNull()) {
			return obj->copy(0, source.ref.ptr, offsetSource, size);
		}
		return 0;
	}

	Memory Atomic<Memory>::duplicate() const noexcept
	{
		Ref<CMemory> obj(ref);
		if (obj.isNotNull()) {
			return obj->duplicate();
		}
		return sl_null;
	}

	sl_bool Atomic<Memory>::getData(MemoryData& data) const noexcept
	{
		Memory mem(*this);
		return mem.getData(data);
	}

	sl_compare_result Atomic<Memory>::compare(const Memory& other) const noexcept
	{
		Memory mem(*this);
		return mem.compare(other);
	}

	sl_bool Atomic<Memory>::equals(const Memory& other) const noexcept
	{
		Memory mem(*this);
		return mem.equals(other);
	}

	sl_size Atomic<Memory>::getHashCode() const noexcept
	{
		Memory mem(*this);
		return mem.getHashCode();
	}
	
	
	sl_bool Serialize(MemoryBuffer* output, const String& _in)
	{
		return _in.toMemory().serialize(output);
	}

	sl_bool Deserialize(DeserializeBuffer* input, String& _out)
	{
		sl_size size;
		if (!(CVLI::deserialize(input, size))) {
			return sl_false;
		}
		if (size) {
			if (input->current + size <= input->end) {
				if (input->ref.isNotNull()) {
					_out = String::fromRef(input->ref, (sl_char8*)(input->current), size);
				} else {
					_out = String((sl_char8*)(input->current), size);
				}
				if (_out.isNotNull()) {
					input->current += size;
					return sl_true;
				}
			}
			return sl_false;
		} else {
			_out.setNull();
			return sl_true;
		}
	}


	SLIB_DEFINE_DEFAULT_COMPARE_OPERATORS(Memory)

	Memory operator+(const Memory& a, const Memory& b) noexcept
	{
		if (a.isNull()) {
			return b;
		}
		if (b.isNull()) {
			return a;
		}
		sl_size n1 = a.getSize();
		sl_size n2 = b.getSize();
		Memory ret = Memory::create(n1 + n2);
		if (ret.isNotNull()) {
			sl_uint8* data = (sl_uint8*)(ret.getData());
			Base::copyMemory(data, a.getData(), n1);
			Base::copyMemory(data + n1, b.getData(), n2);
			return ret;
		}
		return sl_null;
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
