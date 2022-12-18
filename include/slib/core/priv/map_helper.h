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

#ifndef CHECKHEADER_SLIB_CORE_MAP_HELPER
#define CHECKHEADER_SLIB_CORE_MAP_HELPER

#include "../map.h"
#include "../hash_map.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <map>
#include <unordered_map>
#endif

namespace slib
{

	template <class MAP>
	class MapHelper
	{
	public:
		static void clear(MAP& map)
		{
			map.setNull();
		}

		template <class... ARGS>
		static sl_bool add(MAP& map, ARGS&&... args)
		{
			return map.add_NoLock(Forward<ARGS>(args)...) != sl_null;
		}

	};

#ifdef SLIB_SUPPORT_STD_TYPES
	template <class KT, class... TYPES>
	class MapHelper< std::map<KT, TYPES...> >
	{
	public:
		static void clear(std::map<KT, TYPES...>& map)
		{
			map.clear();
		}

		template <class... ARGS>
		static sl_bool add(std::map<KT, TYPES...>& map, ARGS&&... args)
		{
			map.emplace(Forward<ARGS>(args)...);
			return sl_true;
		}

	};

	template <class KT, class... TYPES>
	class MapHelper< std::unordered_map<KT, TYPES...> >
	{
	public:
		static void clear(std::unordered_map<KT, TYPES...>& map)
		{
			map.clear();
		}

		template <class... ARGS>
		static sl_bool add(std::unordered_map<KT, TYPES...>& map, ARGS&&... args)
		{
			map.emplace(Forward<ARGS>(args)...);
			return sl_true;
		}

	};
#endif

}

#endif