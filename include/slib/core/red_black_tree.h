/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_RED_BLACK_TREE
#define CHECKHEADER_SLIB_CORE_RED_BLACK_TREE

#include "list.h"
#include "assert.h"

#include "priv/map_common.h"

#define PRIV_SLIB_RED_BLACK_TREE_MAX_DISTANCE 128

namespace slib
{

	struct SLIB_EXPORT RedBlackTreeNode
	{
		RedBlackTreeNode* parent;
		RedBlackTreeNode* left;
		RedBlackTreeNode* right;
		sl_bool flagRed;
	};

	namespace priv
	{
		namespace rb_tree
		{
			class SLIB_EXPORT Helper
			{
			public:
				static RedBlackTreeNode* getPrevious(RedBlackTreeNode* node) noexcept;

				static RedBlackTreeNode* getNext(RedBlackTreeNode* node) noexcept;

				static RedBlackTreeNode* getFirst(RedBlackTreeNode* node) noexcept;

				static RedBlackTreeNode* getLast(RedBlackTreeNode* node) noexcept;

				static void rebalanceAfterInsert(RedBlackTreeNode* node, RedBlackTreeNode** pRoot) noexcept;

				static void removeNode(RedBlackTreeNode* node, RedBlackTreeNode** pRoot) noexcept;

			};
		}
	}

	class SLIB_EXPORT RedBlackTree
	{
	public:
		template <class NODE>
		static NODE* getPreviousNode(NODE* node) noexcept
		{
			return reinterpret_cast<NODE*>(priv::rb_tree::Helper::getPrevious(reinterpret_cast<RedBlackTreeNode*>(node)));
		}

		template <class NODE>
		static NODE* getNextNode(NODE* node) noexcept
		{
			return reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
		}

		template <class NODE>
		static NODE* getFirstNode(NODE* root) noexcept
		{
			if (root) {
				return reinterpret_cast<NODE*>(priv::rb_tree::Helper::getFirst(reinterpret_cast<RedBlackTreeNode*>(root)));
			} else {
				return sl_null;
			}
		}

		template <class NODE>
		static NODE* getLastNode(NODE* root) noexcept
		{
			if (root) {
				return reinterpret_cast<NODE*>(priv::rb_tree::Helper::getLast(reinterpret_cast<RedBlackTreeNode*>(root)));
			} else {
				return sl_null;
			}
		}

		template <class NODE, class KEY, class KEY_COMPARE>
		static NODE* tryFind(NODE* look, KEY&& key, KEY_COMPARE&& key_compare, sl_compare_result& compare_result) noexcept
		{
			sl_compare_result comp;
			for (;;) {
				comp = key_compare(look->key, Forward<KEY>(key));
				if (!comp) {
					break;
				} else if (comp > 0) {
					NODE* left = look->left;
					if (left) {
						look = left;
					} else {
						break;
					}
				} else {
					NODE* right = look->right;
					if (right) {
						look = right;
					} else {
						break;
					}
				}
			}
			compare_result = comp;
			return look;
		}

		template <class NODE, class KEY, class KEY_COMPARE>
		static sl_bool getEqualRange(NODE* look, KEY&& key, KEY_COMPARE&& key_compare, NODE** pStart = sl_null, NODE** pEnd = sl_null) noexcept
		{
			if (!look) {
				return sl_false;
			}
			sl_compare_result compare_result;
			look = tryFind(look, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), compare_result);
			if (compare_result != 0) {
				return sl_false;
			}
			if (pStart) {
				NODE* last_equal = look;
				NODE* left = look->left;
				if (left) {
					NODE* node = left;
					for (;;) {
						if (!(key_compare(node->key, Forward<KEY>(key)))) {
							last_equal = node;
							left = node->left;
							if (left) {
								node = left;
							} else {
								break;
							}
						} else {
							NODE* right = node->right;
							if (right) {
								node = right;
							} else {
								break;
							}
						}
					}
				}
				*pStart = last_equal;
			}
			if (pEnd) {
				NODE* last_equal = look;
				NODE* right = look->right;
				if (right) {
					NODE* node = right;
					for (;;) {
						if (key_compare(node->key, Forward<KEY>(key)) != 0) {
							NODE* left = node->left;
							if (left) {
								node = left;
							} else {
								break;
							}
						} else {
							last_equal = node;
							right = node->right;
							if (right) {
								node = right;
							} else {
								break;
							}
						}
					}
				}
				*pEnd = last_equal;
			}
			return sl_true;
		}

		template <class NODE, class KEY, class KEY_COMPARE>
		static void getNearest(NODE* look, KEY&& key, KEY_COMPARE&& key_compare, NODE** pLessEqual = sl_null, NODE** pGreaterEqual = sl_null) noexcept
		{
			if (!look) {
				if (pLessEqual) {
					*pLessEqual = sl_null;
				}
				if (pGreaterEqual) {
					*pGreaterEqual = sl_null;
				}
				return;
			}
			sl_compare_result compare_result;
			NODE* node = tryFind(look, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), compare_result);
			if (!compare_result) {
				if (pLessEqual) {
					*pLessEqual = node;
				}
				if (pGreaterEqual) {
					*pGreaterEqual = node;
				}
			} else if (compare_result > 0) {
				if (pLessEqual) {
					*pLessEqual = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getPrevious(reinterpret_cast<RedBlackTreeNode*>(node)));
				}
				if (pGreaterEqual) {
					*pGreaterEqual = node;
				}
			} else {
				if (pLessEqual) {
					*pLessEqual = node;
				}
				if (pGreaterEqual) {
					*pGreaterEqual = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
				}
			}
		}

		template <class NODE, class KEY, class KEY_COMPARE>
		static NODE* getLowerBound(NODE* look, KEY&& key, KEY_COMPARE&& key_compare) noexcept
		{
			if (!look) {
				return sl_null;
			}
			NODE* last_greater_equal = sl_null;
			for (;;) {
				if (key_compare(look->key, Forward<KEY>(key)) >= 0) {
					last_greater_equal = look;
					NODE* left = look->left;
					if (left) {
						look = left;
					} else {
						break;
					}
				} else {
					NODE* right = look->right;
					if (right) {
						look = right;
					} else {
						break;
					}
				}
			}
			return last_greater_equal;
		}

		template <class NODE, class KEY, class KEY_COMPARE>
		static NODE* getUpperBound(NODE* look, KEY&& key, KEY_COMPARE&& key_compare) noexcept
		{
			if (!look) {
				return sl_null;
			}
			NODE* last_greater = sl_null;
			for (;;) {
				if (key_compare(look->key, Forward<KEY>(key)) > 0) {
					last_greater = look;
					NODE* left = look->left;
					if (left) {
						look = left;
					} else {
						break;
					}
				} else {
					NODE* right = look->right;
					if (right) {
						look = right;
					} else {
						break;
					}
				}
			}
			return last_greater;
		}

		template <class NODE, class KEY, class KEY_COMPARE>
		static NODE* find(NODE* look, KEY&& key, KEY_COMPARE&& key_compare) noexcept
		{
			if (!look) {
				return sl_null;
			}
			sl_compare_result compare_result;
			NODE* node = tryFind(look, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), compare_result);
			if (!compare_result) {
				return node;
			}
			return sl_null;
		}

		template <class NODE, class KEY, class KEY_COMPARE, class VALUE, class VALUE_EQUALS>
		static NODE* findKeyAndValue(NODE* look, KEY&& key, KEY_COMPARE&& key_compare, VALUE&& value, VALUE_EQUALS&& value_equals) noexcept
		{
			NODE *node, *end;
			if (getEqualRange(look, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), &node, &end)) {
				for (;;) {
					if (value_equals(node->value, Forward<VALUE>(value))) {
						return node;
					}
					if (node == end) {
						break;
					}
					node = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
				}
			}
			return sl_null;
		}

		template <class VT, class NODE, class KEY, class KEY_COMPARE>
		static void getValues(List<VT>& list, NODE* look, KEY&& key, KEY_COMPARE&& key_compare) noexcept
		{
			NODE *node, *end;
			if (getEqualRange(look, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), &node, &end)) {
				for (;;) {
					list.add_NoLock(node->value);
					if (node == end) {
						break;
					}
					node = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
				}
			}
		}

		template <class VT, class NODE, class KEY, class KEY_COMPARE, class VALUE, class VALUE_EQUALS>
		static void getValuesByKeyAndValue(List<VT>& list, NODE* look, KEY&& key, KEY_COMPARE&& key_compare, VALUE&& value, VALUE_EQUALS&& value_equals) noexcept
		{
			NODE *node, *end;
			if (getEqualRange(look, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), &node, &end)) {
				for (;;) {
					if (value_equals(node->value, Forward<VALUE>(value))) {
						list.add_NoLock(node->value);
					}
					if (node == end) {
						break;
					}
					node = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
				}
			}
		}

		template <class NODE>
		static void insertNode(NODE** pRoot, NODE* node, NODE* where, sl_compare_result compare_result) noexcept
		{
			if (compare_result > 0) {
				where->left = node;
			} else {
				where->right = node;
			}
			node->parent = where;
			priv::rb_tree::Helper::rebalanceAfterInsert(reinterpret_cast<RedBlackTreeNode*>(node), reinterpret_cast<RedBlackTreeNode**>(pRoot));
		}

		template <class NODE, class KEY_COMPARE>
		static void addNode(NODE** pRoot, NODE* node, KEY_COMPARE&& key_compare) noexcept
		{
			NODE* look = *pRoot;
			if (look) {
				for (;;) {
					if (key_compare(look->key, node->key) > 0) {
						NODE* left = look->left;
						if (left) {
							look = left;
						} else {
							look->left = node;
							break;
						}
					} else {
						NODE* right = look->right;
						if (right) {
							look = right;
						} else {
							look->right = node;
							break;
						}
					}
				}
				node->parent = look;
				priv::rb_tree::Helper::rebalanceAfterInsert(reinterpret_cast<RedBlackTreeNode*>(node), reinterpret_cast<RedBlackTreeNode**>(pRoot));
			} else {
				*pRoot = node;
			}

		}

		template <class NODE, class KEY, class KEY_COMPARE, class VALUE>
		static NODE* put(NODE** pRoot, sl_size& count, KEY&& key, KEY_COMPARE&& key_compare, VALUE&& value, sl_bool* isInsertion) noexcept
		{
			NODE* root = *pRoot;
			if (root) {
				sl_compare_result compare_result;
				NODE* where = tryFind(root, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), compare_result);
				if (!compare_result) {
					where->value = Forward<VALUE>(value);
					if (isInsertion) {
						*isInsertion = sl_false;
					}
					return where;
				}
				NODE* node = new NODE(Forward<KEY>(key), Forward<VALUE>(value));
				if (node) {
					insertNode(pRoot, node, where, compare_result);
					count++;
					if (isInsertion) {
						*isInsertion = sl_true;
					}
					return node;
				}
			} else {
				NODE* node = new NODE(Forward<KEY>(key), Forward<VALUE>(value));
				if (node) {
					*pRoot = node;
					count++;
					if (isInsertion) {
						*isInsertion = sl_true;
					}
					return node;
				}
			}
			if (isInsertion) {
				*isInsertion = sl_false;
			}
			return sl_null;
		}

		template <class NODE, class KEY, class KEY_COMPARE, class VALUE>
		static NODE* replace(NODE* root, KEY&& key, KEY_COMPARE&& key_compare, VALUE&& value) noexcept
		{
			if (!root) {
				return sl_null;
			}
			sl_compare_result compare_result;
			NODE* node = tryFind(root, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), compare_result);
			if (!compare_result) {
				node->value = Forward<VALUE>(value);
				return node;
			} else {
				return sl_null;
			}
		}

		template <class NODE, class KEY, class KEY_COMPARE, class... VALUE_ARGS>
		static NODE* add(NODE** pRoot, sl_size& count, KEY&& key, KEY_COMPARE&& key_compare, VALUE_ARGS&&... value_args) noexcept
		{
			NODE* node = new NODE(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
			if (node) {
				addNode(pRoot, node, Forward<KEY_COMPARE>(key_compare));
				count++;
				return node;
			}
			return sl_null;
		}

		template <class NODE, class KEY, class KEY_COMPARE, class... VALUE_ARGS>
		static MapEmplaceReturn<NODE> emplace(NODE** pRoot, sl_size& count, KEY&& key, KEY_COMPARE&& key_compare, VALUE_ARGS&&... value_args) noexcept
		{
			NODE* root = *pRoot;
			if (root) {
				sl_compare_result compare_result;
				NODE* where = tryFind(root, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), compare_result);
				if (!compare_result) {
					return MapEmplaceReturn<NODE>(sl_false, where);
				}
				NODE* node = new NODE(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				if (node) {
					insertNode(pRoot, node, where, compare_result);
					count++;
					return MapEmplaceReturn<NODE>(sl_true, node);
				}
			} else {
				NODE* node = new NODE(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...);
				if (node) {
					*pRoot = node;
					count++;
					return MapEmplaceReturn<NODE>(sl_true, node);
				}
			}
			return sl_null;
		}

		template <class NODE>
		static void removeNode(NODE** pRoot, sl_size& count, NODE* node) noexcept
		{
			count--;
			priv::rb_tree::Helper::removeNode(reinterpret_cast<RedBlackTreeNode*>(node), reinterpret_cast<RedBlackTreeNode**>(pRoot));
			delete node;
		}

		template <class NODE>
		static sl_size removeNodes(NODE** pRoot, sl_size& count, NODE* node, sl_size countRemove) noexcept
		{
			if (!countRemove) {
				return 0;
			}
			sl_size n = countRemove - 1;
			for (sl_size i = 0; i < n; i++) {
				NODE* next = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
				removeNode(pRoot, count, node);
				if (next) {
					node = next;
				} else {
					return i + 1;
				}
			}
			removeNode(pRoot, count, node);
			return countRemove;
		}

		template <class NODE>
		static sl_size removeRange(NODE** pRoot, sl_size& count, NODE* node, NODE* last) noexcept
		{
			if (!node) {
				node = *pRoot;
				if (node) {
					node = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getFirst(reinterpret_cast<RedBlackTreeNode*>(node)));
				} else {
					return 0;
				}
			}
			sl_size n = 0;
			for (;;) {
				n++;
				if (node == last) {
					removeNode(pRoot, count, node);
					break;
				} else {
					NODE* next = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
					removeNode(pRoot, count, node);
					node = next;
					if (!node) {
						break;
					}
				}
			}
			return n;
		}

		template <class NODE, class KEY, class KEY_COMPARE, class VALUE>
		static sl_bool remove(NODE** pRoot, sl_size& count, KEY&& key, KEY_COMPARE&& key_compare, VALUE* outValue) noexcept
		{
			NODE* root = *pRoot;
			if (root) {
				sl_compare_result compare_result;
				NODE* node = tryFind(root, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), compare_result);
				if (!compare_result) {
					if (outValue) {
						*outValue = Move(node->value);
					}
					removeNode(pRoot, count, node);
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class NODE, class KEY, class KEY_COMPARE>
		static sl_size removeItems(NODE** pRoot, sl_size& count, KEY&& key, KEY_COMPARE&& key_compare) noexcept
		{
			NODE* node;
			NODE* end;
			if (getEqualRange(*pRoot, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), &node, &end)) {
				sl_size n = 0;
				for (;;) {
					n++;
					if (node == end) {
						removeNode(pRoot, count, node);
						break;
					} else {
						NODE* next = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
						removeNode(pRoot, count, node);
						node = next;
					}
				}
				return n;
			} else {
				return 0;
			}
		}

		template <class VT, class NODE, class KEY, class KEY_COMPARE>
		static sl_size removeItemsAndReturnValues(List<VT>& list, NODE** pRoot, sl_size& count, KEY&& key, KEY_COMPARE&& key_compare) noexcept
		{
			NODE* node;
			NODE* end;
			if (getEqualRange(*pRoot, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), &node, &end)) {
				sl_size n = 0;
				for (;;) {
					n++;
					list.add_NoLock(node->value);
					if (node == end) {
						removeNode(pRoot, count, node);
						break;
					} else {
						NODE* next = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
						removeNode(pRoot, count, node);
						node = next;
					}
				}
				return n;
			} else {
				return 0;
			}
		}

		template <class NODE, class KEY, class KEY_COMPARE, class VALUE, class VALUE_EQUALS>
		static sl_bool removeKeyAndValue(NODE** pRoot, sl_size& count, KEY&& key, KEY_COMPARE&& key_compare, VALUE&& value, VALUE_EQUALS&& value_equals) noexcept
		{
			NODE* node;
			NODE* end;
			if (getEqualRange(*pRoot, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), &node, &end)) {
				for (;;) {
					if (value_equals(node->value, Forward<VALUE>(value))) {
						removeNode(pRoot, count, node);
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

		template <class NODE, class KEY, class KEY_COMPARE, class VALUE, class VALUE_EQUALS>
		static sl_size removeItemsByKeyAndValue(NODE** pRoot, sl_size& count, KEY&& key, KEY_COMPARE&& key_compare, VALUE&& value, VALUE_EQUALS&& value_equals) noexcept
		{
			sl_size n = 0;
			NODE* node;
			NODE* end;
			if (getEqualRange(*pRoot, Forward<KEY>(key), Forward<KEY_COMPARE>(key_compare), &node, &end)) {
				for (;;) {
					if (value_equals(node->value, Forward<VALUE>(value))) {
						n++;
						if (node == end) {
							removeNode(pRoot, count, node);
							break;
						} else {
							NODE* next = reinterpret_cast<NODE*>(priv::rb_tree::Helper::getNext(reinterpret_cast<RedBlackTreeNode*>(node)));
							removeNode(pRoot, count, node);
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

		template <class NODE>
		static void freeNodes(NODE* node) noexcept
		{
			NODE* stack[PRIV_SLIB_RED_BLACK_TREE_MAX_DISTANCE];
			sl_size nStack = 0;
			for(;;) {
				NODE* left = node->left;
				NODE* right = node->right;
				delete node;
				if (left) {
					if (right) {
						node = left;
						SLIB_ASSERT(nStack < PRIV_SLIB_RED_BLACK_TREE_MAX_DISTANCE)
						stack[nStack] = right;
						nStack++;
					} else {
						node = left;
					}
				} else {
					if (right) {
						node = right;
					} else {
						if (nStack > 0) {
							node = stack[nStack-1];
							nStack--;
						} else {
							break;
						}
					}
				}
			}
		}

		template <class NODE>
		static NODE* duplicateNode(NODE* nodeSource) noexcept
		{
			if (!nodeSource) {
				return sl_null;
			}

			NODE* nodeTarget = new NODE(nodeSource->key, nodeSource->value);
			if (!nodeTarget) {
				return sl_null;
			}

			NODE* nodeTargetRoot = nodeTarget;
			NODE* stackSource[PRIV_SLIB_RED_BLACK_TREE_MAX_DISTANCE];
			NODE* stackTarget[PRIV_SLIB_RED_BLACK_TREE_MAX_DISTANCE];
			sl_size nStack = 0;

			for(;;) {
				NODE* leftSource = nodeSource->left;
				NODE* leftTarget;
				if (leftSource) {
					leftTarget = new NODE(leftSource->key, leftSource->value);
					if (!leftTarget) {
						freeNodes(nodeTargetRoot);
						return sl_null;
					}
					leftTarget->flagRed = leftSource->flagRed;
					leftTarget->parent = nodeTarget;
					nodeTarget->left = leftTarget;
				} else {
					leftTarget = sl_null;
				}

				NODE* rightSource = nodeSource->right;
				NODE* rightTarget;
				if (rightSource) {
					rightTarget = new NODE(rightSource->key, rightSource->value);
					if (!rightTarget) {
						freeNodes(nodeTargetRoot);
						return sl_null;
					}
					rightTarget->flagRed = rightSource->flagRed;
					rightTarget->parent = nodeTarget;
					nodeTarget->right = rightTarget;
				} else {
					rightTarget = sl_null;
				}

				if (leftSource) {
					if (rightSource) {
						nodeSource = leftSource;
						nodeTarget = leftTarget;
						SLIB_ASSERT(nStack < PRIV_SLIB_RED_BLACK_TREE_MAX_DISTANCE)
						stackSource[nStack] = rightSource;
						stackTarget[nStack] = rightTarget;
						nStack++;
					} else {
						nodeSource = leftSource;
						nodeTarget = leftTarget;
					}
				} else {
					if (rightSource) {
						nodeSource = rightSource;
						nodeTarget = rightTarget;
					} else {
						if (nStack > 0) {
							nodeSource = stackSource[nStack-1];
							nodeTarget = stackTarget[nStack-1];
							nStack--;
						} else {
							break;
						}
					}
				}
			}

			return nodeTargetRoot;
		}

	};

}

#endif
