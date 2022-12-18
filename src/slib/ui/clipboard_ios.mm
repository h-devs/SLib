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

#include "slib/ui/clipboard.h"

#if defined(SLIB_UI_IS_IOS)

#include "slib/platform.h"

namespace slib
{

	sl_bool Clipboard::hasText()
	{
		UIPasteboard* board = [UIPasteboard generalPasteboard];
		if (board != nil) {
			return [board hasStrings] ? sl_true : sl_false;
		}
		return sl_false;
	}

	String Clipboard::getText()
	{
		UIPasteboard* board = [UIPasteboard generalPasteboard];
		if (board != nil) {
			NSString* text = [board string];
			return Apple::getStringFromNSString(text);
		}
		return sl_null;
	}

	void Clipboard::setText(const StringParam& text)
	{
		UIPasteboard* board = [UIPasteboard generalPasteboard];
		if (board != nil) {
			NSString* str = Apple::getNSStringFromString(text);
			[board setString:str];
		}
	}

}

#endif
