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

#include "slib/core/collection.h"

#include "slib/core/variant.h"
#include "slib/core/linked_list.h"
#include "slib/core/queue.h"
#include "slib/core/queue_channel.h"
#include "slib/core/linked_object.h"
#include "slib/core/loop_queue.h"
#include "slib/core/string_buffer.h"
#include "slib/core/serialize.h"

namespace slib
{

	SLIB_DEFINE_ROOT_OBJECT(Collection)

	Collection::Collection()
	{
	}

	Collection::~Collection()
	{
	}

	sl_uint64 Collection::getElementsCount()
	{
		return 0;
	}

	Variant Collection::getElement(sl_uint64 index)
	{
		return Variant();
	}

	sl_bool Collection::setElement(sl_uint64 index, const Variant& item)
	{
		return sl_false;
	}

	sl_bool Collection::addElement(const Variant& item)
	{
		return sl_false;
	}

	String Collection::toString()
	{
		StringBuffer buf;
		if (toJsonString(buf)) {
			return buf.merge();
		}
		return sl_null;
	}

	sl_bool Collection::toJsonString(StringBuffer& buf)
	{
		sl_uint64 n = getElementsCount();
		if (!(buf.addStatic("["))) {
			return sl_false;
		}
		for (sl_uint64 i = 0; i < n; i++) {
			Variant v = getElement(i);
			if (i) {
				if (!(buf.addStatic(", "))) {
					return sl_false;
				}
			}
			if (!(v.toJsonString(buf))) {
				return sl_false;
			}
		}
		if (!(buf.addStatic("]"))) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool Collection::toJsonBinary(MemoryBuffer& buf)
	{
		if (!(SerializeByte(&buf, (sl_uint8)(VariantType::Collection)))) {
			return sl_false;
		}
		sl_uint64 n = getElementsCount();
		if (!(CVLI::serialize(&buf, n))) {
			return sl_false;
		}
		for (sl_uint64 i = 0; i < n; i++) {
			if (!(Serialize(&buf, getElement(i)))) {
				return sl_false;
			}
		}
		return sl_true;
	}


	SLIB_DEFINE_ROOT_OBJECT(CArrayBase)
	
	CArrayBase::CArrayBase()
	{
	}

	CArrayBase::~CArrayBase()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(CMapBase)

	CMapBase::CMapBase()
	{
	}

	CMapBase::~CMapBase()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(CHashMapBase)

	CHashMapBase::CHashMapBase()
	{
	}

	CHashMapBase::~CHashMapBase()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(CLinkedListBase)

	CLinkedListBase::CLinkedListBase()
	{
	}

	CLinkedListBase::~CLinkedListBase()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(LinkedObjectListBase)

	LinkedObjectListBase::LinkedObjectListBase()
	{
	}

	LinkedObjectListBase::~LinkedObjectListBase()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(LoopQueueBase)

	LoopQueueBase::LoopQueueBase()
	{
	}

	LoopQueueBase::~LoopQueueBase()
	{
	}

}


#include "slib/core/ptr.h"
#include "slib/core/shared_ptr.h"

namespace slib
{
	namespace priv
	{
		namespace ptr
		{
			
			struct ConstStruct
			{
				void* ptr;
				void* ref;
			};
			
			const ConstStruct g_null = {0, 0};
			void* const g_shared_null = 0;

		}
	}

	CSharedPtrBase::~CSharedPtrBase()
	{
	}

	sl_reg CSharedPtrBase::increaseReference() noexcept
	{
		return Base::interlockedIncrement(&refCount);
	}

	sl_reg CSharedPtrBase::decreaseReference() noexcept
	{
		sl_reg nRef = Base::interlockedDecrement(&refCount);
		if (nRef == 0) {
			delete this;
		}
		return nRef;
	}

}


#include "slib/core/function.h"

namespace slib
{
	
	SLIB_DEFINE_ROOT_OBJECT(CallableBase)

	CallableBase::CallableBase()
	{
	}

	CallableBase::~CallableBase()
	{
	}
	
	namespace priv
	{
		namespace function_list
		{
			sl_object_type GetObjectType()
			{
				return (sl_object_type)(sl_size)(object_types::FunctionList);
			}
		}
	}
	
}


#include "slib/core/promise.h"

namespace slib
{

	SLIB_DEFINE_ROOT_OBJECT(CPromiseBase)

	CPromiseBase::CPromiseBase()
	{
	}

	CPromiseBase::~CPromiseBase()
	{
	}

}
