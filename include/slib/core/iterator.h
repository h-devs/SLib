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

#ifndef CHECKHEADER_SLIB_CORE_ITERATOR
#define CHECKHEADER_SLIB_CORE_ITERATOR

#include "ref.h"

namespace slib
{

	class SLIB_EXPORT CIteratorBase : public CRef
	{
		SLIB_DECLARE_OBJECT

	public:
		CIteratorBase();

		~CIteratorBase();

	};

	template <class KT, class VT>
	class SLIB_EXPORT CIterator : public CIteratorBase
	{
	public:
		typedef KT KEY_TYPE;
		typedef VT VALUE_TYPE;

	public:
		virtual KT getKey() = 0;

		virtual VT getValue() = 0;

		virtual sl_bool moveNext() = 0;

		virtual sl_bool seek(const KT& key) = 0;

	};

	template <class KT, class VT>
	class SLIB_EXPORT Iterator
	{
	public:
		typedef CIterator<KT, VT> CITERATOR;
		typedef KT KEY_TYPE;
		typedef VT VALUE_TYPE;

	public:
		Ref<CITERATOR> ref;
		SLIB_REF_WRAPPER_NO_ATOMIC(Iterator, CITERATOR)

	public:
		KT getKey() const
		{
			CITERATOR* c = ref.ptr;
			if (c) {
				return c->getKey();
			} else {
				return KT();
			}
		}

		VT getValue() const
		{
			CITERATOR* c = ref.ptr;
			if (c) {
				return c->getValue();
			} else {
				return VT();
			}
		}

		sl_bool moveNext() const
		{
			CITERATOR* c = ref.ptr;
			if (c) {
				return c->moveNext();
			} else {
				return sl_false;
			}
		}

		sl_bool seek(const KT& key) const
		{
			CITERATOR* c = ref.ptr;
			if (c) {
				return c->seek(key);
			} else {
				return sl_false;
			}
		}

		SLIB_DEFINE_CAST_REF_FUNCTIONS(class... TYPES, Iterator, Iterator<TYPES...>)

	};

}

#endif
