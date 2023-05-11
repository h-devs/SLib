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

#ifndef CHECKHEADER_SLIB_CORE_HASH_MAP
#define CHECKHEADER_SLIB_CORE_HASH_MAP

#include "map.h"
#include "hash_table.h"
#include "../math/math.h"

namespace slib
{

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	class CHashMap;

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	class HashMap;

	template < class KT, class VT, class HASH = Hash<KT>, class KEY_COMPARE = Compare<KT> >
	using AtomicHashMap = Atomic< HashMap<KT, VT, HASH, KEY_COMPARE> >;


	template <class KT, class VT>
	class SLIB_EXPORT HashMapNode
	{
	public:
		typedef KT KEY_TYPE;
		typedef VT VALUE_TYPE;

		HashMapNode* parent;
		HashMapNode* left;
		HashMapNode* right;
		sl_bool flagRed;

		KT key;
		VT value;

		sl_size hash;
		HashMapNode* previous;
		HashMapNode* next;

	public:
		HashMapNode(const HashMapNode& other) = delete;

#ifdef SLIB_COMPILER_IS_GCC
		HashMapNode(HashMapNode& other) = delete;
#endif

		template <class KEY, class... VALUE_ARGS>
		HashMapNode(KEY&& _key, VALUE_ARGS&&... value_args) noexcept: key(Forward<KEY>(_key)), value(Forward<VALUE_ARGS>(value_args)...), parent(sl_null), left(sl_null), right(sl_null), flagRed(sl_false) {}

	public:
		HashMapNode* getNext() const noexcept
		{
			return next;
		}

		HashMapNode* getPrevious() const noexcept
		{
			return previous;
		}

	};

	class SLIB_EXPORT CHashMapBase : public CRef, public Lockable
	{
		SLIB_DECLARE_OBJECT

	public:
		CHashMapBase();

		~CHashMapBase();

	};

	template < class KT, class VT, class HASH = Hash<KT>, class KEY_COMPARE = Compare<KT> >
	class SLIB_EXPORT CHashMap : public CHashMapBase
	{
	public:
		typedef KT KEY_TYPE;
		typedef VT VALUE_TYPE;
		typedef HashMap<KT, VT, HASH, KEY_COMPARE> MAP_TYPE;
		typedef HashMapNode<KT, VT> NODE;
		typedef NodePosition<NODE> POSITION;

		struct TABLE
		{
			NODE** nodes;
			sl_size count;
			sl_size capacity;
			sl_size capacityMinimum;
			sl_size capacityMaximum;
			sl_size thresholdDown;
			sl_size thresholdUp;
		};

	protected:
		TABLE m_table;
		NODE* m_nodeFirst;
		NODE* m_nodeLast;
		HASH m_hash;
		KEY_COMPARE m_compare;

	public:
		template <class HASH_ARG, class KEY_COMPARE_ARG>
		CHashMap(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept: m_hash(Forward<HASH_ARG>(hash)), m_compare(Forward<KEY_COMPARE_ARG>(compare))
		{
			priv::hash_table::Helper::initialize(reinterpret_cast<HashTableStructBase*>(&m_table), capacityMinimum, capacityMaximum);
			m_nodeFirst = sl_null;
			m_nodeLast = sl_null;
		}

		template <class HASH_ARG>
		CHashMap(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: CHashMap(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), KEY_COMPARE()) {}

		CHashMap(sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept: CHashMap(capacityMinimum, capacityMaximum, HASH(), KEY_COMPARE()) {}

		~CHashMap() noexcept
		{
			_free();
		}

	public:
		CHashMap(const CHashMap& other) = delete;

		CHashMap& operator=(const CHashMap& other) = delete;

		CHashMap(CHashMap&& other) noexcept: m_hash(Move(other.m_hash)), m_compare(Move(other.m_compare))
		{
			priv::hash_table::Helper::move(reinterpret_cast<HashTableStructBase*>(&m_table), reinterpret_cast<HashTableStructBase*>(&(other.m_table)));
			m_nodeFirst = other.m_nodeFirst;
			m_nodeLast = other.m_nodeLast;
			other.m_nodeFirst = sl_null;
			other.m_nodeLast = sl_null;
		}

		CHashMap& operator=(CHashMap&& other) noexcept
		{
			_free();
			priv::hash_table::Helper::move(reinterpret_cast<HashTableStructBase*>(&m_table), reinterpret_cast<HashTableStructBase*>(&(other.m_table)));
			m_nodeFirst = other.m_nodeFirst;
			m_nodeLast = other.m_nodeLast;
			other.m_nodeFirst = sl_null;
			other.m_nodeLast = sl_null;
			m_hash = Move(other.m_hash);
			m_compare = Move(other.m_compare);
			return *this;
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		template <class HASH_ARG, class KEY_COMPARE_ARG>
		CHashMap(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept: m_hash(Forward<HASH_ARG>(hash)), m_compare(Forward<KEY_COMPARE_ARG>(compare))
		{
			priv::hash_table::Helper::initialize(reinterpret_cast<HashTableStructBase*>(&m_table), capacityMinimum, capacityMaximum);
			m_nodeFirst = sl_null;
			m_nodeLast = sl_null;
			const Pair<KT, VT>* data = l.begin();
			sl_size n = l.size();
			for (sl_size i = 0; i < n; i++) {
				add_NoLock(data[i].first, data[i].second);
			}
		}

		template <class HASH_ARG>
		CHashMap(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: CHashMap(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), KEY_COMPARE()) {}

		CHashMap(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept: CHashMap(l, capacityMinimum, capacityMaximum, HASH(), KEY_COMPARE()) {}
#endif

	public:
		sl_size getCount() const noexcept
		{
			return m_table.count;
		}

		sl_bool isEmpty() const noexcept
		{
			return !(m_table.count);
		}

		sl_bool isNotEmpty() const noexcept
		{
			return m_table.count != 0;
		}

		sl_size getCapacity() const noexcept
		{
			return m_table.capacity;
		}

		sl_size getMinimumCapacity() const noexcept
		{
			return m_table.capacityMinimum;
		}

		void setMinimumCapacity_NoLock(sl_size capacity) noexcept
		{
			priv::hash_table::Helper::setMinimumCapacity(reinterpret_cast<HashTableStructBase*>(&m_table), capacity);
		}

		void setMinimumCapacity(sl_size capacity) noexcept
		{
			ObjectLocker lock(this);
			setMinimumCapacity_NoLock(capacity);
		}

		sl_size getMaximumCapacity() const noexcept
		{
			return m_table.capacityMaximum;
		}

		void setMaximumCapacity_NoLock(sl_size capacity) noexcept
		{
			priv::hash_table::Helper::setMaximumCapacity(reinterpret_cast<HashTableStructBase*>(&m_table), capacity);
		}

		void setMaximumCapacity(sl_size capacity) noexcept
		{
			ObjectLocker lock(this);
			setMaximumCapacity_NoLock(capacity);
		}

		NODE* getFirstNode() const noexcept
		{
			return m_nodeFirst;
		}

		NODE* getLastNode() const noexcept
		{
			return m_nodeLast;
		}

		NODE* find_NoLock(const KT& key) const noexcept
		{
			NODE* entry = _getEntry(key);
			return RedBlackTree::find(entry, key, m_compare);
		}

		sl_bool find(const KT& key) const noexcept
		{
			ObjectLocker lock(this);
			NODE* entry = _getEntry(key);
			return RedBlackTree::find(entry, key, m_compare) != sl_null;
		}

		// unsynchronized function
		sl_bool getEqualRange(const KT& key, MapNode<KT, VT>** pStart = sl_null, MapNode<KT, VT>** pEnd = sl_null) const noexcept
		{
			NODE* entry = _getEntry(key);
			return RedBlackTree::getEqualRange(entry, key, m_compare, (NODE**)pStart, (NODE**)pEnd);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		NODE* findKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			NODE* entry = _getEntry(key);
			return RedBlackTree::findKeyAndValue(entry, key, m_compare, value, value_equals);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool findKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			ObjectLocker lock(this);
			NODE* entry = _getEntry(key);
			return RedBlackTree::findKeyAndValue(entry, key, m_compare, value, value_equals) != sl_null;
		}

		/* added for compatibility with `Map` */
		NODE* getLowerBound(const KT& key) const noexcept
		{
			return sl_null;
		}

		/* added for compatibility with `Map` */
		NODE* getUpperBound(const KT& key) const noexcept
		{
			return sl_null;
		}

		// unsynchronized function
		VT* getItemPointer(const KT& key) const noexcept
		{
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				return &(node->value);
			}
			return sl_null;
		}

		// unsynchronized function
		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		VT* getItemPointerByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::findKeyAndValue(entry, key, m_compare, value, value_equals);
			if (node) {
				return &(node->value);
			}
			return sl_null;
		}

		sl_bool get_NoLock(const KT& key, VT* _out) const noexcept
		{
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				if (_out) {
					*_out = node->value;
				}
				return sl_true;
			}
			return sl_false;
		}

		sl_bool get(const KT& key, VT* _out) const noexcept
		{
			ObjectLocker lock(this);
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				if (_out) {
					*_out = node->value;
				}
				return sl_true;
			}
			return sl_false;
		}

		sl_bool get_NoLock(const KT& key, Nullable<VT>* _out) const noexcept
		{
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				if (_out) {
					*_out = node->value;
				}
				return sl_true;
			} else {
				if (_out) {
					_out->setNull();
				}
				return sl_false;
			}
		}

		sl_bool get(const KT& key, Nullable<VT>* _out) const noexcept
		{
			ObjectLocker lock(this);
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				if (_out) {
					*_out = node->value;
				}
				return sl_true;
			} else {
				if (_out) {
					_out->setNull();
				}
				return sl_false;
			}
		}

		VT getValue_NoLock(const KT& key) const noexcept
		{
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				return node->value;
			} else {
				return VT();
			}
		}

		VT getValue(const KT& key) const noexcept
		{
			ObjectLocker lock(this);
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				return node->value;
			} else {
				return VT();
			}
		}

		VT getValue_NoLock(const KT& key, const VT& def) const noexcept
		{
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				return node->value;
			} else {
				return def;
			}
		}

		VT getValue(const KT& key, const VT& def) const noexcept
		{
			ObjectLocker lock(this);
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				return node->value;
			} else {
				return def;
			}
		}

		List<VT> getValues_NoLock(const KT& key) const noexcept
		{
			List<VT> list;
			NODE* entry = _getEntry(key);
			RedBlackTree::getValues(list, entry, key, m_compare);
			return list;
		}

		List<VT> getValues(const KT& key) const noexcept
		{
			ObjectLocker lock(this);
			List<VT> list;
			NODE* entry = _getEntry(key);
			RedBlackTree::getValues(list, entry, key, m_compare);
			return list;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		List<VT> getValuesByKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			List<VT> list;
			NODE* entry = _getEntry(key);
			RedBlackTree::getValuesByKeyAndValue(list, entry, key, m_compare, value, value_equals);
			return list;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		List<VT> getValuesByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			ObjectLocker lock(this);
			List<VT> list;
			NODE* entry = _getEntry(key);
			RedBlackTree::getValuesByKeyAndValue(list, entry, key, m_compare, value, value_equals);
			return list;
		}

		template <class KEY, class VALUE>
		NODE* put_NoLock(KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			if (!(priv::hash_table::Helper::validateNodes(reinterpret_cast<HashTableStructBase*>(&m_table)))) {
				return sl_null;
			}
			sl_size capacity = m_table.capacity;
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);
			NODE** nodes = m_table.nodes;
			sl_bool flagInsert;
			NODE* node = RedBlackTree::put(&(nodes[index]), m_table.count, Forward<KEY>(key), m_compare, Forward<VALUE>(value), &flagInsert);
			if (isInsertion) {
				*isInsertion = flagInsert;
			}
			if (flagInsert) {
				_linkNode(node, hash);
				_expand();
			}
			return node;
		}

		template <class KEY, class VALUE>
		sl_bool put(KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			ObjectLocker lock(this);
			return put_NoLock(Forward<KEY>(key), Forward<VALUE>(value), isInsertion) != sl_null;
		}

		template <class KEY, class VALUE>
		NODE* replace_NoLock(const KEY& key, VALUE&& value) noexcept
		{
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				node->value = Forward<VALUE>(value);
				return node;
			}
			return sl_null;
		}

		template <class KEY, class VALUE>
		sl_bool replace(const KEY& key, VALUE&& value) noexcept
		{
			ObjectLocker lock(this);
			NODE* entry = _getEntry(key);
			NODE* node = RedBlackTree::find(entry, key, m_compare);
			if (node) {
				node->value = Forward<VALUE>(value);
				return sl_true;
			}
			return sl_false;
		}

		template <class KEY, class... VALUE_ARGS>
		NODE* add_NoLock(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			if (!(priv::hash_table::Helper::validateNodes(reinterpret_cast<HashTableStructBase*>(&m_table)))) {
				return sl_null;
			}
			sl_size capacity = m_table.capacity;
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);
			NODE** nodes = m_table.nodes;
			NODE* node = RedBlackTree::add(&(nodes[index]), m_table.count, Forward<KEY>(key), m_compare, Forward<VALUE_ARGS>(value_args)...);
			if (node) {
				_linkNode(node, hash);
				_expand();
				return node;
			}
			return sl_null;
		}

		template <class KEY, class... VALUE_ARGS>
		sl_bool add(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			ObjectLocker lock(this);
			return add_NoLock(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...) != sl_null;
		}

		template <class KEY, class... VALUE_ARGS>
		MapEmplaceReturn<NODE> emplace_NoLock(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			if (!(priv::hash_table::Helper::validateNodes(reinterpret_cast<HashTableStructBase*>(&m_table)))) {
				return sl_null;
			}
			sl_size capacity = m_table.capacity;
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);
			NODE** nodes = m_table.nodes;
			MapEmplaceReturn<NODE> ret = RedBlackTree::emplace(&(nodes[index]), m_table.count, Forward<KEY>(key), m_compare, Forward<VALUE_ARGS>(value_args)...);
			if (ret.isSuccess) {
				_linkNode(ret.node, hash);
				_expand();
			}
			return ret;
		}

		template <class KEY, class... VALUE_ARGS>
		sl_bool emplace(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			ObjectLocker lock(this);
			return emplace_NoLock(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
		}

		template <class MAP>
		sl_bool putAll_NoLock(const MAP& other) noexcept
		{
			typename MAP::EnumHelper helper(other);
			auto node = helper.node;
			if (!node) {
				return sl_true;
			}
			if (reinterpret_cast<void*>(m_nodeFirst) == reinterpret_cast<void*>(node)) {
				return sl_true;
			}
			while (node) {
				if (!(put_NoLock(node->key, node->value))) {
					return sl_false;
				}
				node = node->getNext();
			}
			return sl_true;
		}

		template <class MAP>
		sl_bool putAll(const MAP& other) noexcept
		{
			typename MAP::EnumLockHelper helper(other);
			auto node = helper.node;
			if (!node) {
				return sl_true;
			}
			if (reinterpret_cast<void*>(m_nodeFirst) == reinterpret_cast<void*>(node)) {
				return sl_true;
			}
			MultipleMutexLocker lock(getLocker(), helper.mutex);
			while (node) {
				if (!(put_NoLock(node->key, node->value))) {
					return sl_false;
				}
				node = node->getNext();
			}
			return sl_true;
		}

		template <class MAP>
		void replaceAll_NoLock(const MAP& other) noexcept
		{
			typename MAP::EnumHelper helper(other);
			auto node = helper.node;
			if (!node) {
				return;
			}
			if (reinterpret_cast<void*>(m_nodeFirst) == reinterpret_cast<void*>(node)) {
				return;
			}
			while (node) {
				replace_NoLock(node->key, node->value);
				node = node->getNext();
			}
		}

		template <class MAP>
		void replaceAll(const MAP& other) noexcept
		{
			typename MAP::EnumLockHelper helper(other);
			auto node = helper.node;
			if (!node) {
				return;
			}
			if (reinterpret_cast<void*>(m_nodeFirst) == reinterpret_cast<void*>(node)) {
				return;
			}
			MultipleMutexLocker lock(getLocker(), helper.mutex);
			while (node) {
				replace_NoLock(node->key, node->value);
				node = node->getNext();
			}
		}

		template <class MAP>
		sl_bool addAll_NoLock(const MAP& other) noexcept
		{
			typename MAP::EnumHelper helper(other);
			auto node = helper.node;
			if (!node) {
				return sl_true;
			}
			if (reinterpret_cast<void*>(m_nodeFirst) == reinterpret_cast<void*>(node)) {
				return sl_false;
			}
			while (node) {
				if (!(add_NoLock(node->key, node->value))) {
					return sl_false;
				}
				node = node->getNext();
			}
			return sl_true;
		}

		template <class MAP>
		sl_bool addAll(const MAP& other) noexcept
		{
			typename MAP::EnumLockHelper helper(other);
			auto node = helper.node;
			if (!node) {
				return sl_true;
			}
			if (reinterpret_cast<void*>(m_nodeFirst) == reinterpret_cast<void*>(node)) {
				return sl_false;
			}
			MultipleMutexLocker lock(getLocker(), helper.mutex);
			while (node) {
				if (!(add_NoLock(node->key, node->value))) {
					return sl_false;
				}
				node = node->getNext();
			}
			return sl_true;
		}

		template <class MAP>
		sl_bool emplaceAll_NoLock(const MAP& other) noexcept
		{
			typename MAP::EnumHelper helper(other);
			auto node = helper.node;
			if (!node) {
				return sl_true;
			}
			if (reinterpret_cast<void*>(m_nodeFirst) == reinterpret_cast<void*>(node)) {
				return sl_true;
			}
			while (node) {
				MapEmplaceReturn<NODE> ret = emplace_NoLock(node->key, node->value);
				if (!(ret.node)) {
					return sl_false;
				}
				node = node->getNext();
			}
			return sl_true;
		}

		template <class MAP>
		sl_bool emplaceAll(const MAP& other) noexcept
		{
			typename MAP::EnumLockHelper helper(other);
			auto node = helper.node;
			if (!node) {
				return sl_true;
			}
			if (reinterpret_cast<void*>(m_nodeFirst) == reinterpret_cast<void*>(node)) {
				return sl_true;
			}
			MultipleMutexLocker lock(getLocker(), helper.mutex);
			while (node) {
				MapEmplaceReturn<NODE> ret = emplace_NoLock(node->key, node->value);
				if (!(ret.node)) {
					return sl_false;
				}
				node = node->getNext();
			}
			return sl_true;
		}

		// unsynchronized function
		void removeAt(NODE* node) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return;
			}
			_unlinkNode(node);
			RedBlackTree::removeNode(m_table.nodes + (node->hash & (capacity - 1)), m_table.count, node);
		}

		// unsynchronized function
		sl_size removeAt(NODE* node, sl_size count) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return 0;
			}
			NODE** nodes = m_table.nodes;
			sl_size n = 0;
			while (n < count && node) {
				NODE* next = node->next;
				_unlinkNode(node);
				RedBlackTree::removeNode(nodes + (node->hash & (capacity - 1)), m_table.count, node);
				node = next;
				n++;
			}
			return n;
		}

		// unsynchronized function
		sl_size removeRange(NODE* first, NODE* last) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return 0;
			}
			NODE* node = first;
			if (!node) {
				node = m_nodeFirst;
				if (!node) {
					return 0;
				}
			}
			NODE** nodes = m_table.nodes;
			sl_size n = 0;
			for (;;) {
				n++;
				NODE* next = node->next;
				_unlinkNode(node);
				RedBlackTree::removeNode(nodes + (node->hash & (capacity - 1)), m_table.count, node);
				if (node == last || !next) {
					break;
				}
				node = next;
			}
			return n;
		}

		sl_bool remove_NoLock(const KT& key, VT* outValue = sl_null) noexcept
		{
			NODE** pEntry = _getEntryPtr(key);
			if (!pEntry) {
				return sl_false;
			}
			NODE* node = RedBlackTree::find(*pEntry, key, m_compare);
			if (node) {
				if (outValue) {
					*outValue = Move(node->value);
				}
				_unlinkNode(node);
				RedBlackTree::removeNode(pEntry, m_table.count, node);
				return sl_true;
			}
			return sl_false;
		}

		sl_bool remove(const KT& key, VT* outValue = sl_null) noexcept
		{
			ObjectLocker lock(this);
			return remove_NoLock(key, outValue);
		}

		sl_size removeItems_NoLock(const KT& key) noexcept
		{
			NODE** pEntry = _getEntryPtr(key);
			if (!pEntry) {
				return 0;
			}
			NODE* node;
			NODE* end;
			if (RedBlackTree::getEqualRange(*pEntry, key, m_compare, &node, &end)) {
				sl_size n = 0;
				for (;;) {
					n++;
					if (node == end) {
						_unlinkNode(node);
						RedBlackTree::removeNode(pEntry, m_table.count, node);
						break;
					} else {
						NODE* next = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
						_unlinkNode(node);
						RedBlackTree::removeNode(pEntry, m_table.count, node);
						node = next;
					}
				}
				return n;
			} else {
				return 0;
			}
		}

		sl_size removeItems(const KT& key) noexcept
		{
			ObjectLocker lock(this);
			return removeItems_NoLock(key);
		}

		List<VT> removeItemsAndReturnValues_NoLock(const KT& key) noexcept
		{
			List<VT> ret;
			NODE** pEntry = _getEntryPtr(key);
			if (!pEntry) {
				return sl_null;
			}
			NODE* node;
			NODE* end;
			if (RedBlackTree::getEqualRange(*pEntry, key, m_compare, &node, &end)) {
				for (;;) {
					ret.add_NoLock(node->value);
					if (node == end) {
						_unlinkNode(node);
						RedBlackTree::removeNode(pEntry, m_table.count, node);
						break;
					} else {
						NODE* next = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
						_unlinkNode(node);
						RedBlackTree::removeNode(pEntry, m_table.count, node);
						node = next;
					}
				}
			}
			return ret;
		}

		List<VT> removeItemsAndReturnValues(const KT& key) noexcept
		{
			ObjectLocker lock(this);
			return removeItemsAndReturnValues_NoLock(key);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool removeKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			NODE** pEntry = _getEntryPtr(key);
			if (!pEntry) {
				return sl_false;
			}
			NODE* node;
			NODE* end;
			if (RedBlackTree::getEqualRange(*pEntry, key, m_compare, &node, &end)) {
				for (;;) {
					if (value_equals(node->value, value)) {
						_unlinkNode(node);
						RedBlackTree::removeNode(pEntry, m_table.count, node);
						return sl_true;
					}
					if (node == end) {
						break;
					}
					node = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
				}
			}
			return sl_false;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool removeKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			ObjectLocker lock(this);
			return removeKeyAndValue_NoLock(key, value, value_equals);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_size removeItemsByKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			NODE** pEntry = _getEntryPtr(key);
			if (!pEntry) {
				return 0;
			}
			NODE* node;
			NODE* end;
			if (RedBlackTree::getEqualRange(*pEntry, key, m_compare, &node, &end)) {
				sl_size n = 0;
				for (;;) {
					if (value_equals(node->value, value)) {
						n++;
						if (node == end) {
							_unlinkNode(node);
							RedBlackTree::removeNode(pEntry, m_table.count, node);
							break;
						} else {
							NODE* next = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
							_unlinkNode(node);
							RedBlackTree::removeNode(pEntry, m_table.count, node);
							node = next;
						}
					} else {
						if (node == end) {
							break;
						}
						node = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
					}
				}
				return n;
			} else {
				return 0;
			}
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_size removeItemsByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			ObjectLocker lock(this);
			return removeItemsByKeyAndValue_NoLock(key, value, value_equals);
		}

		sl_size removeAll_NoLock() noexcept
		{
			if (!(m_table.capacity)) {
				return 0;
			}
			sl_size count = m_table.count;
			_free();
			priv::hash_table::Helper::initialize(reinterpret_cast<HashTableStructBase*>(&m_table), m_table.capacityMinimum, m_table.capacityMaximum);
			m_nodeFirst = sl_null;
			m_nodeLast = sl_null;
			return count;
		}

		sl_size removeAll() noexcept
		{
			NODE* first;
			NODE** nodes;
			sl_size count;
			{
				ObjectLocker lock(this);
				if (m_table.capacity == 0) {
					return 0;
				}
				first = m_nodeFirst;
				nodes = m_table.nodes;
				count = m_table.count;
				priv::hash_table::Helper::initialize(reinterpret_cast<HashTableStructBase*>(&m_table), m_table.capacityMinimum, m_table.capacityMaximum);
				m_nodeFirst = sl_null;
				m_nodeLast = sl_null;
			}
			NODE* node = first;
			while (node) {
				NODE* next = node->next;
				delete node;
				node = next;
			}
			if (nodes) {
				Base::freeMemory(nodes);
			}
			return count;
		}

		void shrink_NoLock() noexcept
		{
			TABLE* table = &m_table;
			sl_size n = Math::roundUpToPowerOfTwo(table->count);
			if (n < table->capacityMinimum) {
				n = table->capacityMinimum;
			}
			if (n < table->capacity) {
				if (priv::hash_table::Helper::reallocNodes(reinterpret_cast<HashTableStructBase*>(table), n)) {
					_rebuildTree(n);
				}
			}
		}

		void shrink() noexcept
		{
			ObjectLocker lock(this);
			shrink_NoLock();
		}

		sl_bool copyFrom_NoLock(const CHashMap& other) noexcept
		{
			if (this == &other) {
				return sl_true;
			}
			m_hash = other.m_hash;
			m_compare = other.m_compare;
			_free();
			m_nodeFirst = sl_null;
			m_nodeLast = sl_null;
			priv::hash_table::Helper::initialize(reinterpret_cast<HashTableStructBase*>(&m_table), other.m_table.capacityMinimum, other.m_table.capacityMaximum);
			return _copyFrom(&other);
		}

		sl_bool copyFrom(const CHashMap& other) noexcept
		{
			ObjectLocker lock(this);
			return copyFrom_NoLock(other);
		}

		CHashMap* duplicate_NoLock() const noexcept
		{
			CHashMap* map = new CHashMap(m_table.capacityMinimum, m_table.capacityMaximum, m_hash, m_compare);
			if (map) {
				if (map->_copyFrom(this)) {
					return map;
				}
				delete map;
			}
			return sl_null;
		}

		CHashMap* duplicate() const noexcept
		{
			ObjectLocker lock(this);
			return duplicate_NoLock();
		}

		List<KT> getAllKeys_NoLock() const noexcept
		{
			List<KT> ret;
			NODE* node = m_nodeFirst;
			while (node) {
				ret.add_NoLock(node->key);
				node = node->next;
			}
			return ret;
		}

		List<KT> getAllKeys() const noexcept
		{
			ObjectLocker lock(this);
			return getAllKeys_NoLock();
		}

		List<VT> getAllValues_NoLock() const noexcept
		{
			List<VT> ret;
			NODE* node = m_nodeFirst;
			while (node) {
				ret.add_NoLock(node->value);
				node = node->next;
			}
			return ret;
		}

		List<VT> getAllValues() const noexcept
		{
			ObjectLocker lock(this);
			return getAllValues_NoLock();
		}

		List< Pair<KT, VT> > toList_NoLock() const noexcept
		{
			List< Pair<KT, VT> > ret;
			NODE* node = m_nodeFirst;
			while (node) {
				ret.add_NoLock(Pair<KT, VT>(node->key, node->value));
				node = node->next;
			}
			return ret;
		}

		List< Pair<KT, VT> > toList() const noexcept
		{
			ObjectLocker lock(this);
			return toList_NoLock();
		}

		Ref<Object> toObject() noexcept;

		Ref<Object> toObject_NoLocking() noexcept;

		// range-based for loop
		POSITION begin() const noexcept
		{
			return m_nodeFirst;
		}

		POSITION end() const noexcept
		{
			return sl_null;
		}

	protected:
		void _free() noexcept
		{
			NODE* node = m_nodeFirst;
			while (node) {
				NODE* next = node->next;
				delete node;
				node = next;
			}
			NODE** nodes = m_table.nodes;
			if (nodes) {
				Base::freeMemory(nodes);
			}
		}

		NODE* _getEntry(const KT& key) const noexcept
		{
			sl_size capacity = m_table.capacity;
			if (capacity == 0) {
				return sl_null;
			}
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);
			return m_table.nodes[index];
		}

		NODE** _getEntryPtr(const KT& key) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (capacity == 0) {
				return sl_null;
			}
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);
			return m_table.nodes + index;
		}

		void _linkNode(NODE* node, sl_size hash) noexcept
		{
			NODE* last = m_nodeLast;
			node->hash = hash;
			node->next = sl_null;
			node->previous = last;
			if (last) {
				last->next = node;
			} else {
				m_nodeFirst = node;
			}
			m_nodeLast = node;
		}

		void _unlinkNode(NODE* node) noexcept
		{
			NODE* previous = node->previous;
			NODE* next = node->next;
			if (previous) {
				previous->next = next;
			} else {
				m_nodeFirst = next;
			}
			if (next) {
				next->previous = previous;
			} else {
				m_nodeLast = previous;
			}
		}

		void _expand() noexcept
		{
			TABLE* table = &m_table;
			if (table->capacity < table->capacityMaximum && table->count >= table->thresholdUp) {
				// double capacity
				sl_size n = table->capacity;
				n *= 2;
				if (priv::hash_table::Helper::reallocNodes(reinterpret_cast<HashTableStructBase*>(table), n)) {
					_rebuildTree(n);
				}
			}
		}

		void _rebuildTree(sl_size capacity) noexcept
		{
			TABLE* table = &m_table;
			NODE** nodes = table->nodes;
			Base::zeroMemory(nodes, capacity * sizeof(NODE*));
			NODE* node = m_nodeFirst;
			while (node) {
				node->flagRed = sl_false;
				node->parent = sl_null;
				node->left = sl_null;
				node->right = sl_null;
				sl_size index = node->hash & (capacity - 1);
				RedBlackTree::addNode(nodes + index, node, m_compare);
				node = node->next;
			}
		}

		sl_bool _copyFrom(const CHashMap* other) noexcept
		{
			sl_size capacity = other->m_table.capacity;
			if (capacity == 0) {
				return sl_true;
			}
			if (!(priv::hash_table::Helper::reallocNodes(reinterpret_cast<HashTableStructBase*>(&m_table), capacity))) {
				return sl_false;
			}
			NODE** nodes = m_table.nodes;
			Base::zeroMemory(nodes, capacity * sizeof(NODE*));
			NODE* nodeSrc = other->m_nodeFirst;
			while (nodeSrc) {
				sl_size index = nodeSrc->hash & (capacity - 1);
				NODE* node = RedBlackTree::add(&(nodes[index]), m_table.count, nodeSrc->key, m_compare, nodeSrc->value);
				if (node) {
					_linkNode(node, nodeSrc->hash);
				} else {
					removeAll_NoLock();
					return sl_false;
				}
				nodeSrc = nodeSrc->next;
			}
			return sl_true;
		}

	public:
		class EnumLockHelper
		{
		public:
			EnumLockHelper(const CHashMap& map) noexcept
			{
				node = map.m_nodeFirst;
				mutex = map.getLocker();
			}
		public:
			NODE* node;
			Mutex* mutex;
		};

		class EnumHelper
		{
		public:
			EnumHelper(const CHashMap& map) noexcept
			{
				node = map.m_nodeFirst;
			}
		public:
			NODE* node;
		};

	};


	template < class KT, class VT, class HASH = Hash<KT>, class KEY_COMPARE = Compare<KT> >
	class SLIB_EXPORT HashMap
	{
	public:
		typedef CHashMap<KT, VT, HASH, KEY_COMPARE> CMAP;
		typedef KT KEY_TYPE;
		typedef VT VALUE_TYPE;
		typedef HashMapNode<KT, VT> NODE;
		typedef NodePosition<NODE> POSITION;
		typedef typename MapBaseHelper::EnumLockHelper<HashMap> EnumLockHelper;
		typedef typename MapBaseHelper::EnumHelper<HashMap> EnumHelper;

	public:
		Ref<CMAP> ref;
		SLIB_REF_WRAPPER(HashMap, CMAP)

	public:
		HashMap(sl_size capacityMinimum, sl_size capacityMaximum = 0) noexcept: ref(new CMAP(capacityMinimum, capacityMaximum)) {}

		template <class HASH_ARG>
		HashMap(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: ref(new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash))) {}

		template <class HASH_ARG, class KEY_COMPARE_ARG>
		HashMap(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept: ref(new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<KEY_COMPARE_ARG>(compare))) {}

#ifdef SLIB_SUPPORT_STD_TYPES
		HashMap(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept: ref(new CMAP(l, capacityMinimum, capacityMaximum)) {}

		template <class HASH_ARG>
		HashMap(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept: ref(new CMAP(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash))) {}

		template <class HASH_ARG, class KEY_COMPARE_ARG>
		HashMap(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept: ref(new CMAP(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<KEY_COMPARE_ARG>(compare))) {}
#endif

	public:
		static HashMap create(sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept
		{
			return new CMAP(capacityMinimum, capacityMaximum);
		}

		template <class HASH_ARG>
		static HashMap create(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept
		{
			return new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash));
		}

		template <class HASH_ARG, class KEY_COMPARE_ARG>
		static HashMap create(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept
		{
			return new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<KEY_COMPARE_ARG>(compare));
		}

		static HashMap create(Object* object);

#ifdef SLIB_SUPPORT_STD_TYPES
		static HashMap create(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept
		{
			return new CMAP(l, capacityMinimum, capacityMaximum);
		}

		template <class HASH_ARG>
		static HashMap create(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept
		{
			return new CMAP(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash));
		}

		template <class HASH_ARG, class KEY_COMPARE_ARG>
		static HashMap create(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept
		{
			return new CMAP(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<KEY_COMPARE_ARG>(compare));
		}
#endif

		template <class KEY, class VALUE, class OTHER_HASH, class OTHER_COMPARE>
		static const HashMap& from(const HashMap<KEY, VALUE, OTHER_HASH, OTHER_COMPARE>& other) noexcept
		{
			return *(reinterpret_cast<HashMap const*>(&other));
		}

		void initialize(sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept
		{
			ref = new CMAP(capacityMinimum, capacityMaximum);
		}

		template <class HASH_ARG>
		void initialize(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept
		{
			ref = new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash));
		}

		template <class HASH_ARG, class KEY_COMPARE_ARG>
		void initialize(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept
		{
			ref = new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<KEY_COMPARE_ARG>(compare));
		}

	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		HashMap& operator=(const std::initializer_list< Pair<KT, VT> >& l) noexcept
		{
			ref = new CMAP(l);
			return *this;
		}
#endif

		VT operator[](const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getValue(key);
			} else {
				return VT();
			}
		}

		sl_size getCount() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getCount();
			}
			return 0;
		}

		sl_bool isEmpty() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return (obj->getCount()) == 0;
			}
			return sl_true;
		}

		sl_bool isNotEmpty() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return (obj->getCount()) > 0;
			}
			return sl_false;
		}

		sl_size getCapacity() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getCapacity();
			}
			return 0;
		}

		sl_size getMinimumCapacity() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getMinimumCapacity();
			}
			return 0;
		}

		void setMinimumCapacity_NoLock(sl_size capacity) noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->setMinimumCapacity_NoLock(capacity);
			}
		}

		void setMinimumCapacity(sl_size capacity) noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->setMinimumCapacity(capacity);
			}
		}

		sl_size getMaximumCapacity() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getMaximumCapacity();
			}
			return 0;
		}

		void setMaximumCapacity_NoLock(sl_size capacity) noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->setMaximumCapacity_NoLock(capacity);
			}
		}

		void setMaximumCapacity(sl_size capacity) noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->setMaximumCapacity(capacity);
			}
		}

		NODE* getFirstNode() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getFirstNode();
			}
			return sl_null;
		}

		NODE* getLastNode() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getLastNode();
			}
			return sl_null;
		}

		NODE* find_NoLock(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->find_NoLock(key);
			}
			return sl_null;
		}

		sl_bool find(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->find(key);
			}
			return sl_false;
		}

		// unsynchronized function
		sl_bool getEqualRange(const KT& key, MapNode<KT, VT>** pStart = sl_null, MapNode<KT, VT>** pEnd = sl_null) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getEqualRange(key, pStart, pEnd);
			}
			return sl_false;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		NODE* findKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->findKeyAndValue_NoLock(key, value, value_equals);
			}
			return sl_null;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool findKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->findKeyAndValue(key, value, value_equals);
			}
			return sl_false;
		}

		// unsynchronized function
		VT* getItemPointer(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getItemPointer(key);
			}
			return sl_null;
		}

		// unsynchronized function
		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		VT* getItemPointerByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getItemPointerByKeyAndValue(key, value, value_equals);
			}
			return sl_null;
		}

		sl_bool get_NoLock(const KT& key, VT* _out) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->get_NoLock(key, _out);
			}
			return sl_false;
		}

		sl_bool get(const KT& key, VT* _out) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->get(key, _out);
			}
			return sl_false;
		}

		sl_bool get_NoLock(const KT& key, Nullable<VT>* _out) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->get_NoLock(key, _out);
			}
			return sl_false;
		}

		sl_bool get(const KT& key, Nullable<VT>* _out) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->get(key, _out);
			}
			return sl_false;
		}

		VT getValue_NoLock(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getValue_NoLock(key);
			} else {
				return VT();
			}
		}

		VT getValue(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getValue(key);
			} else {
				return VT();
			}
		}

		VT getValue_NoLock(const KT& key, const VT& def) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getValue_NoLock(key, def);
			} else {
				return def;
			}
		}

		VT getValue(const KT& key, const VT& def) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getValue(key, def);
			} else {
				return def;
			}
		}

		List<VT> getValues_NoLock(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getValues_NoLock(key);
			}
			return sl_null;
		}

		List<VT> getValues(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getValues(key);
			}
			return sl_null;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		List<VT> getValuesByKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getValuesByKeyAndValue_NoLock(key, value, value_equals);
			}
			return sl_null;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		List<VT> getValuesByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getValuesByKeyAndValue(key, value, value_equals);
			}
			return sl_null;
		}

		template <class KEY, class VALUE>
		NODE* put_NoLock(KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			return MapBaseHelper::put_NoLock(this, Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
		}

		template <class KEY, class VALUE>
		sl_bool put(KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			return MapBaseHelper::put(this, Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
		}

		template <class KEY, class VALUE>
		NODE* replace_NoLock(const KEY& key, VALUE&& value) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->replace_NoLock(key, Forward<VALUE>(value));
			}
			return sl_null;
		}

		template <class KEY, class VALUE>
		sl_bool replace(const KEY& key, VALUE&& value) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->replace(key, Forward<VALUE>(value));
			}
			return sl_false;
		}

		template <class KEY, class... VALUE_ARGS>
		NODE* add_NoLock(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			return MapBaseHelper::add_NoLock(this, Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
		}

		template <class KEY, class... VALUE_ARGS>
		sl_bool add(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			return MapBaseHelper::add(this, Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
		}

		template <class KEY, class... VALUE_ARGS>
		MapEmplaceReturn<NODE> emplace_NoLock(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			return MapBaseHelper::emplace_NoLock(this, Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
		}

		template <class KEY, class... VALUE_ARGS>
		sl_bool emplace(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			return MapBaseHelper::emplace(this, Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
		}

		template <class MAP>
		sl_bool putAll_NoLock(const MAP& other) noexcept
		{
			return MapBaseHelper::putAll_NoLock(this, other);
		}

		template <class MAP>
		sl_bool putAll(const MAP& other) noexcept
		{
			return MapBaseHelper::putAll(this, other);
		}

		template <class MAP>
		void replaceAll_NoLock(const MAP& other) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->replaceAll_NoLock(other);
			}
		}

		template <class MAP>
		void replaceAll(const MAP& other) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->replaceAll(other);
			}
		}

		template <class MAP>
		sl_bool addAll_NoLock(const MAP& other) noexcept
		{
			return MapBaseHelper::addAll_NoLock(this, other);
		}

		template <class MAP>
		sl_bool addAll(const MAP& other) noexcept
		{
			return MapBaseHelper::addAll(this, other);
		}

		template <class MAP>
		sl_bool emplaceAll_NoLock(const MAP& other) noexcept
		{
			return MapBaseHelper::emplaceAll_NoLock(this, other);
		}

		template <class MAP>
		sl_bool emplaceAll(const MAP& other) noexcept
		{
			return MapBaseHelper::emplaceAll(this, other);
		}

		// unsynchronized function
		void removeAt(NODE* node) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->removeAt(node);
			}
		}

		// unsynchronized function
		sl_size removeAt(NODE* node, sl_size count) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeAt(node, count);
			}
			return 0;
		}

		// unsynchronized function
		sl_size removeRange(NODE* first, NODE* last) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeRange(first, last);
			}
			return 0;
		}

		sl_bool remove_NoLock(const KT& key, VT* outValue = sl_null) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->remove_NoLock(key, outValue);
			}
			return sl_false;
		}

		sl_bool remove(const KT& key, VT* outValue = sl_null) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->remove(key, outValue);
			}
			return sl_false;
		}

		sl_size removeItems_NoLock(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeItems_NoLock(key);
			}
			return 0;
		}

		sl_size removeItems(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeItems(key);
			}
			return 0;
		}

		List<VT> removeItemsAndReturnValues_NoLock(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeItemsAndReturnValues_NoLock(key);
			}
			return sl_null;
		}

		List<VT> removeItemsAndReturnValues(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeItemsAndReturnValues(key);
			}
			return sl_null;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool removeKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeKeyAndValue_NoLock(key, value, value_equals);
			}
			return sl_false;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool removeKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeKeyAndValue(key, value, value_equals);
			}
			return sl_false;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_size removeItemsByKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeItemsByKeyAndValue_NoLock(key, value, value_equals);
			}
			return 0;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_size removeItemsByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeItemsByKeyAndValue(key, value, value_equals);
			}
			return 0;
		}

		sl_size removeAll_NoLock() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeAll_NoLock();
			}
			return 0;
		}

		sl_size removeAll() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->removeAll();
			}
			return 0;
		}

		void shrink_NoLock() noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->shrink_NoLock();
			}
		}

		void shrink() noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->shrink();
			}
		}

		HashMap duplicate_NoLock() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->duplicate_NoLock();
			}
			return sl_null;
		}

		HashMap duplicate() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->duplicate();
			}
			return sl_null;
		}

		List<KT> getAllKeys_NoLock() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getAllKeys_NoLock();
			}
			return sl_null;
		}

		List<KT> getAllKeys() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getAllKeys();
			}
			return sl_null;
		}

		List<VT> getAllValues_NoLock() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getAllValues_NoLock();
			}
			return sl_null;
		}

		List<VT> getAllValues() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getAllValues();
			}
			return sl_null;
		}

		List< Pair<KT, VT> > toList_NoLock() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->toList_NoLock();
			}
			return sl_null;
		}

		List< Pair<KT, VT> > toList() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->toList();
			}
			return sl_null;
		}

		Ref<Object> toObject() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->toObject();
			}
			return sl_null;
		}

		Ref<Object> toObject_NoLocking() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->toObject_NoLocking();
			}
			return sl_null;
		}

		const Mutex* getLocker() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getLocker();
			}
			return sl_null;
		}

		// range-based for loop
		POSITION begin() const noexcept
		{
			CMAP* obj = ref.ptr;
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

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	class SLIB_EXPORT Atomic< HashMap<KT, VT, HASH, KEY_COMPARE> >
	{
	public:
		typedef CHashMap<KT, VT, HASH, KEY_COMPARE> CMAP;

	public:
		AtomicRef<CMAP> ref;
		SLIB_ATOMIC_REF_WRAPPER(CMAP)

	public:
		Atomic(sl_size capacityMinimum, sl_size capacityMaximum = 0) noexcept : ref(new CMAP(capacityMinimum, capacityMaximum)) {}

		template <class HASH_ARG>
		Atomic(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept : ref(new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash))) {}

		template <class HASH_ARG, class KEY_COMPARE_ARG>
		Atomic(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept : ref(new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<KEY_COMPARE_ARG>(compare))) {}

#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept : ref(new CMAP(l, capacityMinimum, capacityMaximum)) {}

		template <class HASH_ARG>
		Atomic(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept : ref(new CMAP(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash))) {}

		template <class HASH_ARG, class KEY_COMPARE_ARG>
		Atomic(const std::initializer_list< Pair<KT, VT> >& l, sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept : ref(new CMAP(l, capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<KEY_COMPARE_ARG>(compare))) {}
#endif

	public:
		template <class KEY, class VALUE, class OTHER_HASH, class OTHER_COMPARE>
		static const Atomic& from(const Atomic< HashMap<KEY, VALUE, OTHER_HASH, OTHER_COMPARE> >& other) noexcept
		{
			return *(reinterpret_cast<Atomic const*>(&other));
		}

		void initialize(sl_size capacityMinimum = 0, sl_size capacityMaximum = 0) noexcept
		{
			ref = new CMAP(capacityMinimum, capacityMaximum);
		}

		template <class HASH_ARG>
		void initialize(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash) noexcept
		{
			ref = new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash));
		}

		template <class HASH_ARG, class KEY_COMPARE_ARG>
		void initialize(sl_size capacityMinimum, sl_size capacityMaximum, HASH_ARG&& hash, KEY_COMPARE_ARG&& compare) noexcept
		{
			ref = new CMAP(capacityMinimum, capacityMaximum, Forward<HASH_ARG>(hash), Forward<KEY_COMPARE_ARG>(compare));
		}

	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic& operator=(const std::initializer_list< Pair<KT, VT> >& l) noexcept
		{
			ref = new CMAP(l);
			return *this;
		}
#endif

	public:
		template <class KEY, class VALUE>
		sl_bool put(KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			return AtomicMapBaseHelper::put(this, Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
		}

		template <class KEY, class... VALUE_ARGS>
		sl_bool add(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			return AtomicMapBaseHelper::add(this, Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
		}

		template <class KEY, class... VALUE_ARGS>
		sl_bool emplace(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			return AtomicMapBaseHelper::emplace(this, Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
		}

		template <class MAP>
		sl_bool putAll(const MAP& other) noexcept
		{
			return AtomicMapBaseHelper::putAll(this, other);
		}

		template <class MAP>
		sl_bool addAll(const MAP& other) noexcept
		{
			return AtomicMapBaseHelper::addAll(this, other);
		}

		template <class MAP>
		sl_bool emplaceAll(const MAP& other) noexcept
		{
			return AtomicMapBaseHelper::emplaceAll(this, other);
		}

	};

}

#endif

