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

#ifndef CHECKHEADER_SLIB_CORE_HASH_SET
#define CHECKHEADER_SLIB_CORE_HASH_SET

#include "set.h"
#include "hash_map.h"

namespace slib
{

	template <class T, class HASH, class COMPARE>
	class HashSet;

	template < class T, class HASH = Hash<T>, class COMPARE = Compare<T> >
	using AtomicHashSet = Atomic< HashSet<T, HASH, COMPARE> >;


	template < class T, class HASH = Hash<T>, class COMPARE = Compare<T> >
	class SLIB_EXPORT CHashSet : public CHashMap<T, sl_bool, HASH, COMPARE>
	{
	public:
		typedef CHashMap<T, sl_bool, HASH, COMPARE> CMAP;
		typedef T VALUE_TYPE;
		typedef HashMapNode<T, sl_bool> NODE;
		typedef SetPosition<NODE> POSITION;

	public:
		template <class HASH_ARG, class COMPARE_ARG>
		CHashSet(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept: CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare)) {}

		template <class HASH_ARG>
		CHashSet(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), COMPARE()) {}

		CHashSet(sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept: CMAP(capacityMinimum, capacityMaximum, HASH(), COMPARE()) {}

	public:
		CHashSet(const CHashSet& other) = delete;

		CHashSet& operator=(const CHashSet& other) = delete;

		CHashSet(CHashSet&& other) = default;

		CHashSet& operator=(CHashSet&& other) = default;

#ifdef SLIB_SUPPORT_STD_TYPES
		template <class HASH_ARG, class COMPARE_ARG>
		CHashSet(const std::initializer_list<T>& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept: CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare))
		{
			const T* data = l.begin();
			sl_size n = l.size();
			for (sl_size i = 0; i < n; i++) {
				CMAP::add_NoLock(data[i], sl_true);
			}
		}

		template <class HASH_ARG>
		CHashSet(const std::initializer_list<T>& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: CHashSet(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), COMPARE()) {}

		CHashSet(const std::initializer_list<T>& l, sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept: CHashSet(l, capacityMinimum, capacityMaximum, HASH(), COMPARE()) {}
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

		CHashSet* duplicate_NoLock() const noexcept
		{
			return (CHashSet*)(CMAP::duplicate_NoLock());
		}

		CHashSet* duplicate() const noexcept
		{
			return (CHashSet*)(CMAP::duplicate());
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


	template < class T, class HASH = Hash<T>, class COMPARE = Compare<T> >
	class SLIB_EXPORT HashSet
	{
	public:
		typedef CHashSet<T, HASH, COMPARE> CSET;
		typedef T VALUE_TYPE;
		typedef HashMap<T, sl_bool, HASH, COMPARE> MAP_TYPE;
		typedef HashMapNode<T, sl_bool> NODE;
		typedef SetPosition<NODE> POSITION;
		typedef typename MapBaseHelper::EnumLockHelper<HashSet> EnumLockHelper;
		typedef typename MapBaseHelper::EnumHelper<HashSet> EnumHelper;

	public:
		Ref<CSET> ref;
		SLIB_REF_WRAPPER(HashSet, CSET)

	public:
		HashSet(sl_size capacityMinimum, sl_size capacityMaximum = 0) noexcept: ref(new CSET(capacityMinimum, capacityMaximum)) {}

		template <class HASH_ARG>
		HashSet(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: ref(new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash))) {}

		template <class HASH_ARG, class COMPARE_ARG>
		HashSet(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept: ref(new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare))) {}

#ifdef SLIB_SUPPORT_STD_TYPES
		HashSet(const std::initializer_list<T>& l, sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept: ref(new CSET(l, capacityMinimum, capacityMaximum)) {}

		template <class HASH_ARG>
		HashSet(const std::initializer_list<T>& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: ref(new CSET(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash))) {}

		template <class HASH_ARG, class COMPARE_ARG>
		HashSet(const std::initializer_list<T>& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept: ref(new CSET(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare))) {}
#endif

	public:
		static HashSet create(sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept
		{
			return new CSET(capacityMinimum, capacityMaximum);
		}

		template <class HASH_ARG>
		static HashSet create(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept
		{
			return new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash));
		}

		template <class HASH_ARG, class COMPARE_ARG>
		static HashSet create(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept
		{
			return new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare));
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		static HashSet create(const std::initializer_list<T>& l, sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept
		{
			return new CSET(l, capacityMinimum, capacityMaximum);
		}

		template <class HASH_ARG>
		static HashSet create(const std::initializer_list<T>& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept
		{
			return new CSET(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash));
		}

		template <class HASH_ARG, class COMPARE_ARG>
		static HashSet create(const std::initializer_list<T>& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept
		{
			return new CSET(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare));
		}
#endif

		void initialize(sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept
		{
			ref = new CSET(capacityMinimum, capacityMaximum);
		}

		template <class HASH_ARG>
		void initialize(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept
		{
			ref = new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash));
		}

		template <class HASH_ARG, class COMPARE_ARG>
		void initialize(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept
		{
			ref = new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare));
		}

		SLIB_DEFINE_CAST_REF_FUNCTIONS(class... TYPES, HashSet, HashSet<TYPES...>)

	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		HashSet& operator=(const std::initializer_list<T>& l) noexcept
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

		sl_size getCapacity() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->getCapacity();
			}
			return 0;
		}

		sl_size getMinimumCapacity() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->getMinimumCapacity();
			}
			return 0;
		}

		void setMinimumCapacity_NoLock(sl_size capacity) noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				obj->setMinimumCapacity_NoLock(capacity);
			}
		}

		void setMinimumCapacity(sl_size capacity) noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				obj->setMinimumCapacity(capacity);
			}
		}

		sl_size getMaximumCapacity() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->getMaximumCapacity();
			}
			return 0;
		}

		void setMaximumCapacity_NoLock(sl_size capacity) noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				obj->setMaximumCapacity_NoLock(capacity);
			}
		}

		void setMaximumCapacity(sl_size capacity) noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				obj->setMaximumCapacity(capacity);
			}
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

		// unsynchronized function
		void removeAt(NODE* node) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				obj->removeAt(node);
			}
		}

		// unsynchronized function
		sl_size removeAt(NODE* node, sl_size count) const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->removeAt(node, count);
			}
			return 0;
		}

		// unsynchronized function
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

		void shrink_NoLock() noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				obj->shrink_NoLock();
			}
		}

		void shrink() noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				obj->shrink();
			}
		}

		HashSet duplicate_NoLock() const noexcept
		{
			CSET* obj = ref.ptr;
			if (obj) {
				return obj->duplicate_NoLock();
			}
			return sl_null;
		}

		HashSet duplicate() const noexcept
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

	template <class T, class HASH, class COMPARE>
	class SLIB_EXPORT Atomic< HashSet<T, HASH, COMPARE> >
	{
	public:
		typedef CHashSet<T, HASH, COMPARE> CSET;
		typedef HashMap<T, sl_bool, HASH, COMPARE> MAP_TYPE;

	public:
		AtomicRef<CSET> ref;
		SLIB_ATOMIC_REF_WRAPPER(CSET)

	public:
		Atomic(sl_size capacityMinimum, sl_size capacityMaximum = 0) noexcept: ref(new CSET(capacityMinimum, capacityMaximum)) {}

		template <class HASH_ARG>
		Atomic(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: ref(new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash))) {}

		template <class HASH_ARG, class COMPARE_ARG>
		Atomic(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept: ref(new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare))) {}

#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic(const std::initializer_list<T>& l, sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept: ref(new CSET(l, capacityMinimum, capacityMaximum)) {}

		template <class HASH_ARG>
		Atomic(const std::initializer_list<T>& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: ref(new CSET(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash))) {}

		template <class HASH_ARG, class COMPARE_ARG>
		Atomic(const std::initializer_list<T>& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept: ref(new CSET(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare))) {}
#endif

	public:
		void initialize(sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept
		{
			ref = new CSET(capacityMinimum, capacityMaximum);
		}

		template <class HASH_ARG>
		void initialize(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept
		{
			ref = new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash));
		}

		template <class HASH_ARG, class COMPARE_ARG>
		void initialize(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, COMPARE_ARG&& compare) noexcept
		{
			ref = new CSET(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<COMPARE_ARG>(compare));
		}

		SLIB_DEFINE_CAST_REF_FUNCTIONS(class... TYPES, Atomic, Atomic< HashSet<TYPES...> >)

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

