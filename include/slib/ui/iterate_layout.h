/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_ITERATE_LAYOUT
#define CHECKHEADER_SLIB_UI_ITERATE_LAYOUT

#include "linear_layout.h"

namespace slib
{

	template <class PARENT, class CHILD_LAYOUT>
	class SLIB_EXPORT IterateLayout : public PARENT
	{
	public:
		const CList< Ref<CHILD_LAYOUT> >& getChildren()
		{
			return m_children;
		}

		void addChild(const Ref<CHILD_LAYOUT>& child, UIUpdateMode mode = UIUpdateMode::UpdateLayout)
		{
			m_children.add(child);
			PARENT::addChild(child, mode);
		}

		void removeChild(const Ref<CHILD_LAYOUT>& child, UIUpdateMode mode = UIUpdateMode::UpdateLayout)
		{
			m_children.remove(child);
			PARENT::removeChild(child, mode);
		}

		void removeAllChildren(UIUpdateMode mode = UIUpdateMode::UpdateLayout)
		{
			m_children.removeAll();
			PARENT::removeAllChildren(mode);
		}

	protected:
		CList< Ref<CHILD_LAYOUT> > m_children;

	};

	template <class CHILD_LAYOUT>
	using VerticalIterateLayout = IterateLayout<VerticalLinearLayout, CHILD_LAYOUT>;

	template <class CHILD_LAYOUT>
	using HorizontalIterateLayout = IterateLayout<HorizontalLinearLayout, CHILD_LAYOUT>;

}

#endif
