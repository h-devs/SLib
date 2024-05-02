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

#include "slib/core/ref.h"
#include "slib/core/base.h"
#include "slib/core/string_buffer.h"
#include "slib/core/memory_buffer.h"

#define PRIV_SIGNATURE 0x15181289

namespace slib
{

	namespace priv
	{
		namespace ref
		{
			void* const g_null = sl_null;
		}
	}

	CRef::CRef() noexcept: m_nRefCount(0), m_weak(sl_null)
	{
	}

	CRef::CRef(const CRef& other) noexcept: m_nRefCount(0), m_weak(sl_null)
	{
	}

	CRef::CRef(CRef&& other) noexcept: m_nRefCount(0), m_weak(sl_null)
	{
	}

	CRef::~CRef()
	{
		_clearWeak();
	}

	sl_reg CRef::increaseReference(volatile sl_reg& refCount) noexcept
	{
		if (refCount) {
			return Base::interlockedIncrement(&refCount);
		} else {
			refCount = 1;
			return 1;
		}
	}

	sl_reg CRef::decreaseReference(volatile sl_reg& refCount) noexcept
	{
		sl_reg n = refCount;
		if (n == 1) {
			refCount = 0;
			return 0;
		} else if (n > 0) {
			return Base::interlockedDecrement(&refCount);
		} else {
			return -1;
		}
	}

	sl_reg CRef::increaseReference() noexcept
	{
		if (m_nRefCount) {
			return Base::interlockedIncrement(&m_nRefCount);
		} else {
			m_nRefCount = 1;
			init();
			return 1;
		}
	}

	sl_reg CRef::decreaseReference() noexcept
	{
		sl_reg nRef = Base::interlockedDecrement(&m_nRefCount);
		if (!nRef) {
			free();
		}
		return nRef;
	}

	sl_reg CRef::getReferenceCount() noexcept
	{
		return m_nRefCount;
	}

	sl_reg CRef::_decreaseReference() noexcept
	{
		return Base::interlockedDecrement(&m_nRefCount);
	}

	void CRef::init()
	{
	}

	void CRef::free()
	{
		_clearWeak();
		delete this;
	}

	sl_object_type CRef::ObjectType() noexcept
	{
		return 0;
	}

	sl_bool CRef::isDerivedFrom(sl_object_type type) noexcept
	{
		return sl_false;
	}

	sl_object_type CRef::getObjectType() const noexcept
	{
		return 0;
	}

	sl_bool CRef::isInstanceOf(sl_object_type type) const noexcept
	{
		return sl_false;
	}

	String CRef::toString()
	{
		return String::concat("<ref:0x", String::fromPointerValue(this), ">");
	}

	sl_bool CRef::toJsonString(StringBuffer& buf)
	{
		return buf.addStatic("{}");
	}

	sl_bool CRef::toJsonBinary(MemoryBuffer& buf)
	{
		return buf.addStatic("", 1);
	}

	sl_bool CRef::runOperator(sl_uint32 op, Variant& result, const Variant& arg, sl_bool flagThisOnLeft)
	{
		return sl_false;
	}

	sl_bool CRef::_isWeakRef() const noexcept
	{
		return getObjectType() == CWeakRef::ObjectType();
	}

	CWeakRef* CRef::_getWeakObject() noexcept
	{
		if (m_weak) {
			return m_weak;
		}
		SpinLock* locker = SpinLockPoolForWeakRef::get(this);
		locker->lock();
		if (!m_weak) {
			m_weak = CWeakRef::create(this);
		}
		locker->unlock();
		return m_weak;
	}

	void CRef::_clearWeak() noexcept
	{
		if (m_weak) {
			m_weak->release();
			m_weak = sl_null;
		}
	}

	CRef& CRef::operator=(const CRef& other) noexcept
	{
		return *this;
	}

	CRef& CRef::operator=(CRef&& other) noexcept
	{
		return *this;
	}


	SLIB_DEFINE_ROOT_OBJECT(CWeakRef)

	CWeakRef::CWeakRef() noexcept
	{
		m_object = sl_null;
	}

	CWeakRef::~CWeakRef()
	{
	}

	CWeakRef* CWeakRef::create(CRef* object) noexcept
	{
		CWeakRef* ret = new CWeakRef;
		if (ret) {
			ret->m_object = (CRef*)object;
			ret->increaseReference();
		}
		return ret;
	}

	Ref<CRef> CWeakRef::lock() noexcept
	{
		Ref<CRef> ret;
		if (!m_object) {
			return sl_null;
		}
		m_lock.lock();
		CRef* obj = m_object;
		if (obj) {
			sl_reg n = obj->increaseReference();
			if (n > 1) {
				ret.ptr = obj;
			} else {
				obj->_decreaseReference();
			}
		}
		m_lock.unlock();
		return ret;
	}

	void CWeakRef::release() noexcept
	{
		m_lock.lock();
		m_object = sl_null;
		m_lock.unlock();
		decreaseReference();
	}

}
