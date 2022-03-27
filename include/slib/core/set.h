/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_SET
#define CHECKHEADER_SLIB_CORE_SET

#include "map.h"

namespace slib
{
	
	template <class T, class COMPARE>
	class CSet;
	
	template <class T, class COMPARE>
	class Set;
	
	template < class T, class COMPARE = Compare<T> >
	using AtomicSet = Atomic< Set<T, COMPARE> >;
	
	
	template <class NODE>
	class SLIB_EXPORT SetPosition
	{
	public:
		SLIB_CONSTEXPR SetPosition(): node(sl_null) {}
		
		SLIB_CONSTEXPR SetPosition(sl_null_t): node(sl_null) {}
		
		SLIB_CONSTEXPR SetPosition(NODE* other): node(other) {}
		
		SLIB_CONSTEXPR SetPosition(const SetPosition& other): node(other.node) {}
		
	public:
		SetPosition& operator=(const SetPosition& other) noexcept
		{
			node = other.node;
			return *this;
		}
		
		SetPosition& operator=(NODE* other) noexcept
		{
			node = other;
			return *this;
		}
		
		typename NODE::KEY_TYPE& operator*() const noexcept
		{
			return node->key;
		}
		
		SLIB_CONSTEXPR sl_bool operator==(const SetPosition& other) const
		{
			return node == other.node;
		}
		
		SLIB_CONSTEXPR sl_bool operator==(const NODE* other) const
		{
			return node == other;
		}
		
		SLIB_CONSTEXPR sl_bool operator!=(const SetPosition& other) const
		{
			return node != other.node;
		}
		
		SLIB_CONSTEXPR sl_bool operator!=(const NODE* other) const
		{
			return node != other;
		}
		
		SetPosition& operator++() noexcept
		{
			node = node->getNext();
			return *this;
		}
		
	public:
		NODE* node;
		
	};

	template < class T, class COMPARE = Compare<T> >
	class SLIB_EXPORT CSet : public CMap<T, sl_bool, COMPARE>
	{
	public:
		typedef CMap<T, sl_bool, COMPARE> CMAP;
		typedef T VALUE_TYPE;
		typedef MapNode<T, sl_bool> NODE;
		typedef SetPosition<NODE> POSITION;

	public:
		CSet() noexcept {}

		CSet(const COMPARE& compare) noexcept: CMAP(compare) {}
		
		CSet(COMPARE&& compare) noexcept: CMAP(Move(compare)) {}
		
	public:
		CSet(const CSet& other) = delete;
		
		CSet& operator=(const CSet& other) = delete;
		
		CSet(CSet&& other) = default;
		
		CSet& operator=(CSet&& other) = default;
		
#ifdef SLIB_SUPPORT_STD_TYPES
		template <class COMPARE_ARG>
		CSet(const std::initializer_list<T>& l, COMPARE_ARG&& compare) noexcept: CMAP(Forward<COMPARE_ARG>(compare))
		{
			const T* data = l.begin();
			sl_size n = l.size();
			for (sl_size i = 0; i < n; i++) {
				CMAP::add_NoLock(data[i], sl_true);
			}
		}

		CSet(const std::initializer_list<T>& l) noexcept: CSet(l, COMPARE()) {}
#endif
		
	public:
		template <class VALUE>
		NODE* put_NoLock(VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			return CMAP::put_NoLock(Forward<VALUE>(value), sl_true, isInsertion);
		}

		template <class VALUE>
		sl_bool put(VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			return CMAP::put(Forward<VALUE>(value), isInsertion);
		}
		
		template <class VALUE>
		NODE* add_NoLock(VALUE&& value) noexcept
		{
			return CMAP::add_NoLock(Forward<VALUE>(value), sl_true);
		}
		
		template <class VALUE>
		NODE* add(VALUE&& value) noexcept
		{
			return CMAP::add(Forward<VALUE>(value), sl_true);
		}
		
		sl_bool remove_NoLock(const T& value) noexcept
		{
			return CMAP::remove_NoLock(value);
		}

		sl_bool remove(const T& value) noexcept
		{
			return CMAP::remove(value);
		}
		
		sl_size removeItems_NoLock(const T& value) noexcept
		{
			return CMAP::removeItem_NoLock(value);
		}

		sl_size removeItems(const T& value) noexcept
		{
			return CMAP::removeItem(value);
		}
		
		CSet* duplicate_NoLock() const noexcept
		{
			return (CSet*)(CMAP::duplicate_NoLock());
		}

		CSet* duplicate() const noexcept
		{
			return (CSet*)(CMAP::duplicate());
		}
		
		List<T> toList_NoLock() const noexcept
		{
			return CMAP::getAllKeys_NoLock();
		}

		List<T> toList() const noexcept
		{
			return CMAP::getAllKeys();
		}

		// range-based for loop
		POSITION begin() const noexcept
		{
			return CMAP::getFirstNode();
		}

		POSITION end() const noexcept
		{
			return sl_null;
		}

	};


	template < class T, class COMPARE = Compare<T> >
	class SLIB_EXPORT Set
	{
	public:
		typedef CSet<T, COMPARE> CSET;
		typedef T VALUE_TYPE;
		typedef Map<T, sl_bool, COMPARE> MAP_TYPE;
		typedef MapNode<T, sl_bool> NODE;
		typedef SetPosition<NODE> POSITION;
		typedef typename MapBaseHelper::EnumLockHelper<Set> EnumLockHelper;
		typedef typename MapBaseHelper::EnumHelper<Set> EnumHelper;

	public:
		Ref<CSET> ref;
		SLIB_REF_WRAPPER(Set, CSET)
		
	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		Set(const std::initializer_list<T>& l) noexcept: ref(new CSET(l)) {}

		template <class COMPARE_ARG>
		Set(const std::initializer_list<T>& l, COMPARE_ARG&& compare) noexcept: ref(new CSET(l, Forward<COMPARE_ARG>(compare))) {}
#endif
		
	public:
		static Set create() noexcept
		{
			return new CSET();
		}
		
		static Set create(const COMPARE& compare) noexcept
		{
			return new CSET(compare);
		}
		
		static Set create(COMPARE&& compare) noexcept
		{
			return new CSET(Move(compare));
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		static Set create(const std::initializer_list<T>& l) noexcept
		{
			return new CSET(l);
		}

		template <class COMPARE_ARG>
		static Set create(const std::initializer_list<T>& l, COMPARE_ARG&& compare) noexcept
		{
			return new CSET(l, Forward<COMPARE_ARG>(compare));
		}
#endif
		
		template <class TYPE, class COMPARE_ARG>
		static const Set& from(const Set<TYPE, COMPARE_ARG>& other) noexcept
		{
			return *(reinterpret_cast<Set const*>(&other));
		}
		
		void initialize() noexcept
		{
			ref = new CSET();
		}
		
		void initialize(const COMPARE& compare) noexcept
		{
			ref = new CSET(compare);
		}

		void initialize(COMPARE&& compare) noexcept
		{
			ref = new CSET(Move(compare));
		}

	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		Set& operator=(const std::initializer_list<T>& l) noexcept
		{
			ref = new CSET(l);
			return *this;
		}
#endif

	public:
		sl_size getCount() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->getCount();
			}
			return 0;
		}

		sl_bool isEmpty() const noexcept
		{
			return !(getCount());
		}

		sl_bool isNotEmpty() const noexcept
		{
			return getCount() != 0;
		}

		NODE* find_NoLock(const T& value) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->find_NoLock(value);
			}
			return sl_null;
		}

		sl_bool find(const T& value) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->find(value);
			}
			return sl_false;
		}
		
		template <class VALUE>
		NODE* put_NoLock(VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			return ((MAP_TYPE*)((void*)this))->put_NoLock(Forward<VALUE>(value), sl_true, isInsertion);
		}

		template <class VALUE>
		sl_bool put(VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			return ((MAP_TYPE*)((void*)this))->put(Forward<VALUE>(value), sl_true, isInsertion);
		}
		
		template <class VALUE>
		NODE* add_NoLock(VALUE&& value) noexcept
		{
			return ((MAP_TYPE*)((void*)this))->add_NoLock(Forward<VALUE>(value), sl_true);
		}

		template <class VALUE>
		sl_bool add(VALUE&& value) noexcept
		{
			return ((MAP_TYPE*)((void*)this))->add(Forward<VALUE>(value), sl_true);
		}

		template <class MAP>
		sl_bool putAll_NoLock(const MAP& other) noexcept
		{
			return ((MAP_TYPE*)((void*)this))->putAll_NoLock(other);
		}

		template <class MAP>
		sl_bool putAll(const MAP& other) noexcept
		{
			return ((MAP_TYPE*)((void*)this))->putAll(other);
		}

		template <class MAP>
		sl_bool addAll_NoLock(const MAP& other) noexcept
		{
			return ((MAP_TYPE*)((void*)this))->addAll_NoLock(other);
		}

		template <class MAP>
		sl_bool addAll(const MAP& other) noexcept
		{
			return ((MAP_TYPE*)((void*)this))->addAll(other);
		}

		/* unsynchronized function */
		void removeAt(NODE* node) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				obj->removeAt(node);
			}
		}

		/* unsynchronized function */
		sl_size removeAt(NODE* node, sl_size count) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->removeAt(node, count);
			}
			return 0;
		}

		/* unsynchronized function */
		sl_size removeRange(NODE* first, NODE* last) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->removeRange(first, last);
			}
			return 0;
		}

		sl_bool remove_NoLock(const T& value) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->remove_NoLock(value);
			}
			return sl_false;
		}

		sl_bool remove(const T& value) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->remove(value);
			}
			return sl_false;
		}

		sl_size removeItems_NoLock(const T& value) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->removeItems_NoLock(value);
			}
			return 0;
		}

		sl_size removeItems(const T& value) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->removeItems(value);
			}
			return 0;
		}

		sl_size removeAll_NoLock() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->removeAll_NoLock();
			}
			return 0;
		}

		sl_size removeAll() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->removeAll();
			}
			return 0;
		}

		Set duplicate_NoLock() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->duplicate_NoLock();
			}
			return sl_null;
		}

		Set duplicate() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->duplicate();
			}
			return sl_null;
		}

		List<T> toList_NoLock() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->toList_NoLock();
			}
			return sl_null;
		}

		List<T> toList() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->toList();
			}
			return sl_null;
		}

		NODE* getFirstNode() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->getFirstNode();
			}
			return sl_null;
		}

		NODE* getLastNode() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->getLastNode();
			}
			return sl_null;
		}

		const Mutex* getLocker() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->getLocker();
			}
			return sl_null;
		}

		// range-based for loop
		POSITION begin() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->getFirstNode();
			}
			return sl_null;
		}

		POSITION end() const noexcept
		{
			return sl_null;
		}

	};

	template <class T, class COMPARE>
	class SLIB_EXPORT Atomic< Set<T, COMPARE> >
	{
	public:
		typedef CSet<T, COMPARE> CSET;
		typedef Map<T, sl_bool, COMPARE> MAP_TYPE;

	public:
		AtomicRef<CSET> ref;
		SLIB_ATOMIC_REF_WRAPPER(CSET)
		
	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic(const std::initializer_list<T>& l) noexcept: ref(new CSET(l)) {}

		template <class COMPARE_ARG>
		Atomic(const std::initializer_list<T>& l, COMPARE_ARG&& compare) noexcept: ref(new CSET(l, Forward<COMPARE_ARG>(compare))) {}
#endif
		
	public:
		template <class TYPE, class COMPARE_ARG>
		static const Atomic& from(const Atomic< Set<TYPE, COMPARE_ARG> >& other) noexcept
		{
			return *(reinterpret_cast<Atomic const*>(&other));
		}
		
		void initialize() noexcept
		{
			ref = new CSET;
		}
		
		void initialize(const COMPARE& compare) noexcept
		{
			ref = new CSET(compare);
		}

		void initialize(COMPARE&& compare) noexcept
		{
			ref = new CSET(Move(compare));
		}

	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic& operator=(const std::initializer_list<T>& l) noexcept
		{
			ref = new CSET(l);
			return *this;
		}
#endif

	public:
		template <class VALUE>
		sl_bool put(VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			return ((Atomic<MAP_TYPE>*)((void*)this))->put(Forward<VALUE>(value), sl_true, isInsertion);
		}

		template <class VALUE>
		sl_bool add(VALUE&& value) noexcept
		{
			return ((Atomic<MAP_TYPE>*)((void*)this))->add(Forward<VALUE>(value), sl_true);
		}

		template <class MAP>
		sl_bool putAll(const MAP& other) noexcept
		{
			return ((Atomic<MAP_TYPE>*)((void*)this))->putAll(other);
		}

		template <class MAP>
		sl_bool addAll(const MAP& other) noexcept
		{
			return ((Atomic<MAP_TYPE>*)((void*)this))->addAll(other);
		}

	};
	
}

#endif
