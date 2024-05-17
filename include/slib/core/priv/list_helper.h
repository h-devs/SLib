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

#ifndef CHECKHEADER_SLIB_CORE_LIST_HELPER
#define CHECKHEADER_SLIB_CORE_LIST_HELPER

#include "../list.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <vector>
#endif

namespace slib
{

	template <class LIST>
	class ListHelper
	{
	public:
		static void clear(LIST& list)
		{
			list.setNull();
		}

		static sl_bool create(LIST& list, sl_size n)
		{
			list = LIST::create(n);
			return list.isNotNull();
		}

		static sl_bool create(LIST& list)
		{
			list = LIST::create();
			return list.isNotNull();
		}

		static typename LIST::ELEMENT_TYPE* getData(LIST& list)
		{
			return list.getData();
		}

	};


#ifdef SLIB_SUPPORT_STD_TYPES
	template <class T, class ALLOC>
	class ListHelper< std::vector<T, ALLOC> >
	{
	public:
		static void clear(std::vector<T, ALLOC>& list)
		{
			list.clear();
		}

		static sl_bool create(std::vector<T, ALLOC>& list, sl_size n)
		{
			list.resize(n);
			return list.size() == n;
		}

		static sl_bool create(std::vector<T, ALLOC>& list)
		{
			list.clear();
			return sl_true;
		}

		static T* getData(std::vector<T, ALLOC>& list)
		{
			return list.data();
		}

	};
#endif

}

#endif