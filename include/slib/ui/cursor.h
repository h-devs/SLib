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

#ifndef CHECKHEADER_SLIB_UI_CURSOR
#define CHECKHEADER_SLIB_UI_CURSOR

#include "definition.h"

#include "../core/object.h"

namespace slib
{

	class SLIB_EXPORT Cursor : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		Cursor();

		~Cursor();

	public:
		static Ref<Cursor> getNone();

		static Ref<Cursor> getArrow();

		static Ref<Cursor> getIBeam();

		static Ref<Cursor> getCross();

		static Ref<Cursor> getHand();

		static Ref<Cursor> getResizeLeftRight();

		static Ref<Cursor> getResizeUpDown();

	public:
		static void setCurrent(const Ref<Cursor>& cursor);

		static Ref<Cursor> getCurrent();

	public:
		static void show();

		static void hide();

	};

}

#endif
