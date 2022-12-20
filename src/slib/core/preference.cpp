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

#include "slib/core/preference.h"

#include "slib/data/json.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace priv
	{
		namespace preference
		{
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_appKey)
		}
	}

	using namespace priv::preference;

	void Preference::removeValue(const StringParam& key)
	{
		setValue(key, sl_null);
	}

	String Preference::getApplicationKeyName()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_appKey)) {
			return sl_null;
		}
		return g_appKey;
	}

	void Preference::setApplicationKeyName(const String& name)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_appKey)) {
			return;
		}
		g_appKey = name;
	}

}
