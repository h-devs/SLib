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

#ifndef CHECKHEADER_SLIB_UI_LINEAR_LAYOUT
#define CHECKHEADER_SLIB_UI_LINEAR_LAYOUT

#include "view.h"

namespace slib
{

	class SLIB_EXPORT LinearLayout : public ViewGroup
	{
		SLIB_DECLARE_OBJECT
		
	public:
		LinearLayout();
		
		~LinearLayout();

	public:
		LayoutOrientation getOrientation();
		
		void setOrientation(LayoutOrientation orientation, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isHorizontal();
		
		void setHorizontal(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isVertical();
		
		void setVertical(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
	protected:
		void onAddChild(View* child) override;
		
		void onUpdateLayout() override;
		
	private:
		LayoutOrientation m_orientation;
		
	};
	
	class SLIB_EXPORT VerticalLinearLayout : public LinearLayout
	{
	public:
		VerticalLinearLayout();

		~VerticalLinearLayout();

	};
	
	class SLIB_EXPORT HorizontalLinearLayout : public LinearLayout
	{
	public:
		HorizontalLinearLayout();

		~HorizontalLinearLayout();

	};

}

#endif
