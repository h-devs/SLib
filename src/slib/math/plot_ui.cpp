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

#include "slib/math/plot.h"

#include "slib/ui/window.h"
#include "slib/ui/view.h"

namespace slib
{

	Ref<Window> Plot::show(sl_uint32 width, sl_uint32 height)
	{
		Ref<Window> window = new Window;
		if (window.isNotNull()) {
			Ref<View> view = window->getContentView();
			if (view.isNotNull()) {
				if (getReferenceCount()) {
					WeakRef<Plot> thiz = this;
					view->setOnDraw([thiz, this](View* view, Canvas* canvas) {
						Ref<Plot> ref = thiz;
						if (ref.isNull()) {
							return;
						}
						draw(canvas, view->getWidth(), view->getHeight());
					});
				} else {
					view->setOnDraw([this](View* view, Canvas* canvas) {
						draw(canvas, view->getWidth(), view->getHeight());
					});
				}
				view->setOpaque();
				window->setResizable();
				window->setCenterScreen();
				window->setMaximizeButtonEnabled();
				window->setMinimizeButtonEnabled();
				window->setClientWidth(width);
				window->setClientHeight(height);
				window->showAndKeep();
				return window;
			}
		}
		return sl_null;
	}

}
