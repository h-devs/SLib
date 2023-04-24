/*
*   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_VIEW_STATE_MAP
#define CHECKHEADER_SLIB_UI_VIEW_STATE_MAP

#include "constants.h"

#include "../core/ref.h"

namespace slib
{

	template <class VALUE>
	class ViewStateMap
	{
	public:
		Atomic<VALUE> defaultValue;
		AtomicRef<CRef> values;

	public:
		VALUE get(ViewState state);

		void set(ViewState state, const VALUE& value);

		void setNonDefault(ViewState state, const VALUE& value);

		void remove(ViewState state);

		VALUE evaluate(ViewState state, sl_bool* outFlagReturnDefault = sl_null);

		void copyFrom(const ViewStateMap& other);

	private:
		template <class MAP>
		static sl_bool _evaluate(VALUE& ret, const MAP& map, ViewState state, ViewState def, sl_bool& flagReturnDefault);

		template <class MAP>
		static sl_bool _evaluate(VALUE& ret, const MAP& map, ViewState state, sl_bool& flagReturnDefault);

	};

}

#endif