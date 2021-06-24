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

#include "slib/core/list.h"
#include "slib/core/iterator.h"
#include "slib/core/map.h"
#include "slib/core/hash_map.h"
#include "slib/core/linked_list.h"
#include "slib/core/queue.h"
#include "slib/core/queue_channel.h"
#include "slib/core/linked_object.h"
#include "slib/core/loop_queue.h"
#include "slib/core/ptr.h"
#include "slib/core/shared_ptr.h"
#include "slib/core/function.h"
#include "slib/core/promise.h"

namespace slib
{

	#define PRIV_SLIB_LIST_CAPACITY_MIN 5

	namespace priv
	{
		namespace list
		{

			sl_bool setCapacity(void* _pData, sl_size elementSize, sl_size* pCapacity, sl_size *pCount, sl_size capacity) noexcept
			{
				void** pData = reinterpret_cast<void**>(_pData);
				if (capacity < *pCount) {
					capacity = *pCount;
				}
				if (capacity == *pCapacity) {
					return sl_true;
				}
				void* data = *pData;
				if (data) {
					data = Base::reallocMemory(data, capacity * elementSize);
				} else {
					data = Base::createMemory(capacity * elementSize);
				}
				if (data) {
					*pData = data;
					*pCapacity = capacity;
					return sl_true;
				}
				return sl_false;
			}

			sl_bool adjustCapacity(void* _pData, sl_size elementSize, sl_size* pCapacity, sl_size* pCount, sl_size count) noexcept
			{
				void** pData = reinterpret_cast<void**>(_pData);
				if (count < *pCount) {
					count = *pCount;
				}
				if (*pCapacity < count) {
					sl_size newCapacity = *pCapacity * 3 / 2 + 1;
					if (newCapacity < count) {
						newCapacity = count;
					}
					if (newCapacity < PRIV_SLIB_LIST_CAPACITY_MIN) {
						newCapacity = PRIV_SLIB_LIST_CAPACITY_MIN;
					}
					void* newData;
					if (*pData) {
						newData = Base::reallocMemory(*pData, newCapacity * elementSize);
					} else {
						newData = Base::createMemory(newCapacity * elementSize);
					}
					if (newData) {
						*pData = newData;
						*pCapacity = newCapacity;
					} else {
						return sl_false;
					}
				} else if (*pCapacity > PRIV_SLIB_LIST_CAPACITY_MIN && count < *pCapacity / 2) {
					sl_size newCapacity = count * 3 / 2 + 1;
					if (newCapacity < PRIV_SLIB_LIST_CAPACITY_MIN) {
						newCapacity = PRIV_SLIB_LIST_CAPACITY_MIN;
					}
					if (newCapacity < *pCapacity) {
						void* newData = Base::reallocMemory(*pData, newCapacity * elementSize);
						if (newData) {
							*pData = newData;
							*pCapacity = newCapacity;
						}
					}
				}
				return sl_true;
			}
			
			sl_bool growCapacity(void* _pData, sl_size elementSize, sl_size* pCapacity, sl_size count) noexcept
			{
				if (*pCapacity < count) {
					void** pData = reinterpret_cast<void**>(_pData);
					sl_size newCapacity = *pCapacity * 3 / 2 + 1;
					if (newCapacity < count) {
						newCapacity = count;
					}
					if (newCapacity < PRIV_SLIB_LIST_CAPACITY_MIN) {
						newCapacity = PRIV_SLIB_LIST_CAPACITY_MIN;
					}
					void* newData;
					if (*pData) {
						newData = Base::reallocMemory(*pData, newCapacity * elementSize);
					} else {
						newData = Base::createMemory(newCapacity * elementSize);
					}
					if (newData) {
						*pData = newData;
						*pCapacity = newCapacity;
					} else {
						return sl_false;
					}
				}
				return sl_true;
			}
			
			sl_bool shrinkCapacity(void* _pData, sl_size elementSize, sl_size* pCapacity, sl_size count) noexcept
			{
				if (*pCapacity > PRIV_SLIB_LIST_CAPACITY_MIN && count < *pCapacity / 2) {
					void** pData = reinterpret_cast<void**>(_pData);
					sl_size newCapacity = count * 3 / 2 + 1;
					if (newCapacity < PRIV_SLIB_LIST_CAPACITY_MIN) {
						newCapacity = PRIV_SLIB_LIST_CAPACITY_MIN;
					}
					if (newCapacity < *pCapacity) {
						void* newData = Base::reallocMemory(*pData, newCapacity * elementSize);
						if (newData) {
							*pData = newData;
							*pCapacity = newCapacity;
						}
					}
				}
				return sl_true;
			}
			
		}

		namespace ptr
		{

			struct ConstStruct
			{
				void* ptr;
				void* ref;
			};

			const ConstStruct g_null = { 0, 0 };

		}

		namespace shared
		{
			void* const g_shared_null = 0;
		}

		namespace function_list
		{
			sl_object_type GetObjectType()
			{
				return (sl_object_type)(sl_size)(object_types::FunctionList);
			}
		}

	}


	SLIB_DEFINE_ROOT_OBJECT(CListBase)
	
	CListBase::CListBase()
	{
	}

	CListBase::~CListBase()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(CIteratorBase)

	CIteratorBase::CIteratorBase()
	{
	}

	CIteratorBase::~CIteratorBase()
	{
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


	CSharedPtrBase::~CSharedPtrBase()
	{
	}

	sl_reg CSharedPtrBase::increaseReference() noexcept
	{
		return Base::interlockedIncrement(&refCount);
	}

	sl_reg CSharedPtrBase::decreaseReference()
	{
		sl_reg nRef = Base::interlockedDecrement(&refCount);
		if (!nRef) {
			delete this;
		}
		return nRef;
	}


	SLIB_DEFINE_ROOT_OBJECT(CallableBase)

	CallableBase::CallableBase()
	{
	}

	CallableBase::~CallableBase()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(CPromiseBase)

	CPromiseBase::CPromiseBase()
	{
	}

	CPromiseBase::~CPromiseBase()
	{
	}

}
