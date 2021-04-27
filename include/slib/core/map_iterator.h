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

#ifndef CHECKHEADER_SLIB_CORE_MAP_ITERATOR
#define CHECKHEADER_SLIB_CORE_MAP_ITERATOR

#include "iterator.h"
#include "cast.h"

namespace slib
{

	template <class CMAP, class KT = typename CMAP::KEY_TYPE, class VT = typename CMAP::VALUE_TYPE>
	class MapIterator : public CIterator<KT, VT>
	{
	public:
		Ref<CMAP> map;
		typename CMAP::NODE* node;

	public:
		template <class T>
		MapIterator(T&& t): map(Forward<T>(t)), node(sl_null) {}

	public:
		KT getKey() override
		{
			return Cast<typename CMAP::KEY_TYPE, KT>()(node->key);
		}

		VT getValue() override
		{
			return Cast<typename CMAP::VALUE_TYPE, VT>()(node->value);
		}

		sl_bool moveNext() override
		{
			if (node) {
				node = node->getNext();
			} else {
				node = map->getFirstNode();
			}
			return node != sl_null;
		}

		sl_bool seek(const KT& key) override
		{
			node = map->getLowerBound(Cast<KT, typename CMAP::KEY_TYPE>()(key));
			return node != sl_null;
		}

	};

	template <class CMAP>
	class MapIterator<CMAP, typename CMAP::KEY_TYPE, typename CMAP::VALUE_TYPE> : public CIterator<typename CMAP::KEY_TYPE, typename CMAP::VALUE_TYPE>
	{
	public:
		Ref<CMAP> map;
		typename CMAP::NODE* node;

	public:
		template <class T>
		MapIterator(T&& t): map(Forward<T>(t)), node(sl_null) {}

	public:
		typename CMAP::KEY_TYPE getKey() override
		{
			return node->key;
		}

		typename CMAP::VALUE_TYPE getValue() override
		{
			return node->value;
		}

		sl_bool moveNext() override
		{
			if (node) {
				node = node->getNext();
			} else {
				node = map->getFirstNode();
			}
			return node != sl_null;
		}

		sl_bool seek(const typename CMAP::KEY_TYPE& key) override
		{
			node = map->getLowerBound(key);
			return node != sl_null;
		}

	};

}

#endif
