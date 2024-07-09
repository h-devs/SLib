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

#ifndef CHECKHEADER_SLIB_UI_TILE_LAYOUT
#define CHECKHEADER_SLIB_UI_TILE_LAYOUT

#include "view.h"

namespace slib
{

	class SLIB_EXPORT TileLayout : public ViewGroup
	{
		SLIB_DECLARE_OBJECT

	public:
		TileLayout();

		~TileLayout();

	public:
		sl_uint32 getColumnCount();

		void setColumnCount(sl_uint32 nColumns, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_uint32 getRowCount();

		void setRowCount(sl_uint32 nRows, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getColumnWidth();

		void setColumnWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getRowHeight();

		void setRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		float getCellRatio();

		void setCellRatio(float ratio, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<Pen> getGrid();

		void setGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);

	protected:
		void onAddChild(View* child) override;

		void onUpdateLayout() override;

		void dispatchDraw(Canvas* canvas) override;

	private:
		sl_uint32 m_nColumns;
		sl_uint32 m_nRows;
		sl_ui_len m_columnWidth;
		sl_ui_len m_rowHeight;
		float m_cellRatio;
		AtomicRef<Pen> m_grid;

		sl_uint32 m_nCurrentColumns;
		sl_uint32 m_nCurrentRows;
		sl_ui_len m_currentColumnWidth;
		sl_ui_len m_currentRowHeight;

	};

}

#endif
