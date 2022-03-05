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

	Referable::Referable() noexcept: m_nRefCount(0), m_weak(sl_null)
	{
	}

	Referable::Referable(const Referable& other) noexcept: m_nRefCount(0), m_weak(sl_null)
	{
	}
	
	Referable::Referable(Referable&& other) noexcept: m_nRefCount(0), m_weak(sl_null)
	{
	}

	Referable::~Referable()
	{
		_clearWeak();
	}

	sl_reg Referable::increaseReference() noexcept
	{
		sl_reg nRef = Base::interlockedIncrement(&m_nRefCount);
		if (nRef == 1) {
			init();
		}
		return nRef;
	}

	sl_reg Referable::decreaseReference() noexcept
	{
		sl_reg nRef = Base::interlockedDecrement(&m_nRefCount);
		if (nRef == 0) {
			_free();
		}
		return nRef;
	}
	
	sl_reg Referable::getReferenceCount() noexcept
	{
		return m_nRefCount;
	}

	sl_reg Referable::decreaseReferenceNoFree() noexcept
	{
		if (m_nRefCount > 0) {
			return Base::interlockedDecrement(&m_nRefCount);
		}
		return 1;
	}

	void Referable::init()
	{
	}
	
	sl_object_type Referable::ObjectType() noexcept
	{
		return 0;
	}
	
	sl_bool Referable::isDerivedFrom(sl_object_type type) noexcept
	{
		return sl_false;
	}

	sl_object_type Referable::getObjectType() const noexcept
	{
		return 0;
	}

	sl_bool Referable::isInstanceOf(sl_object_type type) const noexcept
	{
		return sl_false;
	}

	String Referable::toString()
	{
		return String::concat("<ref:0x", String::fromPointerValue(this), ">");
	}

	sl_bool Referable::toJsonString(StringBuffer& buf)
	{
		return buf.addStatic("{}");
	}

	sl_bool Referable::toJsonBinary(MemoryBuffer& buf)
	{
		return buf.addStatic("", 1);
	}

	sl_bool Referable::runOperator(sl_uint32 op, Variant& result, const Variant& arg, sl_bool flagThisOnLeft)
	{
		return sl_false;
	}

	sl_bool Referable::_isWeakRef() const noexcept
	{
		return getObjectType() == CWeakRef::ObjectType();
	}

	CWeakRef* Referable::_getWeakObject() noexcept
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

	void Referable::_clearWeak() noexcept
	{
		if (m_weak) {
			m_weak->release();
			m_weak = sl_null;
		}
	}

	void Referable::_free() noexcept
	{
		_clearWeak();
		delete this;
	}
	
	Referable& Referable::operator=(const Referable& other) noexcept
	{
		return *this;
	}
	
	Referable& Referable::operator=(Referable&& other) noexcept
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

	CWeakRef* CWeakRef::create(Referable* object) noexcept
	{
		CWeakRef* ret = new CWeakRef;
		if (ret) {
			ret->m_object = (Referable*)object;
			ret->increaseReference();
		}
		return ret;
	}

	Ref<Referable> CWeakRef::lock() noexcept
	{
		Ref<Referable> ret;
		if (!m_object) {
			return sl_null;
		}
		m_lock.lock();
		Referable* obj = m_object;
		if (obj) {
			sl_reg n = obj->increaseReference();
			if (n > 1) {
				ret = obj;
			}
			obj->decreaseReferenceNoFree();
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
