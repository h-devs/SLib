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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_MACOS)

#include "slib/ui/cursor.h"

#include "slib/ui/platform.h"

namespace slib
{

	namespace {
		class NativeCursorImpl : public Cursor
		{
		public:
			NSCursor* m_cursor;

		public:
			NativeCursorImpl(NSCursor* cursor): m_cursor(cursor)
			{
			}

		public:
			static Ref<NativeCursorImpl> create(NSCursor* cursor)
			{
				if (cursor != nil) {
					return new NativeCursorImpl(cursor);
				}
				return sl_null;
			}
		};
	}

	Ref<Cursor> UIPlatform::createCursor(NSCursor* cursor)
	{
		return NativeCursorImpl::create(cursor);
	}

	NSCursor* UIPlatform::getCursorHandle(Cursor* cursor)
	{
		if (!cursor) {
			return nil;
		}
		NativeCursorImpl* c = (NativeCursorImpl*)(cursor);
		return c->m_cursor;
	}

	Ref<Cursor> Cursor::getNone()
	{
		return new NativeCursorImpl(nil);
	}

	Ref<Cursor> Cursor::getArrow()
	{
		return UIPlatform::createCursor([NSCursor arrowCursor]);
	}

	Ref<Cursor> Cursor::getIBeam()
	{
		return UIPlatform::createCursor([NSCursor IBeamCursor]);
	}

	Ref<Cursor> Cursor::getCross()
	{
		return UIPlatform::createCursor([NSCursor crosshairCursor]);
	}

	Ref<Cursor> Cursor::getHand()
	{
		return UIPlatform::createCursor([NSCursor pointingHandCursor]);
	}

	Ref<Cursor> Cursor::getResizeLeftRight()
	{
		return UIPlatform::createCursor([NSCursor resizeLeftRightCursor]);
	}

	Ref<Cursor> Cursor::getResizeUpDown()
	{
		return UIPlatform::createCursor([NSCursor resizeUpDownCursor]);
	}

	void Cursor::setCurrent(const Ref<Cursor>& cursor)
	{
		if (cursor.isNotNull()) {
			NSCursor* handle = ((NativeCursorImpl*)(cursor.get()))->m_cursor;
			if (handle != nil) {
				[handle set];
			}
		}
	}

	Ref<Cursor> Cursor::getCurrent()
	{
		return UIPlatform::createCursor([NSCursor currentCursor]);
	}

	void Cursor::show()
	{
		[NSCursor unhide];
	}

	void Cursor::hide()
	{
		[NSCursor hide];
	}

}

#endif
