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

#ifndef CHECKHEADER_SLIB_CORE_HASH_TABLE
#define CHECKHEADER_SLIB_CORE_HASH_TABLE

#include "map_common.h"
#include "hash.h"
#include "list.h"

namespace slib
{

	template <class KT, class VT>
	class HashTableNode
	{
	public:
		HashTableNode* next;
		sl_size hash;
		KT key;
		VT value;

	public:
		template <class KEY, class... VALUE_ARGS>
		HashTableNode(KEY&& _key, VALUE_ARGS&&... value_args) noexcept: key(Forward<KEY>(_key)), value(Forward<VALUE_ARGS>(value_args)...) {}

	};

	template <class KT, class VT>
	struct HashTableStruct
	{
		HashTableNode<KT, VT>** nodes;
		sl_size count;
		sl_size capacity;
		sl_size capacityMinimum;
		sl_size capacityMaximum;
		sl_size thresholdDown;
		sl_size thresholdUp;
	};


	class HashTableNodeBase
	{
	public:
		HashTableNodeBase* next;
		sl_size hash;
	};

	struct HashTableStructBase
	{
		HashTableNodeBase** nodes;
		sl_size count;
		sl_size capacity;
		sl_size capacityMinimum;
		sl_size capacityMaximum;
		sl_size thresholdDown;
		sl_size thresholdUp;
	};

	namespace priv
	{
		namespace hash_table
		{

			class Helper
			{
			public:
				typedef HashTableNodeBase NODE;
				typedef HashTableStructBase TABLE;

			public:
				static void fixCapacityRange(TABLE* table) noexcept;

				static void updateThresholds(TABLE* table) noexcept;

				static void initialize(TABLE* table, sl_size capacityMinimum, sl_size capacityMaximum) noexcept;

				static void move(TABLE* dst, TABLE* src) noexcept;

				static void clear(TABLE* table) noexcept;

				static void setMinimumCapacity(TABLE* table, sl_size capacity) noexcept;

				static void setMaximumCapacity(TABLE* table, sl_size capacity) noexcept;

				static sl_bool validateNodes(TABLE* table) noexcept;

				static sl_bool reallocNodes(TABLE* table, sl_size capacity) noexcept;

				static void expand(TABLE* table) noexcept;

				static void shrink(TABLE* table) noexcept;

				template <class KT, class VT>
				static void free(HashTableStruct<KT, VT>* table) noexcept
				{
					HashTableNode<KT, VT>** nodes = table->nodes;
					if (nodes) {
						sl_size capacity = table->capacity;
						for (sl_size i = 0; i < capacity; i++) {
							HashTableNode<KT, VT>* node = nodes[i];
							while (node) {
								HashTableNode<KT, VT>* next = node->next;
								delete node;
								node = next;
							}
						}
						Base::freeMemory(nodes);
					}
				}

			};

		}
	}


	template <class KT, class VT>
	class SLIB_EXPORT HashTablePosition
	{
	public:
		typedef HashTableNode<KT, VT> NODE;

	public:
		HashTablePosition(NODE** _entry, NODE** _last_entry, NODE* _node) noexcept
		{
			entry = _entry;
			last_entry = _last_entry;
			next = _node;
			++(*this);
		}

		HashTablePosition(const HashTablePosition& other) = default;

	public:
		HashTablePosition& operator=(const HashTablePosition& other) = default;

		NODE& operator*() const noexcept
		{
			return *node;
		}

		sl_bool operator==(const HashTablePosition& other) const noexcept
		{
			return node == other.node;
		}

		sl_bool operator!=(const HashTablePosition& other) const noexcept
		{
			return node != other.node;
		}

		HashTablePosition& operator++() noexcept
		{
			node = next;
			if (node) {
				next = node->next;
				while (!next) {
					entry++;
					if (entry == last_entry) {
						break;
					}
					next = *entry;
				}
			} else {
				next = sl_null;
			}
			return *this;
		}

	public:
		NODE** entry;
		NODE** last_entry;
		NODE* node;
		NODE* next;

	};


	template < class KT, class VT, class HASH = Hash<KT>, class KEY_EQUALS = Equals<KT> >
	class SLIB_EXPORT HashTable
	{
	public:
		typedef KT KEY_TYPE;
		typedef VT VALUE_TYPE;
		typedef HashTableNode<KT, VT> NODE;

	public:
		HashTable(sl_size capacityMinimum = 0, sl_size capacityMaximum = 0, const HASH& hash = HASH(), const KEY_EQUALS& equals = KEY_EQUALS()) noexcept: m_hash(hash), m_equals(equals)
		{
			priv::hash_table::Helper::initialize(reinterpret_cast<HashTableStructBase*>(&m_table), capacityMinimum, capacityMaximum);
		}

		HashTable(const HashTable& other) = delete;

		HashTable(HashTable&& other) noexcept: m_hash(Move(other.m_hash)), m_equals(Move(other.m_equals))
		{
			priv::hash_table::Helper::move(reinterpret_cast<HashTableStructBase*>(&m_table), reinterpret_cast<HashTableStructBase*>(&(other.m_table)));
		}

		~HashTable() noexcept
		{
			priv::hash_table::Helper::free(&m_table);
		}

	public:
		HashTable& operator=(const HashTable& other) = delete;

		HashTable& operator=(HashTable&& other) noexcept
		{
			priv::hash_table::Helper::free(&m_table);
			priv::hash_table::Helper::move(reinterpret_cast<HashTableStructBase*>(&m_table), reinterpret_cast<HashTableStructBase*>(&(other.m_table)));
			m_hash = Move(other.m_hash);
			m_equals = Move(other.m_equals);
			return *this;
		}

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
			return m_table.count > 0;
		}

		sl_size getCapacity() const noexcept
		{
			return m_table.capacity;
		}

		sl_size getMinimumCapacity() const noexcept
		{
			return m_table.capacityMinimum;
		}

		void setMinimumCapacity(sl_size capacity) noexcept
		{
			priv::hash_table::Helper::setMinimumCapacity(reinterpret_cast<HashTableStructBase*>(&m_table), capacity);
		}

		sl_size getMaximumCapacity() const noexcept
		{
			return m_table.capacityMaximum;
		}

		void setMaximumCapacity(sl_size capacity) noexcept
		{
			priv::hash_table::Helper::setMaximumCapacity(reinterpret_cast<HashTableStructBase*>(&m_table), capacity);
		}

		NODE* find(const KT& key) const noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return sl_null;
			}
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);
			NODE* node = m_table.nodes[index];
			while (node) {
				if (node->hash == hash && m_equals(node->key, key)) {
					return node;
				}
				node = node->next;
			}
			return sl_null;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		NODE* findKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return sl_null;
			}
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);
			NODE* node = m_table.nodes[index];
			while (node) {
				if (node->hash == hash && m_equals(node->key, key) && value_equals(node->value, value)) {
					return node;
				}
				node = node->next;
			}
			return sl_null;
		}

		VT* getItemPointer(const KT& key) const noexcept
		{
			NODE* node = find(key);
			if (node) {
				return &(node->value);
			}
			return sl_null;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		VT* getItemPointerByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			NODE* node = findKeyAndValue(key, value, value_equals);
			if (node) {
				return &(node->value);
			}
			return sl_null;
		}

		sl_bool get(const KT& key, VT* outValue = sl_null) const noexcept
		{
			NODE* node = find(key);
			if (node) {
				if (outValue) {
					*outValue = node->value;
				}
				return sl_true;
			}
			return sl_false;
		}

		VT getValue(const KT& key) const noexcept
		{
			NODE* node = find(key);
			if (node) {
				return node->value;
			} else {
				return VT();
			}
		}

		VT getValue(const KT& key, const VT& def) const noexcept
		{
			NODE* node = find(key);
			if (node) {
				return node->value;
			}
			return def;
		}

		List<VT> getValues(const KT& key) const noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return sl_null;
			}
			List<VT> ret;
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);
			NODE* node = m_table.nodes[index];
			while (node) {
				if (node->hash == hash && m_equals(node->key, key)) {
					ret.add_NoLock(node->value);
				}
				node = node->next;
			}
			return ret;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		List<VT> getValuesByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return sl_null;
			}
			List<VT> ret;
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);
			NODE* node = m_table.nodes[index];
			while (node) {
				if (node->hash == hash && m_equals(node->key, key) && value_equals(node->value, value)) {
					ret.add_NoLock(node->value);
				}
				node = node->next;
			}
			return ret;
		}

		template <class KEY, class VALUE>
		NODE* put(KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			if (!(priv::hash_table::Helper::validateNodes(reinterpret_cast<HashTableStructBase*>(&m_table)))) {
				return sl_null;
			}

			sl_size capacity = m_table.capacity;
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);

			NODE** nodes = m_table.nodes;
			NODE* node = nodes[index];
			while (node) {
				if (node->hash == hash && m_equals(node->key, key)) {
					node->value = Forward<VALUE>(value);
					if (isInsertion) {
						*isInsertion = sl_false;
					}
					return node;
				}
				node = node->next;
			}

			node = new NODE(Forward<KEY>(key), Forward<VALUE>(value));
			if (node) {
				(m_table.count)++;
				node->hash = hash;
				node->next = nodes[index];
				nodes[index] = node;
				priv::hash_table::Helper::expand(reinterpret_cast<HashTableStructBase*>(&m_table));
				if (isInsertion) {
					*isInsertion = sl_true;
				}
				return node;
			}
			if (isInsertion) {
				*isInsertion = sl_false;
			}
			return sl_null;
		}

		template <class KEY, class VALUE>
		NODE* replace(const KEY& key, VALUE&& value) noexcept
		{
			NODE* node = find(key);
			if (node) {
				node->value = Forward<VALUE>(value);
				return node;
			}
			return sl_null;
		}

		template <class KEY, class... VALUE_ARGS>
		NODE* add(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			if (!(priv::hash_table::Helper::validateNodes(reinterpret_cast<HashTableStructBase*>(&m_table)))) {
				return sl_null;
			}
			NODE* node = new NODE(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
			if (node) {
				sl_size capacity = m_table.capacity;
				sl_size hash = m_hash(key);
				sl_size index = hash & (capacity - 1);
				NODE** nodes = m_table.nodes;
				(m_table.count)++;
				node->hash = hash;
				node->next = nodes[index];
				nodes[index] = node;
				priv::hash_table::Helper::expand(reinterpret_cast<HashTableStructBase*>(&m_table));
				return node;
			}
			return sl_null;
		}

		template <class KEY, class... VALUE_ARGS>
		MapEmplaceReturn<NODE> emplace(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			if (!(priv::hash_table::Helper::validateNodes(reinterpret_cast<HashTableStructBase*>(&m_table)))) {
				return sl_null;
			}

			sl_size capacity = m_table.capacity;
			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);

			NODE** nodes = m_table.nodes;
			NODE* node = nodes[index];
			while (node) {
				if (node->hash == hash && m_equals(node->key, key)) {
					return MapEmplaceReturn< HashTableNode<KT, VT> >(sl_false, node);
				}
				node = node->next;
			}

			node = new NODE(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
			if (node) {
				(m_table.count)++;
				node->hash = hash;
				node->next = nodes[index];
				nodes[index] = node;
				priv::hash_table::Helper::expand(reinterpret_cast<HashTableStructBase*>(&m_table));
				return MapEmplaceReturn< HashTableNode<KT, VT> >(sl_true, node);
			}
			return sl_null;
		}

		sl_bool removeAt(const NODE* nodeRemove) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return sl_null;
			}

			sl_size hash = nodeRemove->hash;
			sl_size index = hash & (capacity - 1);

			NODE** link = m_table.nodes + index;
			NODE* node = *link;
			while (node) {
				NODE* next = node->next;
				if (node == nodeRemove) {
					*link = next;
					(m_table.count)--;
					delete node;
					return sl_true;
				} else {
					link = &(node->next);
				}
				node = next;
			}
			return sl_false;
		}

		sl_bool remove(const KT& key, VT* outValue = sl_null) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return sl_false;
			}

			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);

			NODE** link = m_table.nodes + index;
			NODE* node = *link;
			while (node) {
				NODE* next = node->next;
				if (node->hash == hash && m_equals(node->key, key)) {
					*link = next;
					(m_table.count)--;
					if (outValue) {
						*outValue = Move(node->value);
					}
					delete node;
					return sl_true;
				} else {
					link = &(node->next);
				}
				node = next;
			}
			return sl_false;
		}

		sl_size removeItems(const KT& key) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return 0;
			}

			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);

			NODE** link = m_table.nodes + index;
			NODE* node = *link;
			NODE* nodeDelete = sl_null;
			NODE** linkDelete = &nodeDelete;
			while ((node = *link)) {
				NODE* next = node->next;
				if (node->hash == hash && m_equals(node->key, key)) {
					*link = next;
					*linkDelete = node;
					node->next = sl_null;
					linkDelete = &(node->next);
				} else {
					link = &(node->next);
				}
				node = next;
			}
			if (!nodeDelete) {
				return 0;
			}
			sl_size nDelete = 0;
			while (nodeDelete) {
				node = nodeDelete;
				nodeDelete = nodeDelete->next;
				delete node;
				nDelete++;
			}
			(m_table.count) -= nDelete;
			return nDelete;
		}

		List<VT> removeItemsAndReturnValues(const KT& key) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return sl_null;
			}

			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);

			List<VT> ret;
			NODE** link = m_table.nodes + index;
			NODE* node = *link;
			NODE* nodeDelete = sl_null;
			NODE** linkDelete = &nodeDelete;
			while ((node = *link)) {
				NODE* next = node->next;
				if (node->hash == hash && m_equals(node->key, key)) {
					*link = next;
					ret.add_NoLock(Move(node->value));
					*linkDelete = node;
					node->next = sl_null;
					linkDelete = &(node->next);
				} else {
					link = &(node->next);
				}
				node = next;
			}
			if (!nodeDelete) {
				return sl_null;
			}
			sl_size nDelete = 0;
			while (nodeDelete) {
				node = nodeDelete;
				nodeDelete = nodeDelete->next;
				delete node;
				nDelete++;
			}
			(m_table.count) -= nDelete;
			return ret;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool removeKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return sl_false;
			}

			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);

			NODE** link = m_table.nodes + index;
			NODE* node = *link;
			while (node) {
				NODE* next = node->next;
				if (node->hash == hash && m_equals(node->key, key) && value_equals(node->value, value)) {
					*link = next;
					(m_table.count)--;
					delete node;
					return sl_true;
				} else {
					link = &(node->next);
				}
				node = next;
			}
			return sl_false;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_size removeItemsByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return 0;
			}

			sl_size hash = m_hash(key);
			sl_size index = hash & (capacity - 1);

			NODE** link = m_table.nodes + index;
			NODE* node = *link;
			NODE* nodeDelete = sl_null;
			NODE** linkDelete = &nodeDelete;
			while (node) {
				NODE* next = node->next;
				if (node->hash == hash && m_equals(node->key, key) && value_equals(node->value, value)) {
					*link = next;
					*linkDelete = node;
					node->next = sl_null;
					linkDelete = &(node->next);
				} else {
					link = &(node->next);
				}
				node = next;
			}
			if (!nodeDelete) {
				return 0;
			}
			sl_size nDelete = 0;
			while (nodeDelete) {
				node = nodeDelete;
				nodeDelete = nodeDelete->next;
				delete node;
				nDelete++;
			}
			(m_table.count) -= nDelete;
			return nDelete;
		}

		sl_size removeAll() noexcept
		{
			sl_size capacity = m_table.capacity;
			if (!capacity) {
				return 0;
			}
			sl_size count = m_table.count;
			priv::hash_table::Helper::free(&m_table);
			priv::hash_table::Helper::initialize(reinterpret_cast<HashTableStructBase*>(&m_table), m_table.capacityMinimum, m_table.capacityMaximum);
			return count;
		}

		void shrink() noexcept
		{
			priv::hash_table::Helper::shrink(reinterpret_cast<HashTableStructBase*>(&m_table));
		}

		sl_bool copyFrom(const HashTable<KT, VT, HASH, KEY_EQUALS>& other) noexcept
		{
			priv::hash_table::Helper::free(&m_table);
			m_hash = other.m_hash;
			m_equals = other.m_equals;
			sl_size capacity = other.m_table.capacity;
			priv::hash_table::Helper::initialize(reinterpret_cast<HashTableStructBase*>(&m_table), other.m_table.capacityMinimum, other.m_table.capacityMaximum);
			if (!capacity) {
				return sl_true;
			}
			if (!(priv::hash_table::Helper::reallocNodes(reinterpret_cast<HashTableStructBase*>(&m_table), capacity))) {
				return sl_false;
			}
			NODE** nodesTarget = m_table.nodes;
			Base::zeroMemory(nodesTarget, capacity*sizeof(NODE*));
			NODE** nodesSource = other.m_table.nodes;
			for (sl_size i = 0; i < capacity; i++) {
				NODE* nodeSource = nodesSource[i];
				if (nodeSource) {
					NODE** link = &(nodesTarget[i]);
					do {
						NODE* nodeTarget = new NODE(nodeSource->key, nodeSource->value);
						*link = nodeTarget;
						if (!nodeTarget) {
							priv::hash_table::Helper::free(&m_table);
							priv::hash_table::Helper::initialize(reinterpret_cast<HashTableStructBase*>(&m_table), other.m_table.capacityMinimum, other.m_table.capacityMaximum);
							return sl_false;
						}
						nodeTarget->hash = nodeSource->hash;
						link = &(nodeTarget->next);
						nodeSource = nodeSource->next;
					} while (nodeSource);
				}
			}
			m_table.count = other.m_table.count;
			return sl_true;
		}

		// range-based for loop
		HashTablePosition<KT, VT> begin() const noexcept
		{
			NODE** nodes = m_table.nodes;
			sl_size capacity = m_table.capacity;
			for (sl_size i = 0; i < capacity; i++) {
				NODE* node = nodes[i];
				if (node) {
					return HashTablePosition<KT, VT>(nodes + i, nodes + capacity, node);
				}
			}
			return HashTablePosition<KT, VT>(sl_null, sl_null, sl_null);
		}

		HashTablePosition<KT, VT> end() const noexcept
		{
			return HashTablePosition<KT, VT>(sl_null, sl_null, sl_null);
		}

	private:
		typedef HashTableStruct<KT, VT> Table;

		Table m_table;
		HASH m_hash;
		KEY_EQUALS m_equals;

	};

}

#endif
