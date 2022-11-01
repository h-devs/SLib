/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UIX_CONTROL
#define CHECKHEADER_SLIB_UIX_CONTROL

#include "definition.h"

#include "../ui/view.h"

namespace slib
{

	class SLIB_EXPORT XControlProvider
	{
	public:
		XControlProvider();

		~XControlProvider();

	public:
		void initXControl(View* view);

	public:
		Color getFocusedBorderColor();

		void setFocusedBorderColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

	protected:
		void drawXControlBorder(Canvas* canvas);

	protected:
		View* m_view;
		Ref<Pen> m_focusedBorder;

	};

	class SLIB_EXPORT XControl : public ViewGroup, public XControlProvider
	{
		SLIB_DECLARE_OBJECT

	public:
		XControl();

		~XControl();

	protected:
		void init() override;

	protected:
		void onDrawBorder(Canvas* canvas) override;

	};

}

#endif
