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

#ifndef CHECKHEADER_SLIB_UI_DRAG
#define CHECKHEADER_SLIB_UI_DRAG

#include "constants.h"

#include "../core/string.h"
#include "../core/list.h"
#include "../core/default_members.h"

namespace slib
{

	class View;
	class Drawable;

	class SLIB_EXPORT DragItem
	{
	public:
		DragItem();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DragItem)

	public:
		void clear();

		const String& getText() const;

		void setText(const String& text);

		const List<String>& getFiles() const;

		void setFiles(const List<String>& files);

		const UIRect& getFrame() const;

		void setFrame(const UIRect& frame);

		void setDraggingSize(sl_ui_pos width, sl_ui_pos height);

		const Ref<Drawable>& getDraggingImage() const;

		void setDraggingImage(const Ref<Drawable>& image);

	protected:
		String m_text;
		List<String> m_files;
		UIRect m_frame;
		Ref<Drawable> m_image;

	};

	class SLIB_EXPORT DragContext
	{
	public:
		Ref<View> view;
		DragItem item;
		DragOperations operationMask;
		DragOperations operation;

	public:
		DragContext();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DragContext)

	public:
		sl_bool isAlive() const;

		void release();

	};

}

#endif
