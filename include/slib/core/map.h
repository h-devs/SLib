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

#ifndef CHECKHEADER_SLIB_CORE_MAP
#define CHECKHEADER_SLIB_CORE_MAP

#include "object.h"
#include "pair.h"
#include "red_black_tree.h"
#include "nullable.h"
#include "priv/node_position.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <initializer_list>
#endif

namespace slib
{

	template <class KT, class VT, class KEY_COMPARE>
	class CMap;

	template <class KT, class VT, class KEY_COMPARE>
	class Map;

	template < class KT, class VT, class KEY_COMPARE = Compare<KT> >
	using AtomicMap = Atomic< Map<KT, VT, KEY_COMPARE> >;


	template <class KT, class VT>
	class SLIB_EXPORT MapNode
	{
	public:
		typedef KT KEY_TYPE;
		typedef VT VALUE_TYPE;

		MapNode* parent;
		MapNode* left;
		MapNode* right;
		sl_bool flagRed;

		KT key;
		VT value;

	public:
		MapNode(const MapNode& other) = delete;

#ifdef SLIB_COMPILER_IS_GCC
		MapNode(MapNode& other) = delete;
#endif

		template <class KEY, class... VALUE_ARGS>
		MapNode(KEY&& _key, VALUE_ARGS&&... value_args) noexcept: key(Forward<KEY>(_key)), value(Forward<VALUE_ARGS>(value_args)...), parent(sl_null), left(sl_null), right(sl_null), flagRed(sl_false) {}

	public:
		MapNode* getNext() const noexcept
		{
			return reinterpret_cast<MapNode<KT, VT>*>(priv::rb_tree::Helper::getNext(const_cast<RedBlackTreeNode*>(reinterpret_cast<RedBlackTreeNode const*>(this))));
		}

		MapNode* getPrevious() const noexcept
		{
			return reinterpret_cast<MapNode<KT, VT>*>(priv::rb_tree::Helper::getPrevious(const_cast<RedBlackTreeNode*>(reinterpret_cast<RedBlackTreeNode const*>(this))));
		}

	};

	class SLIB_EXPORT CMapBase : public CRef, public Lockable
	{
		SLIB_DECLARE_OBJECT

	public:
		CMapBase();

		~CMapBase();

	};

	template < class KT, class VT, class KEY_COMPARE = Compare<KT> >
	class SLIB_EXPORT CMap : public CMapBase
	{
	public:
		typedef KT KEY_TYPE;
		typedef VT VALUE_TYPE;
		typedef Map<KT, VT, KEY_COMPARE> MAP_TYPE;
		typedef MapNode<KT, VT> NODE;
		typedef NodePosition<NODE> POSITION;

	protected:
		NODE* m_root;
		sl_size m_count;
		KEY_COMPARE m_compare;

	public:
		CMap() noexcept: m_root(sl_null), m_count(0) {}

		CMap(const KEY_COMPARE& compare) noexcept: m_root(sl_null), m_count(0), m_compare(compare) {}

		CMap(KEY_COMPARE&& compare) noexcept: m_root(sl_null), m_count(0), m_compare(Move(compare)) {}

		~CMap() noexcept
		{
			NODE* root = m_root;
			if (root) {
				RedBlackTree::freeNodes(root);
				m_root = sl_null;
				m_count = 0;
			}
		}

	public:
		CMap(const CMap& other) = delete;

		CMap& operator=(const CMap& other) = delete;

		CMap(CMap&& other) noexcept: m_root(other.m_root), m_count(other.m_count), m_compare(Move(other.m_compare))
		{
			other.m_root = sl_null;
			other.m_count = 0;
		}

		CMap& operator=(CMap&& other) noexcept
		{
			NODE* root = m_root;
			if (root) {
				RedBlackTree::freeNodes(root);
			}
			m_root = other.m_root;
			m_count = other.m_count;
			m_compare = Move(other.m_compare);
			other.m_root = sl_null;
			other.m_count = 0;
			return *this;
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		template <class KEY_COMPARE_ARG>
		CMap(const std::initializer_list< Pair<KT, VT> >& l, KEY_COMPARE_ARG&& compare) noexcept: m_root(sl_null), m_count(0), m_compare(Forward<KEY_COMPARE_ARG>(compare))
		{
			const Pair<KT, VT>* data = l.begin();
			sl_size n = l.size();
			for (sl_size i = 0; i < n; i++) {
				RedBlackTree::add(&m_root, m_count, data[i].first, compare, data[i].second);
			}
		}

		CMap(const std::initializer_list< Pair<KT, VT> >& l) noexcept: CMap(l, KEY_COMPARE()) {}
#endif

	public:
		sl_size getCount() const noexcept
		{
			return m_count;
		}

		sl_bool isEmpty() const noexcept
		{
			return !(m_count);
		}

		sl_bool isNotEmpty() const noexcept
		{
			return m_count != 0;
		}

		NODE* getFirstNode() const noexcept
		{
			return RedBlackTree::getFirstNode(m_root);
		}

		NODE* getLastNode() const noexcept
		{
			return RedBlackTree::getLastNode(m_root);
		}

		NODE* find_NoLock(const KT& key) const noexcept
		{
			return RedBlackTree::find(m_root, key, m_compare);
		}

		sl_bool find(const KT& key) const noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::find(m_root, key, m_compare) != sl_null;
		}

		// unsynchronized function
		sl_bool getEqualRange(const KT& key, NODE** pStart = sl_null, NODE** pEnd = sl_null) const noexcept
		{
			return RedBlackTree::getEqualRange(m_root, key, m_compare, pStart, pEnd);
		}

		// unsynchronized function
		void getNearest(const KT& key, NODE** pLessEqual = sl_null, NODE** pGreaterEqual = sl_null) const noexcept
		{
			RedBlackTree::getNearest(m_root, key, m_compare, pLessEqual, pGreaterEqual);
		}

		// unsynchronized function
		NODE* getLowerBound(const KT& key) const noexcept
		{
			return RedBlackTree::getLowerBound(m_root, key, m_compare);
		}

		// unsynchronized function
		NODE* getUpperBound(const KT& key) const noexcept
		{
			return RedBlackTree::getUpperBound(m_root, key, m_compare);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		NODE* findKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			return RedBlackTree::findKeyAndValue(m_root, key, m_compare, value, value_equals);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool findKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::findKeyAndValue(m_root, key, m_compare, value, value_equals) != sl_null;
		}

		// unsynchronized function
		VT* getItemPointer(const KT& key) const noexcept
		{
			NODE* node = RedBlackTree::find(m_root, key, m_compare);
			if (node) {
				return &(node->value);
			}
			return sl_null;
		}

		// unsynchronized function
		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		VT* getItemPointerByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			NODE* node = RedBlackTree::findKeyAndValue(m_root, key, m_compare, value, value_equals);
			if (node) {
				return &(node->value);
			}
			return sl_null;
		}

		sl_bool get_NoLock(const KT& key, VT* _out) const noexcept
		{
			NODE* node = RedBlackTree::find(m_root, key, m_compare);
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
			NODE* node = RedBlackTree::find(m_root, key, m_compare);
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
			NODE* node = RedBlackTree::find(m_root, key, m_compare);
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
			NODE* node = RedBlackTree::find(m_root, key, m_compare);
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
			NODE* node = RedBlackTree::find(m_root, key, m_compare);
			if (node) {
				return node->value;
			} else {
				return VT();
			}
		}

		VT getValue(const KT& key) const noexcept
		{
			ObjectLocker lock(this);
			NODE* node = RedBlackTree::find(m_root, key, m_compare);
			if (node) {
				return node->value;
			} else {
				return VT();
			}
		}

		VT getValue_NoLock(const KT& key, const VT& def) const noexcept
		{
			NODE* node = RedBlackTree::find(m_root, key, m_compare);
			if (node) {
				return node->value;
			} else {
				return def;
			}
		}

		VT getValue(const KT& key, const VT& def) const noexcept
		{
			ObjectLocker lock(this);
			NODE* node = RedBlackTree::find(m_root, key, m_compare);
			if (node) {
				return node->value;
			} else {
				return def;
			}
		}

		List<VT> getValues_NoLock(const KT& key) const noexcept
		{
			List<VT> list;
			RedBlackTree::getValues(list, m_root, key, m_compare);
			return list;
		}

		List<VT> getValues(const KT& key) const noexcept
		{
			ObjectLocker lock(this);
			List<VT> list;
			RedBlackTree::getValues(list, m_root, key, m_compare);
			return list;
		}

		template <class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		List<VT> getValuesByKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			List<VT> list;
			RedBlackTree::getValuesByKeyAndValue(list, m_root, key, m_compare, value, value_equals);
			return list;
		}

		template <class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		List<VT> getValuesByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const noexcept
		{
			ObjectLocker lock(this);
			List<VT> list;
			RedBlackTree::getValuesByKeyAndValue(list, m_root, key, m_compare, value, value_equals);
			return list;
		}

		template <class KEY, class VALUE>
		NODE* put_NoLock(KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			return RedBlackTree::put(&m_root, m_count, Forward<KEY>(key), m_compare, Forward<VALUE>(value), isInsertion);
		}

		template <class KEY, class VALUE>
		sl_bool put(KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::put(&m_root, m_count, Forward<KEY>(key), m_compare, Forward<VALUE>(value), isInsertion) != sl_null;
		}

		template <class KEY, class VALUE>
		NODE* replace_NoLock(const KEY& key, VALUE&& value) noexcept
		{
			return RedBlackTree::replace(m_root, key, m_compare, Forward<VALUE>(value));
		}

		template <class KEY, class VALUE>
		sl_bool replace(const KEY& key, VALUE&& value) noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::replace(m_root, key, m_compare, Forward<VALUE>(value)) != sl_null;
		}

		template <class KEY, class... VALUE_ARGS>
		NODE* add_NoLock(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			return RedBlackTree::add(&m_root, m_count, Forward<KEY>(key), m_compare, Forward<VALUE_ARGS>(value_args)...);
		}

		template <class KEY, class... VALUE_ARGS>
		sl_bool add(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::add(&m_root, m_count, Forward<KEY>(key), m_compare, Forward<VALUE_ARGS>(value_args)...) != sl_null;
		}

		template <class KEY, class... VALUE_ARGS>
		MapEmplaceReturn<NODE> emplace_NoLock(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			return RedBlackTree::emplace(&m_root, m_count, Forward<KEY>(key), m_compare, Forward<VALUE_ARGS>(value_args)...);
		}

		template <class KEY, class... VALUE_ARGS>
		sl_bool emplace(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::emplace(&m_root, m_count, Forward<KEY>(key), m_compare, Forward<VALUE_ARGS>(value_args)...);
		}

		template <class MAP>
		sl_bool putAll_NoLock(const MAP& other) noexcept
		{
			typename MAP::EnumHelper helper(other);
			auto node = helper.node;
			if (!node) {
				return sl_true;
			}
			if (reinterpret_cast<void*>(RedBlackTree::getFirstNode(m_root)) == reinterpret_cast<void*>(node)) {
				return sl_true;
			}
			while (node) {
				if (!(RedBlackTree::put(&m_root, m_count, node->key, m_compare, node->value, sl_null))) {
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
			if (reinterpret_cast<void*>(RedBlackTree::getFirstNode(m_root)) == reinterpret_cast<void*>(node)) {
				return sl_true;
			}
			MultipleMutexLocker lock(getLocker(), helper.mutex);
			while (node) {
				if (!(RedBlackTree::put(&m_root, m_count, node->key, m_compare, node->value, sl_null))) {
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
			if (reinterpret_cast<void*>(RedBlackTree::getFirstNode(m_root)) == reinterpret_cast<void*>(node)) {
				return;
			}
			while (node) {
				RedBlackTree::replace(m_root, node->key, m_compare, node->value);
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
			if (reinterpret_cast<void*>(RedBlackTree::getFirstNode(m_root)) == reinterpret_cast<void*>(node)) {
				return;
			}
			MultipleMutexLocker lock(getLocker(), helper.mutex);
			while (node) {
				RedBlackTree::replace(m_root, node->key, m_compare, node->value);
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
			if (reinterpret_cast<void*>(RedBlackTree::getFirstNode(m_root)) == reinterpret_cast<void*>(node)) {
				return sl_false;
			}
			while (node) {
				if (!(RedBlackTree::add(&m_root, m_count, node->key, m_compare, node->value))) {
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
			if (reinterpret_cast<void*>(RedBlackTree::getFirstNode(m_root)) == reinterpret_cast<void*>(node)) {
				return sl_false;
			}
			MultipleMutexLocker lock(getLocker(), helper.mutex);
			while (node) {
				if (!(RedBlackTree::add(&m_root, m_count, node->key, m_compare, node->value))) {
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
			if (reinterpret_cast<void*>(RedBlackTree::getFirstNode(m_root)) == reinterpret_cast<void*>(node)) {
				return sl_true;
			}
			while (node) {
				MapEmplaceReturn<NODE> ret = RedBlackTree::emplace(&m_root, m_count, node->key, m_compare, node->value);
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
			if (reinterpret_cast<void*>(RedBlackTree::getFirstNode(m_root)) == reinterpret_cast<void*>(node)) {
				return sl_true;
			}
			MultipleMutexLocker lock(getLocker(), helper.mutex);
			while (node) {
				MapEmplaceReturn<NODE> ret = RedBlackTree::emplace(&m_root, m_count, node->key, m_compare, node->value);
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
			RedBlackTree::removeNode(&m_root, m_count, node);
		}

		// unsynchronized function
		sl_size removeAt(NODE* node, sl_size count) noexcept
		{
			return RedBlackTree::removeNodes(&m_root, m_count, node, count);
		}

		// unsynchronized function
		sl_size removeRange(NODE* first, NODE* last) noexcept
		{
			return RedBlackTree::removeRange(&m_root, m_count, first, last);
		}

		sl_bool remove_NoLock(const KT& key, VT* outValue = sl_null) noexcept
		{
			return RedBlackTree::remove(&m_root, m_count, key, m_compare, outValue);
		}

		sl_bool remove(const KT& key, VT* outValue = sl_null) noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::remove(&m_root, m_count, key, m_compare, outValue);
		}

		sl_size removeItems_NoLock(const KT& key) noexcept
		{
			return RedBlackTree::removeItems(&m_root, m_count, key, m_compare);
		}

		sl_size removeItems(const KT& key) noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::removeItems(&m_root, m_count, key, m_compare);
		}

		List<VT> removeItemsAndReturnValues_NoLock(const KT& key) noexcept
		{
			List<VT> list;
			RedBlackTree::removeItemsAndReturnValues(list, &m_root, m_count, key, m_compare);
			return list;
		}

		List<VT> removeItemsAndReturnValues(const KT& key) noexcept
		{
			ObjectLocker lock(this);
			List<VT> list;
			RedBlackTree::removeItemsAndReturnValues(list, &m_root, m_count, key, m_compare);
			return list;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool removeKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			return RedBlackTree::removeKeyAndValue(&m_root, m_count, key, m_compare, value, value_equals);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool removeKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::removeKeyAndValue(&m_root, m_count, key, m_compare, value, value_equals);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_size removeItemsByKeyAndValue_NoLock(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			return RedBlackTree::removeItemsByKeyAndValue(&m_root, m_count, key, m_compare, value, value_equals);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_size removeItemsByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) noexcept
		{
			ObjectLocker lock(this);
			return RedBlackTree::removeItemsByKeyAndValue(&m_root, m_count, key, m_compare, value, value_equals);
		}

		sl_size removeAll_NoLock() noexcept
		{
			NODE* root = m_root;
			sl_size count = m_count;
			if (root) {
				RedBlackTree::freeNodes(root);
				m_root = sl_null;
			}
			m_count = 0;
			return count;
		}

		sl_size removeAll() noexcept
		{
			NODE* root;
			sl_size count;
			{
				ObjectLocker lock(this);
				root = m_root;
				count = m_count;
				m_root = sl_null;
				m_count = 0;
			}
			if (root) {
				RedBlackTree::freeNodes(root);
			}
			return count;
		}

		sl_bool copyFrom_NoLock(const CMap& other) noexcept
		{
			if (this == &other) {
				return sl_true;
			}
			NODE* root = m_root;
			if (root) {
				RedBlackTree::freeNodes(root);
			}
			root = other.m_root;
			if (root) {
				root = RedBlackTree::duplicateNode(root);
				if (root) {
					m_root = root;
					m_count = other.m_count;
					return sl_true;
				} else {
					m_root = sl_null;
					m_count = 0;
					return sl_false;
				}
			} else {
				m_root = sl_null;
				m_count = 0;
				return sl_true;
			}
		}

		sl_bool copyFrom(const CMap& other) noexcept
		{
			if (this == &other) {
				return sl_true;
			}
			MultipleObjectsLocker lock(this, &other);
			return copyFrom_NoLock(other);
		}

		CMap* duplicate_NoLock() const noexcept
		{
			NODE* root = m_root;
			if (root) {
				NODE* other = RedBlackTree::duplicateNode(root);
				if (other) {
					CMap* ret = new CMap;
					if (ret) {
						ret->m_root = other;
						ret->m_count = m_count;
						return ret;
					} else {
						RedBlackTree::freeNodes(other);
					}
				}
			}
			return sl_null;
		}

		CMap* duplicate() const noexcept
		{
			ObjectLocker lock(this);
			return duplicate_NoLock();
		}

		List<KT> getAllKeys_NoLock() const noexcept
		{
			List<KT> ret;
			NODE* node = RedBlackTree::getFirstNode(m_root);
			while (node) {
				ret.add_NoLock(node->key);
				node = node->getNext();
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
			NODE* node = RedBlackTree::getFirstNode(m_root);
			while (node) {
				ret.add_NoLock(node->value);
				node = node->getNext();
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
			NODE* node = RedBlackTree::getFirstNode(m_root);
			while (node) {
				ret.add_NoLock(Pair<KT, VT>(node->key, node->value));
				node = node->getNext();
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
			return RedBlackTree::getFirstNode(m_root);
		}

		POSITION end() const noexcept
		{
			return sl_null;
		}

	public:
		class EnumLockHelper
		{
		public:
			EnumLockHelper(const CMap& map) noexcept
			{
				node = map.getFirstNode();
				mutex = map.getLocker();
			}
		public:
			NODE* node;
			Mutex* mutex;
		};

		class EnumHelper
		{
		public:
			EnumHelper(const CMap& map) noexcept
			{
				node = map.getFirstNode();
			}
		public:
			NODE* node;
		};

	};


	class MapBaseHelper
	{
	public:
		template <class THIS_MAP, class KEY, class VALUE>
		static typename THIS_MAP::NODE* put_NoLock(THIS_MAP* thiz, KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->put_NoLock(Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
			} else {
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					if (isInsertion) {
						*isInsertion = sl_true;
					}
					return obj->add_NoLock(Forward<KEY>(key), Forward<VALUE>(value));
				}
			}
			return sl_null;
		}

		template <class THIS_MAP, class KEY, class VALUE>
		static sl_bool put(THIS_MAP* thiz, KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->put(Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->put(Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
				}
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					lock.unlock();
					return obj->put(Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class KEY, class... VALUE_ARGS>
		static typename THIS_MAP::NODE* add_NoLock(THIS_MAP* thiz, KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->add_NoLock(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
			} else {
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					return obj->add_NoLock(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				}
			}
			return sl_null;
		}

		template <class THIS_MAP, class KEY, class... VALUE_ARGS>
		static sl_bool add(THIS_MAP* thiz, KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->add(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->add(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				}
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					lock.unlock();
					return obj->add(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class KEY, class... VALUE_ARGS>
		static MapEmplaceReturn<typename THIS_MAP::NODE> emplace_NoLock(THIS_MAP* thiz, KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			typedef typename THIS_MAP::NODE NODE;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->emplace_NoLock(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
			} else {
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					NODE* node = obj->add_NoLock(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
					if (node) {
						return MapEmplaceReturn<NODE>(sl_true, node);
					} else {
						return sl_null;
					}
				}
			}
			return sl_null;
		}

		template <class THIS_MAP, class KEY, class... VALUE_ARGS>
		static sl_bool emplace(THIS_MAP* thiz, KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->emplace(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->emplace(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				}
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					lock.unlock();
					return obj->emplace(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class MAP>
		static sl_bool putAll_NoLock(THIS_MAP* thiz, const MAP& other) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->putAll_NoLock(other);
			} else {
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					return obj->putAll_NoLock(other);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class MAP>
		static sl_bool putAll(THIS_MAP* thiz, const MAP& other) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->putAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->putAll(other);
				}
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					lock.unlock();
					return obj->putAll(other);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class MAP>
		static sl_bool addAll_NoLock(THIS_MAP* thiz, const MAP& other) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->addAll_NoLock(other);
			} else {
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					return obj->addAll_NoLock(other);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class MAP>
		static sl_bool addAll(THIS_MAP* thiz, const MAP& other) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->addAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->addAll(other);
				}
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					lock.unlock();
					return obj->addAll(other);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class MAP>
		static sl_bool emplaceAll_NoLock(THIS_MAP* thiz, const MAP& other) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				return obj->emplaceAll_NoLock(other);
			} else {
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					return obj->emplaceAll_NoLock(other);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class MAP>
		static sl_bool emplaceAll(THIS_MAP* thiz, const MAP& other) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			CMAP* obj = thiz->ref.ptr;
			if (obj) {
				obj->emplaceAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->emplaceAll(other);
				}
				obj = new CMAP;
				if (obj) {
					thiz->ref = obj;
					lock.unlock();
					return obj->emplaceAll(other);
				}
			}
			return sl_false;
		}

	public:
		template <class MAP_TYPE>
		class EnumLockHelper
		{
		public:
			EnumLockHelper(const MAP_TYPE& map) noexcept
			{
				auto obj = map.ref.ptr;
				if (obj) {
					node = obj->getFirstNode();
					mutex = obj->getLocker();
				} else {
					node = sl_null;
					mutex = sl_null;
				}
			}

		public:
			typename MAP_TYPE::NODE* node;
			Mutex* mutex;
		};

		template <class MAP_TYPE>
		class EnumHelper
		{
		public:
			EnumHelper(const MAP_TYPE& map) noexcept
			{
				node = map.getFirstNode();
			}

		public:
			typename MAP_TYPE::NODE* node;
		};

	};

	template < class KT, class VT, class KEY_COMPARE = Compare<KT> >
	class SLIB_EXPORT Map
	{
	public:
		typedef CMap<KT, VT, KEY_COMPARE> CMAP;
		typedef KT KEY_TYPE;
		typedef VT VALUE_TYPE;
		typedef MapNode<KT, VT> NODE;
		typedef NodePosition<NODE> POSITION;
		typedef typename MapBaseHelper::EnumLockHelper<Map> EnumLockHelper;
		typedef typename MapBaseHelper::EnumHelper<Map> EnumHelper;

	public:
		Ref<CMAP> ref;
		SLIB_REF_WRAPPER(Map, CMAP)

	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		Map(const std::initializer_list< Pair<KT, VT> >& l) noexcept : ref(new CMAP(l)) {}

		template <class KEY_COMPARE_ARG>
		Map(const std::initializer_list< Pair<KT, VT> >& l, KEY_COMPARE_ARG&& compare) noexcept : ref(new CMAP(l, Forward<KEY_COMPARE_ARG>(compare))) {}
#endif

	public:
		static Map create() noexcept
		{
			return new CMAP();
		}

		static Map create(const KEY_COMPARE& compare) noexcept
		{
			return new CMAP(compare);
		}

		static Map create(KEY_COMPARE&& compare) noexcept
		{
			return new CMAP(Move(compare));
		}

		static Map create(Object* object);

#ifdef SLIB_SUPPORT_STD_TYPES
		static Map create(const std::initializer_list< Pair<KT, VT> >& l) noexcept
		{
			return new CMAP(l);
		}

		template <class KEY_COMPARE_ARG>
		static Map create(const std::initializer_list< Pair<KT, VT> >& l, KEY_COMPARE_ARG&& compare) noexcept
		{
			return new CMAP(l, Forward<KEY_COMPARE_ARG>(compare));
		}
#endif

		template <class KEY, class VALUE, class OTHER_COMPARE>
		static const Map& from(const Map<KEY, VALUE, OTHER_COMPARE>& other) noexcept
		{
			return *(reinterpret_cast<Map const*>(&other));
		}

		void initialize() noexcept
		{
			ref = new CMAP();
		}

		void initialize(const KEY_COMPARE& compare) noexcept
		{
			ref = new CMAP(compare);
		}

		void initialize(KEY_COMPARE&& compare) noexcept
		{
			ref = new CMAP(Move(compare));
		}

	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		Map& operator=(const std::initializer_list< Pair<KT, VT> >& l) noexcept
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
		sl_bool getEqualRange(const KT& key, NODE** pStart = sl_null, NODE** pEnd = sl_null) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getEqualRange(key, pStart, pEnd);
			}
			return sl_false;
		}

		// unsynchronized function
		void getNearest(const KT& key, NODE** pLessEqual = sl_null, NODE** pGreaterEqual = sl_null) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				obj->getNearest(key, pLessEqual, pGreaterEqual);
			} else {
				if (pLessEqual) {
					*pLessEqual = sl_null;
				}
				if (pGreaterEqual) {
					*pGreaterEqual = sl_null;
				}
			}
		}

		// unsynchronized function
		NODE* getLowerBound(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getLowerBound(key);
			}
			return sl_null;
		}

		// unsynchronized function
		NODE* getUpperBound(const KT& key) const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->getUpperBound(key);
			}
			return sl_null;
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

		sl_bool get(const KT& key, VT* _out = sl_null) const noexcept
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

		Map duplicate_NoLock() const noexcept
		{
			CMAP* obj = ref.ptr;
			if (obj) {
				return obj->duplicate_NoLock();
			}
			return sl_null;
		}

		Map duplicate() const noexcept
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


	class AtomicMapBaseHelper
	{
	public:
		template <class THIS_MAP, class KEY, class VALUE>
		static sl_bool put(THIS_MAP* thiz, KEY&& key, VALUE&& value, sl_bool* isInsertion = sl_null) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			Ref<CMAP> obj(thiz->ref);
			if (obj.isNotNull()) {
				return obj->put(Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->put(Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
				}
				obj = new CMAP;
				if (obj.isNotNull()) {
					thiz->ref = obj;
					lock.unlock();
					return obj->put(Forward<KEY>(key), Forward<VALUE>(value), isInsertion);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class KEY, class... VALUE_ARGS>
		static sl_bool add(THIS_MAP* thiz, KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			Ref<CMAP> obj(thiz->ref);
			if (obj.isNotNull()) {
				return obj->add(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->add(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				}
				obj = new CMAP;
				if (obj.isNotNull()) {
					thiz->ref = obj;
					lock.unlock();
					return obj->add(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class KEY, class... VALUE_ARGS>
		static sl_bool emplace(THIS_MAP* thiz, KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			Ref<CMAP> obj(thiz->ref);
			if (obj.isNotNull()) {
				return obj->emplace(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->emplace(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				}
				obj = new CMAP;
				if (obj.isNotNull()) {
					thiz->ref = obj;
					lock.unlock();
					return obj->emplace(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class MAP>
		static sl_bool putAll(THIS_MAP* thiz, const MAP& other) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			Ref<CMAP> obj(thiz->ref);
			if (obj.isNotNull()) {
				return obj->putAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->putAll(other);
				}
				obj = new CMAP;
				if (obj.isNotNull()) {
					thiz->ref = obj;
					lock.unlock();
					return obj->putAll(other);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class MAP>
		static sl_bool addAll(THIS_MAP* thiz, const MAP& other) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			Ref<CMAP> obj(thiz->ref);
			if (obj.isNotNull()) {
				return obj->addAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->addAll(other);
				}
				obj = new CMAP;
				if (obj.isNotNull()) {
					thiz->ref = obj;
					lock.unlock();
					return obj->addAll(other);
				}
			}
			return sl_false;
		}

		template <class THIS_MAP, class MAP>
		static sl_bool emplaceAll(THIS_MAP* thiz, const MAP& other) noexcept
		{
			typedef typename THIS_MAP::CMAP CMAP;
			Ref<CMAP> obj(thiz->ref);
			if (obj.isNotNull()) {
				return obj->emplaceAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForMap::get(thiz));
				obj = thiz->ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->emplaceAll(other);
				}
				obj = new CMAP;
				if (obj.isNotNull()) {
					thiz->ref = obj;
					lock.unlock();
					return obj->emplaceAll(other);
				}
			}
			return sl_false;
		}

	};

	template <class KT, class VT, class KEY_COMPARE>
	class SLIB_EXPORT Atomic< Map<KT, VT, KEY_COMPARE> >
	{
	public:
		typedef CMap<KT, VT, KEY_COMPARE> CMAP;

	public:
		AtomicRef<CMAP> ref;
		SLIB_ATOMIC_REF_WRAPPER(CMAP)

	public:
#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic(const std::initializer_list< Pair<KT, VT> >& l) noexcept : ref(new CMAP(l)) {}

		template <class KEY_COMPARE_ARG>
		Atomic(const std::initializer_list< Pair<KT, VT> >& l, KEY_COMPARE_ARG&& compare) noexcept : ref(new CMAP(l, Forward<KEY_COMPARE_ARG>(compare))) {}
#endif

	public:
		template <class KEY, class VALUE, class OTHER_COMPARE>
		static const Atomic& from(const Atomic< Map<KEY, VALUE, OTHER_COMPARE> >& other) noexcept
		{
			return *(reinterpret_cast<Atomic const*>(&other));
		}

		void initialize() noexcept
		{
			ref = new CMAP;
		}

		void initialize(const KEY_COMPARE& compare) noexcept
		{
			ref = new CMAP(compare);
		}

		void initialize(KEY_COMPARE&& compare) noexcept
		{
			ref = new CMAP(Move(compare));
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
