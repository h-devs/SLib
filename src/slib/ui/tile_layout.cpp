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

#include "slib/ui/tile_layout.h"

#include "slib/core/scoped_buffer.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(TileLayout, ViewGroup)
	
	TileLayout::TileLayout()
	{
		setCustomLayout(sl_true);
		setSavingCanvasState(sl_false);
		
		m_countColumns = 0;
		m_countRows = 0;
		m_widthColumn = 0;
		m_heightRow = 0;
		m_ratioCell = 1;
	}
	
	TileLayout::~TileLayout()
	{
	}

	sl_uint32 TileLayout::getColumnCount()
	{
		return m_countColumns;
	}

	void TileLayout::setColumnCount(sl_uint32 nColumns, UIUpdateMode mode)
	{
		m_countColumns = nColumns;
		invalidateLayout(mode);
	}

	sl_uint32 TileLayout::getRowCount()
	{
		return m_countRows;
	}

	void TileLayout::setRowCount(sl_uint32 nRows, UIUpdateMode mode)
	{
		m_countRows = nRows;
		invalidateLayout(mode);
	}

	sl_ui_len TileLayout::getColumnWidth()
	{
		return m_widthColumn;
	}

	void TileLayout::setColumnWidth(sl_ui_len width, UIUpdateMode mode)
	{
		m_widthColumn = width;
		invalidateLayout(mode);
	}

	sl_ui_len TileLayout::getRowHeight()
	{
		return m_heightRow;
	}

	void TileLayout::setRowHeight(sl_ui_len height, UIUpdateMode mode)
	{
		m_heightRow = height;
		invalidateLayout(mode);
	}

	float TileLayout::getCellRatio()
	{
		return m_ratioCell;
	}

	void TileLayout::setCellRatio(float ratio, UIUpdateMode mode)
	{
		m_ratioCell = ratio;
		invalidateLayout(mode);
	}

	void TileLayout::onUpdateLayout()
	{
		ListElements< Ref<View> > children(getChildren());

		if (!(children.count)) {
			if (isWidthWrapping()) {
				setLayoutWidth(getPaddingLeft() + getPaddingRight());
			}
			if (isHeightWrapping()) {
				setLayoutHeight(getPaddingTop() + getPaddingBottom());
			}
			return;
		}

		UIRect layoutFrameContainer = getLayoutFrame();
		sl_ui_len widthLayout = layoutFrameContainer.getWidth();
		sl_ui_len heightLayout = layoutFrameContainer.getHeight();
		sl_ui_len widthContainer = widthLayout - getPaddingLeft() - getPaddingTop();
		sl_ui_len heightContainer = heightLayout - getPaddingTop() - getPaddingBottom();

		sl_ui_len widthCol = m_widthColumn;
		sl_ui_len heightRow = m_heightRow;
		sl_uint32 nCols = m_countColumns;
		sl_uint32 nRows = m_countRows;

		sl_bool flagWrapX = isWidthWrapping();
		sl_bool flagWrapY = isHeightWrapping();
		sl_bool flagWrapCellX = sl_false;
		sl_bool flagWrapCellY = sl_false;

		if (widthCol <= 0) {
			if (!nCols) {
				nCols = 1;
			}
			if (flagWrapX) {
				flagWrapCellX = sl_true;
			} else {
				widthCol = (sl_uint32)widthContainer / nCols;
			}
		} else {
			if (flagWrapX) {
				nCols = 1;
			} else {
				nCols = widthContainer / widthCol;
				if (nCols <= 0) {
					nCols = 1;
				}
			}
		}
		if (heightRow <= 0) {
			if (!nRows) {
				nRows = 1;
			}
			if (flagWrapY) {
				flagWrapCellY = sl_true;
			} else {
				heightRow = (sl_uint32)heightContainer / nRows;
			}
		}

		float ratio = m_ratioCell;
		if (ratio < 0.0001f) {
			ratio = 0.0001f;
		}
		if (flagWrapCellX) {
			if (flagWrapCellY) {
				widthCol = 100;
				heightRow = (sl_ui_len)(widthCol / ratio);
			} else {
				widthCol = (sl_ui_len)(heightRow * ratio);
			}
		} else {
			if (flagWrapCellY) {
				heightRow = (sl_ui_len)(widthCol / ratio);
			}
		}

		UpdateLayoutFrameParam updateLayoutParam;
		updateLayoutParam.flagUseLayout = sl_true;
		updateLayoutParam.flagHorizontal = sl_true;
		updateLayoutParam.flagVertical = sl_true;

		sl_ui_len x = getPaddingLeft();
		sl_ui_len y = getPaddingTop();
		sl_size i = 0;
		for (; i < children.count; i++) {
			View* view = children[i].get();
			if (view) {
				updateLayoutParam.parentContentFrame.left = x;
				updateLayoutParam.parentContentFrame.top = y;
				updateLayoutParam.parentContentFrame.right = x + widthCol;
				updateLayoutParam.parentContentFrame.bottom = y + heightRow;
				view->setInvalidateLayoutFrameInParent();
				view->updateLayoutFrameInParent(updateLayoutParam);
				if ((i + 1) % nCols == 0) {
					x = 0;
					y += heightRow;
				} else {
					x += widthCol;
				}
			}
		}
		if (isWidthWrapping()) {
			setLayoutWidth((sl_ui_len)nCols * widthCol + getPaddingLeft() + getPaddingRight());
		}
		if (isHeightWrapping()) {
			if (i % nCols == 0) {
				setLayoutHeight(y + getPaddingBottom());
			} else {
				setLayoutHeight(y + heightRow + getPaddingBottom());
			}
		}
	}
	
}
