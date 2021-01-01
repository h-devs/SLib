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

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MemoryData)

	MemoryData::MemoryData() noexcept : data(sl_null), size(0)
	{
	}

	Memory MemoryData::getMemory() const noexcept
	{
		if (CMemory* mem = CastInstance<CMemory>(refer.ptr)) {
			if (mem->getData() == data && mem->getSize() == size) {
				return mem;
			}
		}
		return Memory::createStatic(data, size, refer.ptr);
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
		return Memory::createStatic((sl_uint8*)data + offset, sizeSub, refer.ptr);
	}


	SLIB_DEFINE_ROOT_OBJECT(CMemory)

	CMemory::CMemory() noexcept
	{
	}

	CMemory::CMemory(const void* data, sl_size size, Referable* refer, sl_bool flagStatic) noexcept: m_refer(refer)
	{
		m_data = (void*)(data);
		m_size = size;
		m_flagStatic = flagStatic;
	}

	CMemory::~CMemory() noexcept
	{
		if (!m_flagStatic) {
			void* data = m_data;
			if (data) {
				Base::freeMemory((void*)data);
			}
		}
	}

	CMemory* CMemory::create(const void* data, sl_size size, Referable* refer, sl_bool flagStatic) noexcept
	{
		if (data && size) {
			return new CMemory(data, size, refer, flagStatic);
		}
		return sl_null;
	}

	CMemory* CMemory::create(sl_size size) noexcept
	{
		if (size) {
			sl_uint8* mem = new sl_uint8[sizeof(CMemory) + size];
			if (mem) {
				CMemory* ret = (CMemory*)mem;
				new (ret) CMemory();
				ret->m_data = mem + sizeof(CMemory);
				ret->m_size = size;
				ret->m_flagStatic = sl_true;
				return ret;
			}
		}
		return sl_null;
	}

	CMemory* CMemory::create(const void* data, sl_size size) noexcept
	{
		CMemory* ret = create(size);
		if (ret) {
			Base::copyMemory(ret->m_data, data, size);
			return ret;
		}
		return sl_null;
	}

	CMemory* CMemory::createResizable(sl_size size) noexcept
	{
		if (size) {
			void* mem = Base::createMemory(size);
			if (mem) {
				CMemory* ret = new CMemory;
				if (ret) {
					ret->m_data = mem;
					ret->m_size = size;
					ret->m_flagStatic = sl_false;
					return ret;
				}
				Base::freeMemory(mem);
			}
		}
		return sl_null;
	}

	CMemory* CMemory::createResizable(const void* data, sl_size size) noexcept
	{
		CMemory* ret = createResizable(size);
		if (ret) {
			Base::copyMemory(ret->m_data, data, size);
			return ret;
		}
		return sl_null;
	}

	CMemory* CMemory::createNoCopy(const void* data, sl_size size) noexcept
	{
		if (data && size) {
			CMemory* ret = new CMemory;
			if (ret) {
				ret->m_data = (void*)data;
				ret->m_size = size;
				ret->m_flagStatic = sl_false;
				return ret;
			}
		}
		return sl_null;
	}

	CMemory* CMemory::createStatic(const void* data, sl_size size, Referable* refer) noexcept
	{
		if (data && size) {
			return new CMemory(data, size, refer, sl_true);
		}
		return sl_null;
	}

	void* CMemory::getData() const noexcept
	{
		return m_data;
	}

	sl_size CMemory::getSize() const noexcept
	{
		return m_size;
	}

	sl_bool CMemory::setSize(sl_size size) noexcept
	{
		if (!m_flagStatic) {
			void* data = m_data;
			if (data) {
				if (size) {
					data = Base::reallocMemory(data, size);
					if (data) {
						m_data = data;
						m_size = size;
						return sl_true;
					}
				} else {
					Base::freeMemory(data);
					m_data = sl_null;
					m_size = 0;
					m_refer.setNull();
					m_flagStatic = sl_true;
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool CMemory::isStatic() const noexcept
	{
		return m_flagStatic;
	}

	const Ref<Referable>& CMemory::getRefer() const noexcept
	{
		return m_refer;
	}

	CMemory* CMemory::sub(sl_size offset, sl_size size) const noexcept
	{
		sl_size sizeParent = m_size;
		if (offset < sizeParent) {
			sl_size limit = sizeParent - offset;
			if (size > limit) {
				size = limit;
			}
			if (sizeParent == size) {
				return (CMemory*)this;
			}
			if (size) {
				if (m_flagStatic) {
					return createStatic((sl_uint8*)m_data + offset, size, m_refer.ptr);
				} else {
					return createStatic((sl_uint8*)m_data + offset, size, (Referable*)this);
				}
			}
		}
		return sl_null;
	}

	sl_size CMemory::read(sl_size offsetSource, sl_size size, void* dst) const noexcept
	{
		sl_uint8* pSrc = (sl_uint8*)m_data;
		sl_uint8* pDst = (sl_uint8*)dst;
		if (pDst && pSrc) {
			sl_size sizeSrc = m_size;
			if (offsetSource < sizeSrc) {
				sl_size n = sizeSrc - offsetSource;
				if (size > n) {
					size = n;
				}
				Base::copyMemory(pDst, pSrc + offsetSource, size);
				return size;
			}
		}
		return 0;
	}

	sl_size CMemory::write(sl_size offsetTarget, sl_size size, const void* src) const noexcept
	{
		sl_uint8* pDst = (sl_uint8*)m_data;
		sl_uint8* pSrc = (sl_uint8*)src;
		if (pSrc && pDst) {
			sl_size sizeTarget = m_size;
			if (offsetTarget < sizeTarget) {
				sl_size n = sizeTarget - offsetTarget;
				if (size > n) {
					size = n;
				}
				Base::copyMemory(pDst + offsetTarget, pSrc, size);
				return size;
			}
		}
		return 0;
	}

	sl_size CMemory::copy(sl_size offsetTarget, const CMemory* source, sl_size offsetSource, sl_size size) const noexcept
	{
		if (source) {
			sl_uint8* pSrc = (sl_uint8*)(source->getData());
			if (pSrc) {
				sl_size sizeSrc = source->getSize();
				if (offsetSource < sizeSrc) {
					sl_size n = sizeSrc - offsetSource;
					if (size > n) {
						size = n;
					}
					return write(offsetSource, size, pSrc + offsetSource);
				}
			}
		}
		return 0;
	}

	CMemory* CMemory::duplicate() const noexcept
	{
		return create(m_data, m_size);
	}


	Memory Memory::create(const void* buf, sl_size size, Referable* refer, sl_bool flagStatic) noexcept
	{
		return CMemory::create(buf, size, refer, flagStatic);
	}

	Memory Memory::create(sl_size size) noexcept
	{
		return CMemory::create(size);
	}

	Memory Memory::create(const void* buf, sl_size size) noexcept
	{
		return CMemory::create(buf, size);
	}

	Memory Memory::createResizable(sl_size size) noexcept
	{
		return CMemory::createResizable(size);
	}

	Memory Memory::createResizable(const void* buf, sl_size size) noexcept
	{
		return CMemory::createResizable(buf, size);
	}

	Memory Memory::createNoCopy(const void* buf, sl_size size) noexcept
	{
		return CMemory::createNoCopy(buf, size);
	}

	Memory Memory::createStatic(const void* buf, sl_size size, Referable* refer) noexcept
	{
		return CMemory::createStatic(buf, size, refer);
	}

	void* Memory::getData() const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->getData();
		}
		return sl_null;
	}

	sl_size Memory::getSize() const noexcept
	{
		CMemory* obj = ref.ptr;
		if (obj) {
			return obj->getSize();
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
				ref = CMemory::create(size);
				return ref.isNotNull();
			} else {
				return sl_true;
			}
		}
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
			data.data = obj->getData();
			data.size = obj->getSize();
			if (obj->isStatic()) {
				data.refer = obj->getRefer();
			} else {
				data.refer = obj;
			}
			return sl_true;
		} else {
			data.data = sl_null;
			data.size = 0;
			data.refer.setNull();
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


	sl_size Atomic<Memory>::getSize() const noexcept
	{
		Ref<CMemory> obj(ref);
		if (obj.isNotNull()) {
			return obj->getSize();
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
	
	
	sl_bool operator==(const Memory& a, const Memory& b) noexcept
	{
		return a.equals(b);
	}
	
	sl_bool operator!=(const Memory& a, const Memory& b) noexcept
	{
		return !(a.equals(b));
	}
	
	sl_bool operator>=(const Memory& a, const Memory& b) noexcept
	{
		return a.compare(b) >= 0;
	}
	
	sl_bool operator>(const Memory& a, const Memory& b) noexcept
	{
		return a.compare(b) > 0;
	}
	
	sl_bool operator<=(const Memory& a, const Memory& b) noexcept
	{
		return a.compare(b) <= 0;
	}
	
	sl_bool operator<(const Memory& a, const Memory& b) noexcept
	{
		return a.compare(b) < 0;
	}
	
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
	
	sl_compare_result Compare<Memory>::operator()(const Memory& a, const Memory& b) const noexcept
	{
		return a.compare(b);
	}

	sl_bool Equals<Memory>::operator()(const Memory& a, const Memory& b) const noexcept
	{
		return a.equals(b);
	}

	sl_size Hash<Memory>::operator()(const Memory& a) const noexcept
	{
		return a.getHashCode();
	}


	SLIB_DEFINE_ROOT_OBJECT(MemoryBuffer)

	MemoryBuffer::MemoryBuffer()
	{
		m_size = 0;
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
		if (mem.size == 0) {
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
	
	sl_bool MemoryBuffer::add(const Memory& mem)
	{
		MemoryData data;
		if (mem.getData(data)) {
			return add(data);
		}
		return sl_true;
	}
	
	sl_bool MemoryBuffer::addStatic(const void* buf, sl_size size)
	{
		if (size == 0) {
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


	SLIB_DEFINE_OBJECT(MemoryQueue, Object)

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
		if (mem.size == 0) {
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
	
	sl_bool MemoryQueue::add(const MemoryData& mem)
	{
		if (mem.size == 0) {
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
	
	sl_bool MemoryQueue::add_NoLock(const Memory& mem)
	{
		MemoryData data;
		if (mem.getData(data)) {
			return add_NoLock(data);
		}
		return sl_true;
	}
	
	sl_bool MemoryQueue::add(const Memory& mem)
	{
		MemoryData data;
		if (mem.getData(data)) {
			return add(data);
		}
		return sl_true;
	}
	
	sl_bool MemoryQueue::addStatic_NoLock(const void* buf, sl_size size)
	{
		if (size == 0) {
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
		if (size == 0) {
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
		MemoryData mem = m_memCurrent;
		if (mem.size > 0) {
			sl_size pos = m_posCurrent;
			m_memCurrent.size = 0;
			m_posCurrent = 0;
			if (pos < mem.size) {
				data.data = (sl_uint8*)(mem.data) + pos;
				data.size = mem.size - pos;
				data.refer = mem.refer;
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
