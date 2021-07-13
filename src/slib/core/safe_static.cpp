/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/safe_static.h"

#include "slib/core/queue.h"

namespace slib
{

	namespace priv
	{
		namespace safe_static
		{

			static Queue<IFreeable*>* g_listFreeables = sl_null;
			static sl_bool g_flagFreed = sl_false;

			class FreeHelper
			{
			public:
				constexpr FreeHelper() {}

				~FreeHelper()
				{
					g_flagFreed = sl_true;
					if (g_listFreeables) {
						IFreeable* obj;
						while (g_listFreeables->pop_NoLock(&obj)) {
							delete obj;
						}
						delete g_listFreeables;
						g_listFreeables = sl_null;
					}
				}

			} g_freeHelper;

			void FreeObjectOnExitImpl(IFreeable* obj)
			{
				if (g_flagFreed) {
					delete obj;
					return;
				}
				if (!g_listFreeables) {
					SLIB_STATIC_SPINLOCKER(lock)
					if (!g_listFreeables) {
						g_listFreeables = new Queue<IFreeable*>;
						if (!g_listFreeables) {
							return;
						}
					}
				}
				g_listFreeables->push(obj);
			}

		}
	}

}
