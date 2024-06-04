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

#ifndef CHECKHEADER_SLIB_CORE_LINKED_LIST
#define CHECKHEADER_SLIB_CORE_LINKED_LIST

#include "list.h"

namespace slib
{

	template <class T>
	struct SLIB_EXPORT Link
	{
		Link<T>* before;
		Link<T>* next;
		T value;
	};

	template <class T>
	class SLIB_EXPORT LinkPosition
	{
	public:
		LinkPosition() noexcept : link(sl_null) {}

		LinkPosition(Link<T>* _link) noexcept : link(_link) {}

		LinkPosition(Link<T>* _link, CRef* _ref) noexcept : link(_link), ref(_ref) {}

		LinkPosition(const LinkPosition& other) = default;

		LinkPosition(LinkPosition&& other) = default;

	public:
		LinkPosition& operator=(const LinkPosition& other) = default;

		LinkPosition& operator=(LinkPosition&& other) = default;

		T& operator*() const noexcept
		{
			return link->value;
		}

		sl_bool operator==(const LinkPosition<T>& p) const noexcept
		{
			return link == p.link;
		}

		sl_bool operator!=(const LinkPosition<T>& p) const noexcept
		{
			return link != p.link;
		}

		operator Link<T>*() const noexcept
		{
			return link;
		}

		LinkPosition<T>& operator++() noexcept
		{
			link = link->next;
			return *this;
		}

	public:
		Link<T>* link;
		Ref<CRef> ref;

	};

	class SLIB_EXPORT CLinkedListBase : public CRef, public Lockable
	{
		SLIB_DECLARE_OBJECT

	public:
		CLinkedListBase();

		~CLinkedListBase();

	};

	template <class T>
	class SLIB_EXPORT CLinkedList : public CLinkedListBase
	{
	protected:
		Link<T>* m_front;
		Link<T>* m_back;
		sl_size m_count;

	public:
		CLinkedList() noexcept
		{
			m_front = sl_null;
			m_back = sl_null;
			m_count = 0;
		}

		~CLinkedList() noexcept
		{
			removeAll_NoLock();
		}

	public:
		CLinkedList(CLinkedList&& other) noexcept
		{
			m_front = other.m_front;
			m_back = other.m_back;
			m_count = other.m_count;
			other.m_front = sl_null;
			other.m_back = sl_null;
			other.m_count = 0;
		}

		CLinkedList& operator=(CLinkedList&& other) noexcept
		{
			m_front = other.m_front;
			m_back = other.m_back;
			m_count = other.m_count;
			other.m_front = sl_null;
			other.m_back = sl_null;
			other.m_count = 0;
			return *this;
		}

	public:
		// not-free existing links
		void initialize() noexcept
		{
			m_front = sl_null;
			m_back = sl_null;
			m_count = 0;
		}

		// not-free existing links
		void initialize(Link<T>* front, Link<T>* back, sl_size count) noexcept
		{
			m_front = front;
			m_back = back;
			m_count = count;
		}

		// not-free existing links
		void initialize(const CLinkedList<T>* other) noexcept
		{
			m_front = other->m_front;
			m_back = other->m_back;
			m_count = other->m_count;
		}

	public:
		Link<T>* getFront() const noexcept
		{
			return m_front;
		}

		Link<T>* getBack() const noexcept
		{
			return m_back;
		}

		sl_size getCount() const noexcept
		{
			return m_count;
		}

		sl_bool isEmpty() const noexcept
		{
			return m_front == sl_null;
		}

		sl_bool isNotEmpty() const noexcept
		{
			return m_front != sl_null;
		}

	public:
		sl_bool getFrontValue_NoLock(T* _out) const noexcept
		{
			if (m_front) {
				if (_out) {
					*_out = m_front->value;
				}
				return sl_true;
			}
			return sl_false;
		}

		sl_bool getFrontValue(T* _out) const noexcept
		{
			ObjectLocker lock(this);
			return getFrontValue_NoLock(_out);
		}

		sl_bool getBackValue_NoLock(T* _out) const noexcept
		{
			if (m_back) {
				if (_out) {
					*_out = m_back->value;
				}
				return sl_true;
			}
			return sl_false;
		}

		sl_bool getBackValue(T* _out) const noexcept
		{
			ObjectLocker lock(this);
			return getBackValue_NoLock(_out);
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		Link<T>* find_NoLock(VALUE&& value, EQUALS&& equals = EQUALS()) const noexcept
		{
			Link<T>* now = m_front;
			while (now) {
				if (equals(now->value, Forward<VALUE>(value))) {
					return now;
				}
				now = now->next;
			}
			return sl_null;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool find(VALUE&& value, EQUALS&& equals = EQUALS()) const noexcept
		{
			ObjectLocker lock(this);
			return find_NoLock(Forward<VALUE>(value), Forward<EQUALS>(equals)) != sl_null;
		}

		template <class... ARGS>
		Link<T>* pushBack_NoLock(ARGS&&... args) noexcept
		{
			Link<T>* item = _createItem(Forward<ARGS>(args)...);
			if (item) {
				_pushBackItem(item);
				return item;
			}
			return sl_null;
		}

		template <class... ARGS>
		sl_bool pushBack(ARGS&&... args) noexcept
		{
			Link<T>* item = _createItem(Forward<ARGS>(args)...);
			if (item) {
				ObjectLocker lock(this);
				_pushBackItem(item);
				return sl_true;
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool pushBackAll_NoLock(const CLinkedList<VALUE>* other) noexcept
		{
			Link<VALUE>* link = other->getFront();
			while (link) {
				if (!(pushBack_NoLock(link->value))) {
					return sl_false;
				}
				link = link->next;
			}
			return sl_true;
		}

		template <class VALUE>
		sl_bool pushBackAll(const CLinkedList<VALUE>* other) noexcept
		{
			MultipleObjectsLocker lock(this, other);
			return pushBackAll_NoLock(other);
		}

		sl_bool popBack_NoLock(T* _out = sl_null) noexcept
		{
			Link<T>* old = _popBackItem();
			if (old) {
				if (_out) {
					*_out = Move(old->value);
				}
				_freeItem(old);
				return sl_true;
			} else {
				return sl_false;
			}
		}

		sl_bool popBack(T* _out = sl_null) noexcept
		{
			Link<T>* old;
			{
				ObjectLocker lock(this);
				old = _popBackItem();
			}
			if (old) {
				if (_out) {
					*_out = Move(old->value);
				}
				_freeItem(old);
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class... ARGS>
		Link<T>* pushFront_NoLock(ARGS&&... args) noexcept
		{
			Link<T>* item = _createItem(Forward<ARGS>(args)...);
			if (item) {
				_pushFrontItem(item);
				return item;
			}
			return sl_null;
		}

		template <class... ARGS>
		sl_bool pushFront(ARGS&&... args) noexcept
		{
			Link<T>* item = _createItem(Forward<ARGS>(args)...);
			if (item) {
				ObjectLocker lock(this);
				_pushFrontItem(item);
				return sl_false;
			}
			return sl_true;
		}

		template <class VALUE>
		sl_bool pushFrontAll_NoLock(const CLinkedList<VALUE>* other) noexcept
		{
			Link<VALUE>* link = other->getBack();
			while (link) {
				if (!(pushFront_NoLock(link->value))) {
					return sl_false;
				}
				link = link->before;
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool pushFrontAll(const CLinkedList<VALUE>* other) noexcept
		{
			MultipleObjectsLocker lock(this, other);
			return pushFrontAll_NoLock(other);
		}

		sl_bool popFront_NoLock(T* _out = sl_null) noexcept
		{
			Link<T>* old = _popFrontItem();
			if (old) {
				if (_out) {
					*_out = Move(old->value);
				}
				_freeItem(old);
				return sl_true;
			} else {
				return sl_false;
			}
		}

		sl_bool popFront(T* _out = sl_null) noexcept
		{
			Link<T>* old;
			{
				ObjectLocker lock(this);
				old = _popFrontItem();
			}
			if (old) {
				if (_out) {
					*_out = Move(old->value);
				}
				_freeItem(old);
				return sl_true;
			} else {
				return sl_false;
			}
		}

		// unsynchronized function
		template <class... ARGS>
		Link<T>* insertBefore(Link<T>* itemWhere, ARGS&&... args) noexcept
		{
			if (itemWhere) {
				Link<T>* itemNew = _createItem(Forward<ARGS>(args)...);
				if (itemNew) {
					_insertBefore(itemWhere, itemNew);
				}
				return itemNew;
			} else {
				return pushBack_NoLock(Forward<ARGS>(args)...);
			}
		}

		// unsynchronized function
		template <class... ARGS>
		Link<T>* insertAfter(Link<T>* itemWhere, ARGS&&... args) noexcept
		{
			if (itemWhere) {
				Link<T>* itemNew = _createItem(Forward<ARGS>(args)...);
				if (itemNew) {
					_insertAfter(itemWhere, itemNew);
				}
				return itemNew;
			} else {
				return pushFront_NoLock(Forward<ARGS>(args)...);
			}
		}

		/* unsynchronized function.*/
		Link<T>* removeAt(Link<T>* item) noexcept
		{
			if (item) {
				Link<T>* next = item->next;
				_removeItem(item);
				_freeItem(item);
				return next;
			} else {
				return sl_null;
			}
		}

		sl_size removeAll_NoLock() noexcept
		{
			Link<T>* front = m_front;
			sl_size count = m_count;
			initialize();
			freeLink(front);
			return count;
		}


		sl_size removeAll() noexcept
		{
			Link<T>* front;
			sl_size count;
			{
				ObjectLocker lock(this);
				front = m_front;
				count = m_count;
				initialize();
			}
			freeLink(front);
			return count;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool remove_NoLock(VALUE&& value, EQUALS&& equals = EQUALS()) noexcept
		{
			Link<T>* now = m_front;
			while (now) {
				Link<T>* next = now->next;
				if (equals(now->value, Forward<VALUE>(value))) {
					removeAt(now);
					return sl_true;
				}
				now = next;
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool remove(VALUE&& value, EQUALS&& equals = EQUALS()) noexcept
		{
			ObjectLocker lock(this);
			return remove_NoLock(Forward<VALUE>(value), Forward<EQUALS>(equals));
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_size removeValues_NoLock(VALUE&& value, EQUALS&& equals = EQUALS()) noexcept
		{
			sl_size n = 0;
			Link<T>* now = m_front;
			while (now) {
				Link<T>* next = now->next;
				if (equals(now->value, Forward<VALUE>(value))) {
					n++;
					removeAt(now);
				}
				now = next;
			}
			return n;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_size removeValues(VALUE&& value, EQUALS&& equals = EQUALS()) noexcept
		{
			ObjectLocker lock(this);
			return removeValues_NoLock(Forward<VALUE>(value), Forward<EQUALS>(equals));
		}

		void merge_NoLock(CLinkedList<T>* other) noexcept
		{
			if (!other) {
				return;
			}
			if (this == other) {
				return;
			}
			Link<T>* _front = other->getFront();
			Link<T>* _back = other->getBack();
			if (_front) {
				sl_size countNew = m_count + other->getCount();
				if (m_back) {
					m_back->next = _front;
					_front->before = m_back;
				} else {
					m_front = _front;
				}
				m_back = _back;
				other->initialize();
				m_count = countNew;
			}
		}

		void merge(CLinkedList<T>* other) noexcept
		{
			if (!other) {
				return;
			}
			if (this == other) {
				return;
			}
			MultipleObjectsLocker lock(this, other);
			merge_NoLock(other);
		}

		Array<T> toArray_NoLock() const noexcept
		{
			Array<T> ret;
			if (m_count) {
				ret = Array<T>::create(m_count);
				if (ret.isNotNull()) {
					T* buf = ret.getData();
					sl_size index = 0;
					Link<T>* now = m_front;
					while (now) {
						buf[index] = now->value;
						now = now->next;
						index++;
					}
				}
			}
			return ret;
		}

		Array<T> toArray() const noexcept
		{
			ObjectLocker lock(this);
			return toArray_NoLock();
		}

		List<T> toList_NoLock() const noexcept
		{
			List<T> ret;
			if (m_count) {
				ret = List<T>::create(m_count);
				if (ret.isNotNull()) {
					ListElements<T> list(ret);
					sl_size index = 0;
					Link<T>* now = m_front;
					while (now) {
						list[index] = now->value;
						now = now->next;
						index++;
					}
				}
			}
			return ret;
		}

		List<T> toList() const noexcept
		{
			ObjectLocker lock(this);
			return toList_NoLock();
		}

		CLinkedList<T>* duplicate_NoLock() const noexcept
		{
			CLinkedList<T>* ret = new CLinkedList<T>;
			if (ret) {
				Link<T>* now = m_front;
				while (now) {
					if (!(ret->pushBack_NoLock(now->value))) {
						delete ret;
						return sl_null;
					}
					now = now->next;
				}
				return ret;
			}
			return sl_null;
		}

		CLinkedList<T>* duplicate() const noexcept
		{
			ObjectLocker lock(this);
			return duplicate_NoLock();
		}

		LinkPosition<T> begin() const noexcept
		{
			return LinkPosition<T>(m_front);
		}

		LinkPosition<T> end() const noexcept
		{
			return LinkPosition<T>();
		}

		static void freeLink(Link<T>* link) noexcept
		{
			while (link) {
				Link<T>* next = link->next;
				_freeItem(link);
				link = next;
			}
		}

	protected:
		template <class... ARGS>
		static Link<T>* _createItem(ARGS&&... args) noexcept
		{
			Link<T>* item = (Link<T>*)(Base::createMemory(sizeof(Link<T>)));
			if (!item) {
				return sl_null;
			}
			item->next = sl_null;
			item->before = sl_null;
			new (&(item->value)) T(Forward<ARGS>(args)...);
			return item;
		}

		static void _freeItem(Link<T>* item) noexcept
		{
			item->value.T::~T();
			Base::freeMemory(item);
		}

		void _pushBackItem(Link<T>* item) noexcept
		{
			if (m_back) {
				m_back->next = item;
				item->before = m_back;
				m_back = item;
			} else {
				m_front = item;
				m_back = item;
			}
			m_count++;
		}

		Link<T>* _popBackItem() noexcept
		{
			Link<T>* back = m_back;
			if (back) {
				m_count--;
				Link<T>* before = back->before;
				if (before) {
					before->next = sl_null;
					m_back = before;
				} else {
					m_front = sl_null;
					m_back = sl_null;
				}
			}
			return back;
		}

		void _pushFrontItem(Link<T>* item) noexcept
		{
			if (m_front) {
				item->next = m_front;
				m_front->before = item;
				m_front = item;
			} else {
				m_front = item;
				m_back = item;
			}
			m_count++;
		}

		Link<T>* _popFrontItem() noexcept
		{
			Link<T>* front = m_front;
			if (front) {
				m_count--;
				Link<T>* next = front->next;
				if (next) {
					next->before = sl_null;
					m_front = next;
				} else {
					m_front = sl_null;
					m_back = sl_null;
				}
			}
			return front;
		}

		void _removeItem(Link<T>* item) noexcept
		{
			m_count--;
			Link<T>* before = item->before;
			Link<T>* next = item->next;
			if (before) {
				before->next = next;
			} else {
				m_front = next;
			}
			if (next) {
				next->before = before;
			} else {
				m_back = before;
			}
		}

		void _insertBefore(Link<T>* itemWhere, Link<T>* itemNew) noexcept
		{
			itemNew->next = itemWhere;
			Link<T>* before = itemWhere->before;
			itemNew->before = before;
			itemWhere->before = itemNew;
			if (before) {
				before->next = itemNew;
			} else {
				m_front = itemNew;
			}
			m_count++;
		}

		void _insertAfter(Link<T>* itemWhere, Link<T>* itemNew) noexcept
		{
			itemNew->before = itemWhere;
			Link<T>* next = itemWhere->next;
			itemNew->next = next;
			itemWhere->next = itemNew;
			if (next) {
				next->before = itemNew;
			} else {
				m_back = itemNew;
			}
			m_count++;
		}

	};

	template <class T>
	class SLIB_EXPORT LinkedList
	{
	public:
		Ref< CLinkedList<T> > ref;
		SLIB_REF_WRAPPER(LinkedList, CLinkedList<T>)

	public:
		static LinkedList<T> create() noexcept
		{
			return new CLinkedList<T>;
		}

	public:
		Link<T>* getFront() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getFront();
			}
			return sl_null;
		}

		Link<T>* getBack() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getBack();
			}
			return sl_null;
		}

		sl_size getCount() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getCount();
			}
			return 0;
		}

		sl_bool isEmpty() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->isEmpty();
			}
			return sl_true;
		}

		sl_bool isNotEmpty() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->isNotEmpty();
			}
			return sl_false;
		}

	public:
		sl_bool getFrontValue_NoLock(T* _out) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getFrontValue_NoLock(_out);
			}
			return sl_false;
		}

		sl_bool getFrontValue(T* _out) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getFrontValue(_out);
			}
			return sl_false;
		}

		sl_bool getBackValue_NoLock(T* _out) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getBackValue_NoLock(_out);
			}
			return sl_false;
		}

		sl_bool getBackValue(T* _out) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getBackValue(_out);
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		Link<T>* find_NoLock(VALUE&& value, EQUALS&& equals = EQUALS()) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->find_NoLock(Forward<VALUE>(value), Forward<EQUALS>(equals));
			}
			return sl_null;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool find(VALUE&& value, EQUALS&& equals = EQUALS()) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->find(Forward<VALUE>(value), Forward<EQUALS>(equals));
			}
			return sl_false;
		}

		template <class... ARGS>
		Link<T>* pushBack_NoLock(ARGS&&... args) noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->pushBack_NoLock(Forward<ARGS>(args)...);
			} else {
				obj = new CLinkedList<T>;
				if (obj) {
					ref = obj;
					return obj->pushBack_NoLock(Forward<ARGS>(args)...);
				}
			}
			return sl_null;
		}

		template <class... ARGS>
		sl_bool pushBack(ARGS&&... args) noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->pushBack(Forward<ARGS>(args)...);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->pushBack(Forward<ARGS>(args)...);
				}
				obj = new CLinkedList<T>;
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->pushBack(Forward<ARGS>(args)...);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool pushBackAll_NoLock(const LinkedList<VALUE>& _other) noexcept
		{
			CLinkedList<T>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->pushBackAll_NoLock(other);
			} else {
				ref = other->duplicate_NoLock();
				return ref.isNotNull();
			}
		}

		template <class VALUE>
		sl_bool pushBackAll(const LinkedList<VALUE>& _other) noexcept
		{
			CLinkedList<T>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->pushBackAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->pushBackAll(other);
				}
				obj = new CLinkedList<T>;
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->pushBackAll(other);
				}
			}
			return sl_false;
		}

		sl_bool popBack_NoLock(T* _out = sl_null) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popBack_NoLock(_out);
			}
			return sl_false;
		}

		sl_bool popBack(T* _out = sl_null) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popBack(_out);
			}
			return sl_false;
		}

		template <class... ARGS>
		Link<T>* pushFront_NoLock(ARGS&&... args) noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->pushFront_NoLock(Forward<ARGS>(args)...);
			} else {
				obj = new CLinkedList<T>;
				if (obj) {
					ref = obj;
					return obj->pushFront_NoLock(Forward<ARGS>(args)...);
				}
			}
			return sl_null;
		}

		template <class... ARGS>
		sl_bool pushFront(ARGS&&... args) noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->pushFront(Forward<ARGS>(args)...);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->pushFront(Forward<ARGS>(args)...);
				}
				obj = new CLinkedList<T>;
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->pushFront(Forward<ARGS>(args)...);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool pushFrontAll_NoLock(const LinkedList<VALUE>& _other) noexcept
		{
			LinkedList<T>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->pushFrontAll_NoLock(other);
			} else {
				ref = other->duplicate_NoLock();
				return ref.isNotNull();
			}
		}

		template <class VALUE>
		sl_bool pushFrontAll(const LinkedList<VALUE>& _other) noexcept
		{
			LinkedList<T>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->pushFrontAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->pushFrontAll(other);
				}
				obj = new CLinkedList<T>;
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->pushFrontAll(other);
				}
			}
			return sl_false;
		}

		sl_bool popFront_NoLock(T* _out = sl_null) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popFront_NoLock(_out);
			}
			return sl_false;
		}

		sl_bool popFront(T* _out = sl_null) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popFront(_out);
			}
			return sl_false;
		}

		// unsynchronized function
		template <class... ARGS>
		Link<T>* insertBefore(Link<T>* itemWhere, ARGS&&... args) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				obj->insertBefore(itemWhere, Forward<ARGS>(args)...);
			}
			return sl_null;
		}

		// unsynchronized function
		template <class... ARGS>
		Link<T>* insertAfter(Link<T>* itemWhere, ARGS&&... args) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				obj->insertAfter(itemWhere, Forward<ARGS>(args)...);
			}
			return sl_null;
		}

		// unsynchronized function
		Link<T>* removeAt(Link<T>* item) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeAt(item);
			}
			return sl_null;
		}

		sl_size removeAll_NoLock() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeAll_NoLock();
			}
			return 0;
		}

		sl_size removeAll() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeAll();
			}
			return 0;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool remove_NoLock(VALUE&& value, EQUALS&& equals = EQUALS()) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->remove_NoLock(Forward<VALUE>(value), Forward<EQUALS>(equals));
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool remove(VALUE&& value, EQUALS&& equals = EQUALS()) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->remove(Forward<VALUE>(value), Forward<EQUALS>(equals));
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_size removeValues_NoLock(VALUE&& value, EQUALS&& equals = EQUALS()) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeValues_NoLock(Forward<VALUE>(value), Forward<EQUALS>(equals));
			}
			return 0;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_size removeValues(VALUE&& value, EQUALS&& equals = EQUALS()) const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeValues(Forward<VALUE>(value), Forward<EQUALS>(equals));
			}
			return 0;
		}

		void merge_NoLock(LinkedList<T>& _other) noexcept
		{
			CLinkedList<T>* other = _other.ref.ptr;
			if (!other) {
				return;
			}
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				obj->merge_NoLock(other);
			} else {
				obj = new CLinkedList<T>;
				if (obj) {
					obj->initialize(other);
					other->initialize();
					ref = obj;
				}
			}
		}

		void merge(LinkedList<T>& _other) noexcept
		{
			CLinkedList<T>* other = _other.ref.ptr;
			if (!other) {
				return;
			}
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				obj->merge(other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					obj->merge(other);
					return;
				}
				obj = new CLinkedList<T>;
				if (obj) {
					ref = obj;
					lock.unlock();
					obj->merge(other);
					return;
				}
			}
		}

		Array<T> toArray_NoLock() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->toArray_NoLock();
			}
			return sl_null;
		}

		Array<T> toArray() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->toArray();
			}
			return sl_null;
		}

		List<T> toList_NoLock() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->toList_NoLock();
			}
			return sl_null;
		}

		List<T> toList() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->toList();
			}
			return sl_null;
		}

		LinkedList<T> duplicate_NoLock() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->duplicate_NoLock();
			}
			return sl_null;
		}

		LinkedList<T> duplicate() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return obj->duplicate();
			}
			return sl_null;
		}

		LinkPosition<T> begin() const noexcept
		{
			CLinkedList<T>* obj = ref.ptr;
			if (obj) {
				return LinkPosition<T>(obj->getFront());
			}
			return LinkPosition<T>();
		}

		LinkPosition<T> end() const noexcept
		{
			return LinkPosition<T>();
		}

	};

	template <class T>
	class SLIB_EXPORT Atomic< LinkedList<T> >
	{
	public:
		AtomicRef< CLinkedList<T> > ref;
		SLIB_ATOMIC_REF_WRAPPER(CLinkedList<T>)

	public:
		template <class... ARGS>
		sl_bool pushBack(ARGS&&... args) noexcept
		{
			Ref< CLinkedList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->pushBack(Forward<ARGS>(args)...);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->pushBack(Forward<ARGS>(args)...);
				}
				obj = new CLinkedList<T>;
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->pushBack(Forward<ARGS>(args)...);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool pushBackAll(const LinkedList<VALUE>& _other) noexcept
		{
			CLinkedList<T>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			Ref< CLinkedList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->pushBackAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->pushBackAll(other);
				}
				obj = new CLinkedList<T>;
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->pushBackAll(other);
				}
			}
			return sl_false;
		}

		template <class... ARGS>
		sl_bool pushFront(ARGS&&... args) noexcept
		{
			Ref< CLinkedList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->pushFront(Forward<ARGS>(args)...);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->pushFront(Forward<ARGS>(args)...);
				}
				obj = new CLinkedList<T>;
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->pushFront(Forward<ARGS>(args)...);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool pushFrontAll(const LinkedList<VALUE>& _other) noexcept
		{
			CLinkedList<T>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			Ref< CLinkedList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->pushFrontAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->pushFrontAll(other);
				}
				obj = new CLinkedList<T>;
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->pushFrontAll(other);
				}
			}
			return sl_false;
		}

		void merge(LinkedList<T>& _other) noexcept
		{
			CLinkedList<T>* other = _other.ref.ptr;
			if (!other) {
				return;
			}
			Ref< CLinkedList<T> > obj(ref);
			if (obj.isNotNull()) {
				obj->merge(other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					obj->merge(other);
					return;
				}
				obj = new CLinkedList<T>;
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					obj->merge(other);
					return;
				}
			}
		}

	};

	template <class T>
	using AtomicLinkedList = Atomic< LinkedList<T> >;

}

#endif
