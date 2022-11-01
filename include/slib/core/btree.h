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

#ifndef CHECKHEADER_SLIB_CORE_BTREE
#define CHECKHEADER_SLIB_CORE_BTREE

#include "search.h"
#include "list.h"
#include "new_helper.h"

#define SLIB_BTREE_DEFAULT_ORDER 16

namespace slib
{

	class SLIB_EXPORT BTreeNode
	{
	public:
		sl_uint64 position;

	public:
		BTreeNode() noexcept
		: position(0)
		{}

		BTreeNode(sl_null_t) noexcept
		: position(0)
		{}

		BTreeNode(sl_uint64 _position) noexcept
		: position(_position)
		{}

		BTreeNode(const BTreeNode& other) noexcept
		: position(other.position)
		{}

	public:
		BTreeNode& operator=(const BTreeNode& other) noexcept
		{
			position = other.position;
			return *this;
		}

		BTreeNode& operator=(sl_null_t) noexcept
		{
			position = 0;
			return *this;
		}

		sl_bool operator==(const BTreeNode& other) const noexcept
		{
			return position == other.position;
		}

		sl_bool operator!=(const BTreeNode& other) const noexcept
		{
			return position != other.position;
		}

		sl_bool isNull() const noexcept
		{
			return position == 0;
		}

		sl_bool isNotNull() const noexcept
		{
			return position != 0;
		}

		void setNull() noexcept
		{
			position = 0;
		}

		explicit operator sl_bool() const noexcept
		{
			return position != 0;
		}

	};

	class SLIB_EXPORT BTreePosition
	{
	public:
		BTreeNode node;
		sl_uint32 item;

	public:
		BTreePosition() noexcept
		: item(0)
		{}

		BTreePosition(sl_null_t) noexcept
		: item(0)
		{}

		BTreePosition(const BTreeNode& _node, sl_uint32 _item) noexcept
		: node(_node), item(_item)
		{}

		BTreePosition(const BTreePosition& other) noexcept
		: node(other.node), item(other.item)
		{}

	public:
		BTreePosition& operator=(const BTreePosition& other) noexcept
		{
			node = other.node;
			item = other.item;
			return *this;
		}

		BTreePosition& operator=(sl_null_t) noexcept
		{
			node.setNull();
			item = 0;
			return *this;
		}

		sl_bool operator==(const BTreePosition& other) const noexcept
		{
			return node == other.node && item == other.item;
		}

		sl_bool operator!=(const BTreePosition& other) const noexcept
		{
			return node != other.node || item != other.item;
		}

		sl_bool isNull() const noexcept
		{
			return node.isNull();
		}

		sl_bool isNotNull() const noexcept
		{
			return node.isNotNull();
		}

		void setNull() noexcept
		{
			node.setNull();
		}

		explicit operator sl_bool() const noexcept
		{
			return node.isNotNull();
		}

	};

	template < class KT, class VT, class KEY_COMPARE = Compare<KT> >
	class SLIB_EXPORT BTree
	{
	public:
		BTree(sl_uint32 order = SLIB_BTREE_DEFAULT_ORDER)
		{
			if (order < 1) {
				order = 1;
			}
			m_order = order;
			m_maxLength = 0;
			m_totalCount = 0;
			initialize();
		}

		BTree(const KEY_COMPARE& compare, sl_uint32 order = SLIB_BTREE_DEFAULT_ORDER): m_compare(compare)
		{
			if (order < 1) {
				order = 1;
			}
			m_order = order;
			m_maxLength = 0;
			m_totalCount = 0;
			initialize();
		}

		virtual ~BTree()
		{
			free();
		}

	public:
		sl_bool isValid() const noexcept
		{
			return m_rootNode != sl_null;
		}

		sl_uint32 getOrder() const noexcept
		{
			return m_order;
		}

		sl_uint32 getMaxLength() const noexcept
		{
			return m_maxLength;
		}

		sl_uint64 getCountInNode(const BTreeNode& node) const
		{
			if (node.isNull()) {
				return 0;
			}
			NodeDataScope data(this, node);
			if (data.isNotNull()) {
				sl_uint64 ret = data->countTotal;
				return ret;
			} else {
				return 0;
			}
		}

		sl_uint64 getCount() const
		{
			return getCountInNode(getRootNode());
		}

		sl_bool isEmpty() const
		{
			return getCountInNode(getRootNode()) == 0;
		}

		sl_bool isNotEmpty() const
		{
			return getCountInNode(getRootNode()) > 0;
		}

		sl_bool getAt(const BTreePosition& pos, KT* key = sl_null, VT* value = sl_null) const
		{
			NodeDataScope data(this, pos.node);
			if (data.isNotNull()) {
				if (pos.item < data->countItems) {
					if (key) {
						*key = data->keys[pos.item];
					}
					if (value) {
						*value = data->values[pos.item];
					}
					return sl_true;
				}
			}
			return sl_false;
		}

		sl_bool moveToFirstInNode(const BTreeNode& _node, BTreePosition& pos, KT* key = sl_null, VT* value = sl_null) const
		{
			BTreeNode node(_node);
			while (1) {
				NodeDataScope data(this, node);
				if (data.isNull()) {
					return sl_false;
				}
				BTreeNode first = data->linkFirst;
				if (first.isNotNull()) {
					node = first;
				} else {
					if (data->countItems == 0) {
						return sl_false;
					} else {
						if (key) {
							*key = data->keys[0];
						}
						if (value) {
							*value = data->values[0];
						}
						pos.node = node;
						pos.item = 0;
						return sl_true;
					}
				}
			}
			return sl_false;
		}

		sl_bool moveToFirst(BTreePosition& pos, KT* key = sl_null, VT* value = sl_null) const
		{
			BTreeNode root = getRootNode();
			if (root.isNull()) {
				return sl_false;
			}
			return moveToFirstInNode(root, pos, key, value);
		}

		sl_bool moveToLastInNode(const BTreeNode& _node, BTreePosition& pos, KT* key = sl_null, VT* value = sl_null) const
		{
			BTreeNode node(_node);
			while (1) {
				NodeDataScope data(this, node);
				if (data.isNull()) {
					return sl_false;
				}
				sl_uint32 n = data->countItems;
				if (n == 0) {
					BTreeNode first = data->linkFirst;
					if (first.isNotNull()) {
						node = first;
					} else {
						return sl_false;
					}
				} else {
					BTreeNode last = data->links[n - 1];
					if (last.isNotNull()) {
						node = last;
					} else {
						if (key) {
							*key = data->keys[n - 1];
						}
						if (value) {
							*value = data->values[n - 1];
						}
						pos.node = node;
						pos.item = n - 1;
						return sl_true;
					}
				}
			}
			return sl_false;
		}

		sl_bool moveToLast(BTreePosition& pos, KT* key = sl_null, VT* value = sl_null) const
		{
			BTreeNode root = getRootNode();
			if (root.isNull()) {
				return sl_false;
			}
			return moveToLastInNode(root, pos, key, value);
		}

		sl_bool moveToPrevious(BTreePosition& pos, KT* key = sl_null, VT* value = sl_null) const
		{
			if (pos.isNull()) {
				return moveToLast(pos, key, value);
			}
			BTreeNode nodeStart = pos.node;
			sl_uint32 itemStart = pos.item;
			NodeDataScope dataStart(this, nodeStart);
			if (dataStart.isNull()) {
				return sl_false;
			}
			if (itemStart >= dataStart->countItems) {
				return sl_false;
			}
			BTreeNode node;
			if (itemStart == 0) {
				node = dataStart->linkFirst;
			} else {
				node = dataStart->links[itemStart - 1];
			}
			if (node.isNotNull()) {
				return moveToLastInNode(node, pos, key, value);
			} else {
				if (itemStart == 0) {
					node = nodeStart;
					BTreeNode parent = dataStart->linkParent;
					while (1) {
						if (parent.isNull()) {
							return sl_false;
						}
						NodeDataScope data(this, parent);
						if (data.isNull()) {
							return sl_false;
						}
						if (data->linkFirst == node) {
							node = parent;
							parent = data->linkParent;
						} else {
							sl_uint32 n = data->countItems;
							sl_uint32 i = 0;
							for (; i < n; i++) {
								if (data->links[i] == node) {
									pos.node = parent;
									pos.item = i;
									if (key) {
										*key = data->keys[i];
									}
									if (value) {
										*value = data->values[i];
									}
									return sl_true;
								}
							}
							return sl_false;
						}
					}
				} else {
					itemStart--;
					pos.item = itemStart;
					if (key) {
						*key = dataStart->keys[itemStart];
					}
					if (value) {
						*value = dataStart->values[itemStart];
					}
					return sl_true;
				}
			}
			return sl_false;
		}

		sl_bool moveToNext(BTreePosition& pos, KT* key = sl_null, VT* value = sl_null) const
		{
			if (pos.isNull()) {
				return moveToFirst(pos, key, value);
			}
			BTreeNode nodeStart = pos.node;
			sl_uint32 itemStart = pos.item;
			NodeDataScope dataStart(this, nodeStart);
			if (dataStart.isNull()) {
				return sl_false;
			}
			if (itemStart >= dataStart->countItems) {
				return sl_false;
			}
			BTreeNode node = dataStart->links[itemStart];
			if (node.isNotNull()) {
				return moveToFirst(pos, key, value);
			} else {
				if (itemStart == dataStart->countItems - 1) {
					node = nodeStart;
					BTreeNode parent = dataStart->linkParent;
					while (1) {
						if (parent.isNull()) {
							return sl_false;
						}
						NodeDataScope data(this, parent);
						if (data.isNull()) {
							return sl_false;
						}
						if (data->linkFirst == node) {
							pos.node = parent;
							pos.item = 0;
							if (key) {
								*key = data->keys[0];
							}
							if (value) {
								*value = data->values[0];
							}
							return sl_true;
						}
						sl_uint32 n = data->countItems;
						for (sl_uint32 i = 1; i < n; i++) {
							if (data->links[i - 1] == node) {
								pos.node = parent;
								pos.item = i;
								if (key) {
									*key = data->keys[i];
								}
								if (value) {
									*value = data->values[i];
								}
								return sl_true;
							}
						}
						if (data->links[n - 1] == node) {
							node = parent;
							parent = data->linkParent;
						} else {
							return sl_false;
						}
					}
				} else {
					itemStart++;
					pos.item = itemStart;
					if (key) {
						*key = dataStart->keys[itemStart];
					}
					if (value) {
						*value = dataStart->values[itemStart];
					}
					return sl_true;
				}
			}
			return sl_false;
		}

		sl_bool findItemInNode(const BTreeNode& node, const KT& key, sl_uint32& pos, BTreeNode& link, VT* outValue = sl_null, sl_uint32* pCountItemsInNode = sl_null) const
		{
			pos = 0;
			NodeDataScope data(this, node);
			if (data.isNull()) {
				link.setNull();
				if (pCountItemsInNode) {
					*pCountItemsInNode = 0;
				}
				return sl_false;
			}
			sl_uint32 n = data->countItems;
			if (pCountItemsInNode) {
				*pCountItemsInNode = n;
			}
			if (n == 0) {
				link.setNull();
				return sl_false;
			}
			sl_size _pos = 0;
			if (BinarySearch::search(data->keys, n, key, &_pos, m_compare)) {
				pos = (sl_uint32)_pos;
				if (outValue) {
					*outValue = data->values[pos];
				}
				link = data->links[pos];
				return sl_true;
			} else {
				pos = (sl_uint32)_pos;
				if (pos) {
					link = data->links[pos - 1];
				} else {
					link = data->linkFirst;
				}
				return sl_false;
			}
		}

		sl_bool findInNode(const BTreeNode& node, const KT& key, BTreePosition* pos = sl_null, VT* outValue = sl_null) const
		{
			BTreeNode link;
			sl_uint32 item;
			if (findItemInNode(node, key, item, link, outValue)) {
				if (pos) {
					pos->node = node;
					pos->item = item;
				}
				return sl_true;
			} else {
				if (link.isNotNull()) {
					return findInNode(link, key, pos, outValue);
				} else {
					if (pos) {
						pos->node = node;
						pos->item = item;
					}
				}
			}
			return sl_false;
		}

		sl_bool find(const KT& key, BTreePosition* pos = sl_null, VT* outValue = sl_null) const
		{
			BTreeNode root = getRootNode();
			if (root.isNull()) {
				return sl_false;
			}
			return findInNode(root, key, pos, outValue);
		}

		BTreePosition findInsertPositionInNode(const BTreeNode& node, const KT& key) const
		{
			BTreeNode link;
			sl_uint32 item;
			findItemInNode(node, key, item, link, sl_null);
			if (link.isNotNull()) {
				return findInsertPositionInNode(link, key);
			} else {
				return BTreePosition(node, item);
			}
		}

		sl_bool getNearestInNode(const BTreeNode& node, const KT& key, BTreePosition* pLessEqual = sl_null, BTreePosition* pGreaterEqual = sl_null) const
		{
			BTreeNode link;
			sl_uint32 item;
			sl_uint32 n;
			if (findItemInNode(node, key, item, link, sl_null, &n)) {
				if (pLessEqual) {
					pLessEqual->node = node;
					pLessEqual->item = item;
				}
				if (pGreaterEqual) {
					pGreaterEqual->node = node;
					pGreaterEqual->item = item;
				}
				return sl_true;
			} else {
				if (link.isNotNull()) {
					return getNearestInNode(link, key, pLessEqual, pGreaterEqual);
				} else {
					if (n == 0) {
						if (pLessEqual) {
							pLessEqual->setNull();
						}
						if (pGreaterEqual) {
							pGreaterEqual->setNull();
						}
					} else {
						if (pLessEqual) {
							pLessEqual->node = node;
							pLessEqual->item = item;
							if (!(moveToPrevious(*pLessEqual))) {
								pLessEqual->setNull();
							}
						}
						if (pGreaterEqual) {
							pGreaterEqual->node = node;
							if (item == n) {
								pGreaterEqual->item = item - 1;
								if (!(moveToNext(*pGreaterEqual))) {
									pGreaterEqual->setNull();
								}
							} else {
								pGreaterEqual->item = item;
							}
						}
					}
					return sl_false;
				}
			}
		}

		sl_bool getNearest(const KT& key, BTreePosition* pLessEqual = sl_null, BTreePosition* pGreaterEqual = sl_null) const
		{
			BTreeNode root = getRootNode();
			if (root.isNull()) {
				if (pLessEqual) {
					pLessEqual->setNull();
				}
				if (pGreaterEqual) {
					pGreaterEqual->setNull();
				}
				return sl_false;
			}
			return getNearestInNode(root, key, pLessEqual, pGreaterEqual);
		}

		sl_bool getEqualRangeInNode(const BTreeNode& node, const KT& key, BTreePosition* pLowerBound = sl_null, BTreePosition* pUpperBound = sl_null) const
		{
			NodeDataScope data(this, node);
			sl_uint32 n;
			if (data.isNull() || !((n = data->countItems))) {
				if (pLowerBound) {
					pLowerBound->setNull();
				}
				if (pUpperBound) {
					pUpperBound->setNull();
				}
				return sl_false;
			}
			sl_size _item = 0;
			if (!(BinarySearch::search(data->keys, n, key, &_item, m_compare))) {
				sl_uint32 item = (sl_uint32)_item;
				BTreeNode link;
				if (item) {
					link = data->links[item - 1];
				} else {
					link = data->linkFirst;
				}
				if (link.isNotNull()) {
					return getEqualRangeInNode(link, key, pLowerBound, pUpperBound);
				} else {
					BTreePosition pos;
					if (item < n) {
						pos.node = node;
						pos.item = item;
					}
					if (pLowerBound) {
						*pLowerBound = pos;
					}
					if (pUpperBound) {
						*pUpperBound = pos;
					}
					return sl_false;
				}
			}
			sl_uint32 itemMiddle = (sl_uint32)_item;
			if (pLowerBound) {
				sl_uint32 item = itemMiddle;
				for (sl_int32 i = itemMiddle - 1; i >= 0; i--) {
					if (!(m_compare(data->keys[i], key))) {
						item = i;
					} else {
						break;
					}
				}
				BTreeNode link;
				if (item) {
					link = data->links[item - 1];
				} else {
					link = data->linkFirst;
				}
				if (link.isNull() || !(getEqualRangeInNode(link, key, pLowerBound, sl_null))) {
					pLowerBound->node = node;
					pLowerBound->item = item;
				}
			}
			if (pUpperBound) {
				sl_uint32 item = itemMiddle;
				for (sl_uint32 i = itemMiddle + 1; i < n; i++) {
					if (!(m_compare(data->keys[i], key))) {
						item = i;
					} else {
						break;
					}
				}
				BTreeNode link = data->links[item];
				if (link.isNull() || !(getEqualRangeInNode(link, key, sl_null, pUpperBound))) {
					if (item < n) {
						pUpperBound->node = node;
						pUpperBound->item = item + 1;
					} else {
						pUpperBound->setNull();
					}
				}
			}
			return sl_true;
		}

		sl_bool getEqualRange(const KT& key, BTreePosition* pLowerBound = sl_null, BTreePosition* pUpperBound = sl_null) const
		{
			BTreeNode root = getRootNode();
			if (root.isNull()) {
				if (pLowerBound) {
					pLowerBound->setNull();
				}
				if (pUpperBound) {
					pUpperBound->setNull();
				}
				return sl_false;
			}
			return getEqualRangeInNode(root, key, pLowerBound, pUpperBound);
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		BTreePosition findKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const
		{
			BTreePosition pos, end;
			if (getEqualRange(key, &pos, &end)) {
				VT v;
				if (getAt(pos, sl_null, &v)) {
					do {
						if (value_equals(value, v)) {
							return pos;
						}
						if (!(moveToNext(pos, sl_null, &v))) {
							break;
						}
					} while (pos != end);
				}
			}
			return sl_null;
		}

		sl_bool get(const KT& key, VT* value = sl_null) const
		{
			return find(key, sl_null, value);
		}

		List<VT> getValues(const KT& key) const
		{
			BTreePosition pos, end;
			if (getEqualRange(key, &pos, &end)) {
				VT v;
				if (getAt(pos, sl_null, &v)) {
					List<VT> ret;
					do {
						ret.add_NoLock(v);
						if (!(moveToNext(pos, sl_null, &v))) {
							break;
						}
					} while (pos != end);
					return ret;
				}
			}
			return sl_null;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		List<VT> getValuesByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS()) const
		{
			List<VT> ret;
			BTreePosition pos, end;
			if (getEqualRange(key, &pos, &end)) {
				VT v;
				if (getAt(pos, sl_null, &v)) {
					do {
						if (value_equals(v, value)) {
							ret.add_NoLock(v);
						}
						if (!(moveToNext(pos, sl_null, &v))) {
							break;
						}
					} while (pos != end);
				}
			}
			return ret;
		}

		sl_bool put(const KT& key, const VT& value, BTreePosition* pPos = sl_null, sl_bool* isInsertion = sl_null) noexcept
		{
			BTreeNode root = getRootNode();
			if (root.isNull()) {
				if (isInsertion) {
					*isInsertion = sl_true;
				}
				return sl_false;
			}
			BTreePosition pos;
			if (findInNode(root, key, &pos)) {
				if (isInsertion) {
					*isInsertion = sl_false;
				}
				if (pPos) {
					*pPos = pos;
				}
				NodeDataScope data(this, pos.node);
				if (data.isNotNull()) {
					if (pos.item < data->countItems) {
						data->values[pos.item] = value;
						return writeNodeData(pos.node, data.data);
					}
				}
				return sl_false;
			}
			BTreeNode link;
			if (_insertItemInNode(pos.node, pos.item, link, key, value, link, pPos)) {
				if (isInsertion) {
					*isInsertion = sl_true;
				}
				return sl_true;
			}
			if (isInsertion) {
				*isInsertion = sl_false;
			}
			return sl_false;
		}

		sl_bool replace(const KT& key, const VT& value, BTreePosition* pPos = sl_null) noexcept
		{
			BTreeNode root = getRootNode();
			if (root.isNull()) {
				return sl_false;
			}
			BTreePosition pos;
			if (findInNode(root, key, &pos)) {
				if (pPos) {
					*pPos = pos;
				}
				NodeDataScope data(this, pos.node);
				if (data.isNotNull()) {
					if (pos.item < data->countItems) {
						data->values[pos.item] = value;
						return writeNodeData(pos.node, data.data);
					}
				}
			}
			return sl_false;
		}

		sl_bool add(const KT& key, const VT& value, BTreePosition* pPos = sl_null) noexcept
		{
			BTreeNode root = getRootNode();
			if (root.isNull()) {
				return sl_false;
			}
			BTreePosition pos = findInsertPositionInNode(root, key);
			BTreeNode link;
			if (_insertItemInNode(pos.node, pos.item, link, key, value, link, pPos)) {
				return sl_true;
			}
			if (pPos) {
				pPos->setNull();
			}
			return sl_false;
		}

		sl_bool emplace(const KT& key, const VT& value, BTreePosition* pPos = sl_null) noexcept
		{
			BTreeNode root = getRootNode();
			if (root.isNull()) {
				return sl_false;
			}
			BTreePosition pos;
			if (findInNode(root, key, &pos)) {
				return sl_false;
			}
			BTreeNode link;
			if (_insertItemInNode(pos.node, pos.item, link, key, value, link, pPos)) {
				return sl_true;
			}
			return sl_false;
		}

		sl_bool removeNode(const BTreeNode& node)
		{
			if (node.isNull()) {
				return sl_false;
			}
			if (node == getRootNode()) {
				return sl_false;
			}
			return _removeNode(node, sl_true);
		}

		sl_bool removeAt(const BTreePosition& pos)
		{
			if (pos.node.isNull()) {
				return sl_false;
			}
			NodeDataScope data(this, pos.node);
			if (data.isNull()) {
				return sl_false;
			}
			sl_uint32 n = data->countItems;
			if (n == 0) {
				return sl_false;
			}
			if (pos.item >= n) {
				return sl_false;
			}
			BTreeNode left;
			if (pos.item == 0) {
				left = data->linkFirst;
			} else {
				left = data->links[pos.item - 1];
			}
			BTreeNode right = data->links[pos.item];
			if (left.isNull()) {
				if (pos.item == 0) {
					data->linkFirst = data->links[0];
				} else {
					data->links[pos.item - 1] = data->links[pos.item];
				}
			} else {
				if (right.isNotNull()) {
					BTreePosition nextPos = pos;
					KT keyNext;
					VT valueNext;
					if (!(moveToNext(nextPos, &keyNext, &valueNext))) {
						return sl_false;
					}
					if (nextPos.node.isNull()) {
						return sl_false;
					}
					if (nextPos.node == pos.node) {
						return sl_false;
					}
					data->keys[pos.item] = keyNext;
					data->values[pos.item] = valueNext;
					if (!writeNodeData(pos.node, data.data)) {
						return sl_false;
					}
					return removeAt(nextPos);
				}
			}
			if (n <= 1 && pos.node != getRootNode()) {
				return _removeNode(pos.node, sl_true);
			}
			for (sl_uint32 i = pos.item; i < n - 1; i++) {
				data->keys[i] = data->keys[i + 1];
				data->values[i] = data->values[i + 1];
				data->links[i] = data->links[i + 1];
			}
			data->countTotal--;
			data->countItems = n - 1;
			if (!writeNodeData(pos.node, data.data)) {
				return sl_false;
			}
			_changeParentTotalCount(data.data, -1);
			return sl_true;
		}

		sl_bool remove(const KT& key, VT* outValue = sl_null)
		{
			BTreePosition pos;
			if (find(key, &pos, outValue)) {
				if (removeAt(pos)) {
					return sl_true;
				}
			}
			return sl_false;
		}

		sl_size removeItems(const KT& key)
		{
			BTreePosition pos;
			if (find(key, &pos, sl_null)) {
				if (removeAt(pos)) {
					sl_size n = 1;
					while (find(key, &pos, sl_null)) {
						if (removeAt(pos)) {
							n++;
						} else {
							break;
						}
					}
					return n;
				}
			}
			return 0;
		}

		List<VT> removeItemsAndReturnValues(const KT& key)
		{
			List<VT> ret;
			BTreePosition pos;
			VT v;
			if (find(key, &pos, &v)) {
				if (removeAt(pos)) {
					ret.add_NoLock(v);
					while (find(key, &pos, &v)) {
						if (removeAt(pos)) {
							ret.add_NoLock(v);
						} else {
							break;
						}
					}
				}
			}
			return ret;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_bool removeKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS())
		{
			BTreePosition pos = findKeyAndValue(key, value, value_equals);
			if (pos.isNotNull()) {
				if (removeAt(pos)) {
					return sl_true;
				}
			}
			return sl_false;
		}

		template < class VALUE, class VALUE_EQUALS = Equals<VT, VALUE> >
		sl_size removeItemsByKeyAndValue(const KT& key, const VALUE& value, const VALUE_EQUALS& value_equals = VALUE_EQUALS())
		{
			BTreePosition pos = findKeyAndValue(key, value, value_equals);
			if (pos.isNotNull()) {
				if (removeAt(pos)) {
					sl_size n = 1;
					for (;;) {
						pos = findKeyAndValue(key, value, value_equals);
						if (pos.isNotNull()) {
							if (removeAt(pos)) {
								n++;
							} else {
								break;
							}
						} else {
							break;
						}
					}
					return n;
				}
			}
			return 0;
		}

		sl_size removeAll()
		{
			BTreeNode node = getRootNode();
			NodeDataScope data(this, node);
			sl_size countTotal = (sl_size)(data->countTotal);
			if (data.isNotNull()) {
				_removeNode(data->linkFirst, sl_false);
				sl_uint32 n = data->countItems;
				for (sl_uint32 i = 0; i < n; i++) {
					if (data->links[i].isNotNull()) {
						_removeNode(data->links[i], sl_false);
					}
				}
				data->countTotal = 0;
				data->countItems = 0;
				data->linkFirst.setNull();
				m_totalCount = 0;
				if (writeNodeData(node, data.data)) {
					return countTotal;
				}
			}
			return 0;
		}

	protected:
		struct NodeData
		{
			sl_uint64 countTotal;
			sl_uint32 countItems;

			BTreeNode linkParent;
			BTreeNode linkFirst;
			KT* keys;
			VT* values;
			BTreeNode* links;
		};

		class NodeDataScope
		{
		public:
			NodeData* data;
			BTree* tree;

		public:
			NodeDataScope(const BTree* tree, const BTreeNode& node)
			{
				this->tree = (BTree*)tree;
				this->data = tree->readNodeData(node);
			}

			~NodeDataScope()
			{
				tree->releaseNodeData(data);
			}

		public:
			NodeData* operator->()
			{
				return data;
			}

			sl_bool isNull()
			{
				return data == sl_null;
			}

			sl_bool isNotNull()
			{
				return data != sl_null;
			}

		};

		friend class NodeDataScope;

	private:
		sl_uint32 m_order;
		sl_uint32 m_maxLength;
		sl_uint64 m_totalCount;
		KEY_COMPARE m_compare;

	private:
		NodeData* _createNodeData()
		{
			NodeData* data = new NodeData;
			if (data) {
				data->countTotal = 0;
				data->countItems = 0;
				data->linkParent.setNull();
				data->linkFirst.setNull();
				data->keys = NewHelper<KT>::create(m_order);
				if (data->keys) {
					data->values = NewHelper<VT>::create(m_order);
					if (data->values) {
						data->links = NewHelper<BTreeNode>::create(m_order);
						if (data->links) {
							return data;
						}
						NewHelper<VT>::free(data->values, m_order);
					}
					NewHelper<KT>::free(data->keys, m_order);
				}
				delete data;
			}
			return sl_null;
		}

		void _freeNodeData(NodeData* data)
		{
			if (data) {
				NewHelper<KT>::free(data->keys, m_order);
				NewHelper<VT>::free(data->values, m_order);
				NewHelper<BTreeNode>::free(data->links, m_order);
				delete data;
			}
		}

		sl_bool _insertItemInNode(const BTreeNode& node, sl_uint32 at, const BTreeNode& after, const KT& key, const VT& value, const BTreeNode& link, BTreePosition* pPosition)
		{
			NodeDataScope data(this, node);
			if (data.isNull()) {
				return sl_false;
			}
			sl_uint32 n = data->countItems;
			if (n > m_order) {
				return sl_false;
			}
			if (after.isNotNull()) {
				if (after == data->linkFirst) {
					at = 0;
				} else {
					for (sl_uint32 i = 0; i < n; i++) {
						if (data->links[i] == after) {
							at = i + 1;
						}
					}
				}
			}
			if (at > n) {
				return sl_false;
			}
			if (n < m_order) {
				for (sl_uint32 i = n; i > at; i--) {
					data->keys[i] = Move(data->keys[i - 1]);
					data->values[i] = Move(data->values[i - 1]);
					data->links[i] = Move(data->links[i - 1]);
				}
				data->keys[at] = key;
				data->values[at] = value;
				data->links[at] = link;
				data->countItems = n + 1;
				data->countTotal += 1;
				if (writeNodeData(node, data.data)) {
					_changeParentTotalCount(data.data, 1);
					if (pPosition) {
						pPosition->node = node;
						pPosition->item = at;
					}
					return sl_true;
				}
			} else {
				sl_uint32 half = n / 2; // equals to  "m_order / 2"
				NodeData* newData = _createNodeData();
				if (newData) {
					KT keyTop;
					VT valueTop;
					sl_bool flagInsertAtTop = sl_false;
					sl_bool flagInsertAtNewNode = sl_false;
					sl_uint32 i;
					if (at > half) {
						sl_uint32 m = at - half - 1;
						newData->linkFirst = data->links[half];
						for (i = 0; i < m; i++) {
							newData->keys[i] = Move(data->keys[i + half + 1]);
							newData->values[i] = Move(data->values[i + half + 1]);
							newData->links[i] = Move(data->links[i + half + 1]);
						}
						newData->keys[m] = key;
						newData->values[m] = value;
						newData->links[m] = link;
						if (pPosition) {
							pPosition->item = m;
						}
						for (i = m + 1; i < n - half; i++) {
							newData->keys[i] = Move(data->keys[i + half]);
							newData->values[i] = Move(data->values[i + half]);
							newData->links[i] = Move(data->links[i + half]);
						}
						keyTop = data->keys[half];
						valueTop = data->values[half];
						flagInsertAtNewNode = sl_true;
					} else if (at < half) {
						newData->linkFirst = data->links[half - 1];
						for (i = 0; i < n - half; i++) {
							newData->keys[i] = Move(data->keys[i + half]);
							newData->values[i] = Move(data->values[i + half]);
							newData->links[i] = Move(data->links[i + half]);
						}
						keyTop = data->keys[half - 1];
						valueTop = data->values[half - 1];
						for (i = half - 1; i > at; i--) {
							data->keys[i] = Move(data->keys[i - 1]);
							data->values[i] = Move(data->values[i - 1]);
							data->links[i] = Move(data->links[i - 1]);
						}
						data->keys[at] = key;
						data->values[at] = value;
						data->links[at] = link;
						if (pPosition) {
							pPosition->node = node;
							pPosition->item = at;
						}
					} else {
						newData->linkFirst = link;
						for (i = 0; i < n - half; i++) {
							newData->keys[i] = Move(data->keys[i + half]);
							newData->values[i] = Move(data->values[i + half]);
							newData->links[i] = Move(data->links[i + half]);
						}
						keyTop = key;
						valueTop = value;
						flagInsertAtTop = sl_true;
					}
					BTreeNode parent = data->linkParent;
					sl_bool flagCreateRoot = sl_false;
					if (parent.isNull()) {
						parent = createNode(sl_null);
						if (parent.isNull()) {
							_freeNodeData(newData);
							return sl_false;
						}
						flagCreateRoot = sl_true;
						if (! setRootNode(parent)) {
							_freeNodeData(newData);
							return sl_false;
						}
						m_maxLength++;
					}
					data->linkParent = parent;
					data->countItems = half;
					data->countTotal = _getTotalCountInData(data.data);
					newData->linkParent = parent;
					newData->countItems = n - half;
					newData->countTotal = _getTotalCountInData(newData);
					BTreeNode newNode = createNode(newData);
					if (newNode.isNull()) {
						_freeNodeData(newData);
						return sl_false;
					}
					if (flagInsertAtNewNode) {
						if (pPosition) {
							pPosition->node = newNode;
						}
					}
					NodeDataScope newNodeData(this, newNode);
					sl_bool flagSuccess = sl_false;
					if (writeNodeData(node, data.data)) {
						if (newNodeData->linkFirst.isNotNull()) {
							NodeDataScope dataChild(this, newNodeData->linkFirst);
							if (dataChild.isNotNull()) {
								dataChild->linkParent = newNode;
								writeNodeData(newNodeData->linkFirst, dataChild.data);
							}
						}
						for (i = 0; i < n - half; i++) {
							BTreeNode child = newNodeData->links[i];
							if (child.isNotNull()) {
								NodeDataScope dataChild(this, child);
								if (dataChild.isNotNull()) {
									dataChild->linkParent = newNode;
									writeNodeData(child, dataChild.data);
								}
							}
						}
						if (flagCreateRoot) {
							NodeDataScope dataRoot(this, parent);
							if (dataRoot.isNotNull()) {
								dataRoot->countTotal = data->countTotal + newNodeData->countTotal + 1;
								m_totalCount = dataRoot->countTotal;
								dataRoot->countItems = 1;
								dataRoot->linkFirst = node;
								dataRoot->keys[0] = Move(keyTop);
								dataRoot->values[0] = Move(valueTop);
								dataRoot->links[0] = Move(newNode);
								flagSuccess = writeNodeData(parent, dataRoot.data);
							}
							if (flagInsertAtTop) {
								if (pPosition) {
									pPosition->node = parent;
									pPosition->item = 0;
								}
							}
						} else {
							if (flagInsertAtTop) {
								flagSuccess = _insertItemInNode(parent, -1, node, keyTop, valueTop, newNode, pPosition);
							} else {
								flagSuccess = _insertItemInNode(parent, -1, node, keyTop, valueTop, newNode, sl_null);
							}
						}
					}
					return flagSuccess;
				}
			}
			return sl_false;
		}

		void _changeTotalCount(const BTreeNode& node, sl_int64 n)
		{
			NodeDataScope data(this, node);
			if (data.isNotNull()) {
				data->countTotal += n;
				writeNodeData(node, data.data);
				BTreeNode parent = data->linkParent;
				if (parent.isNotNull()) {
					_changeTotalCount(parent, n);
				}
			}
		}

		void _changeParentTotalCount(NodeData* data, sl_int64 n)
		{
			BTreeNode parent = data->linkParent;
			if (parent.isNotNull()) {
				_changeTotalCount(parent, n);
			} else {
				m_totalCount = data->countTotal;
			}
		}

		sl_uint64 _getTotalCountInData(NodeData* data) const
		{
			sl_uint32 n = data->countItems;
			sl_uint64 m = n + getCountInNode(data->linkFirst);
			for (sl_uint32 i = 0; i < n; i++) {
				m += getCountInNode(data->links[i]);
			}
			return m;
		}

		sl_bool _removeNode(const BTreeNode& node, sl_bool flagUpdateParent)
		{
			if (node.isNull()) {
				return sl_false;
			}
			NodeDataScope data(this, node);
			if (data.isNull()) {
				return sl_false;
			}
			if (flagUpdateParent) {
				BTreeNode parent = data->linkParent;
				if (parent.isNull()) {
					return sl_false;
				}
				NodeDataScope parentData(this, parent);
				if (parentData.isNotNull()) {
					if (node == parentData->linkFirst) {
						parentData->linkFirst.setNull();
					} else {
						sl_uint32 i;
						sl_uint32 n = parentData->countItems;
						for (i = 0; i < n; i++) {
							if (parentData->links[i] == node) {
								parentData->links[i].setNull();
								break;
							}
						}
						if (i == n) {
							return sl_false;
						}
					}
					parentData->countTotal -= data->countTotal;
					if (!writeNodeData(parent, parentData.data)) {
						return sl_false;
					}
					_changeParentTotalCount(parentData.data, -((sl_int64)(data->countTotal)));
				}
			}
			{
				_removeNode(data->linkFirst, sl_false);
				sl_uint32 n = data->countItems;
				for (sl_uint32 i = 0; i < n; i++) {
					if (data->links[i].isNotNull()) {
						_removeNode(data->links[i], sl_false);
					}
				}
			}
			return deleteNode(node);
		}

		// container-specific implementation
	private:
		NodeData* m_rootNode;

	protected:
		virtual void initialize()
		{
			m_rootNode = _createNodeData();
		}

		virtual void free()
		{
			_removeNode(getRootNode(), sl_false);
		}

		virtual BTreeNode getRootNode() const
		{
			return (sl_size)(m_rootNode);
		}

		virtual sl_bool setRootNode(BTreeNode node)
		{
			if (node.isNull()) {
				return sl_false;
			}
			m_rootNode = (NodeData*)(void*)(sl_size)(node.position);
			return sl_true;
		}

		// here, "data" must be newly created node data. this function should manage the memory of "data" parameter
		virtual BTreeNode createNode(NodeData* data)
		{
			sl_size position;
			if (data) {
				position = (sl_size)data;
			} else {
				position = (sl_size)(_createNodeData());
			}
			// important for file storage
			{
				// _freeNodeData(data);
			}
			return position;
		}

		virtual sl_bool deleteNode(BTreeNode node)
		{
			if (node.isNull()) {
				return sl_false;
			}
			NodeData* data = (NodeData*)(void*)(sl_size)(node.position);
			_freeNodeData(data);
			return sl_true;
		}

		virtual NodeData* readNodeData(const BTreeNode& node) const
		{
			NodeData* data = (NodeData*)(void*)(sl_size)(node.position);
			return data;
		}

		virtual sl_bool writeNodeData(const BTreeNode& node, NodeData* data)
		{
			if (node.isNull()) {
				return sl_false;
			}
			if (!data) {
				return sl_false;
			}
			NodeData* o = (NodeData*)(void*)(sl_size)(node.position);
			if (o != data) {
				sl_uint32 n = o->countItems = data->countItems;
				o->countTotal = data->countTotal;
				o->linkParent = data->linkParent;
				o->linkFirst = data->linkFirst;
				for (sl_uint32 i = 0; i < n; i++) {
					o->keys[i] = data->keys[i];
					o->values[i] = data->values[i];
					o->links[i] = data->links[i];
				}
			}
			return sl_true;
		}

		virtual void releaseNodeData(NodeData* data)
		{
		}

	};

}

#endif
