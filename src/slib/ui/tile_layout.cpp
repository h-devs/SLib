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

#include "slib/ui/tile_layout.h"

#include "slib/graphics/canvas.h"
#include "slib/core/scoped_buffer.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(TileLayout, ViewGroup)

	TileLayout::TileLayout()
	{
		setCustomLayout(sl_true);
		setSavingCanvasState(sl_false);

		m_nColumns = 0;
		m_nRows = 0;
		m_columnWidth = 0;
		m_rowHeight = 0;
		m_cellRatio = 1;

		m_nCurrentColumns = 0;
		m_nCurrentRows = 0;
		m_currentColumnWidth = 0;
		m_currentRowHeight = 0;
	}

	TileLayout::~TileLayout()
	{
	}

	sl_uint32 TileLayout::getColumnCount()
	{
		return m_nColumns;
	}

	void TileLayout::setColumnCount(sl_uint32 nColumns, UIUpdateMode mode)
	{
		m_nColumns = nColumns;
		invalidateLayout(mode);
	}

	sl_uint32 TileLayout::getRowCount()
	{
		return m_nRows;
	}

	void TileLayout::setRowCount(sl_uint32 nRows, UIUpdateMode mode)
	{
		m_nRows = nRows;
		invalidateLayout(mode);
	}

	sl_ui_len TileLayout::getColumnWidth()
	{
		return m_columnWidth;
	}

	void TileLayout::setColumnWidth(sl_ui_len width, UIUpdateMode mode)
	{
		m_columnWidth = width;
		invalidateLayout(mode);
	}

	sl_ui_len TileLayout::getRowHeight()
	{
		return m_rowHeight;
	}

	void TileLayout::setRowHeight(sl_ui_len height, UIUpdateMode mode)
	{
		m_rowHeight = height;
		invalidateLayout(mode);
	}

	float TileLayout::getCellRatio()
	{
		return m_cellRatio;
	}

	void TileLayout::setCellRatio(float ratio, UIUpdateMode mode)
	{
		m_cellRatio = ratio;
		invalidateLayout(mode);
	}

	Ref<Pen> TileLayout::getGrid()
	{
		return m_grid;
	}

	void TileLayout::setGrid(const Ref<Pen>& pen, UIUpdateMode mode)
	{
		m_grid = pen;
		invalidate(mode);
	}

	void TileLayout::setGrid(const PenDesc& desc, UIUpdateMode mode)
	{
		m_grid = Pen::create(desc, m_grid);
		invalidate(mode);
	}

	void TileLayout::onAddChild(View* child)
	{
		if (child->isTopFree() && child->isBottomFree()) {
			child->setAlignParentTop(UIUpdateMode::Init);
		}
		if (child->isLeftFree() && child->isRightFree()) {
			child->setAlignParentLeft(UIUpdateMode::Init);
		}
	}

	void TileLayout::onUpdateLayout()
	{
		ListElements< Ref<View> > children(getChildren());

		if (!(children.count)) {
			if (isLastWidthWrapping()) {
				setLayoutWidth(getPaddingLeft() + getPaddingRight());
			}
			if (isLastHeightWrapping()) {
				setLayoutHeight(getPaddingTop() + getPaddingBottom());
			}
			return;
		}

		UIRect layoutFrameContainer = getLayoutFrame();
		sl_ui_len widthLayout = layoutFrameContainer.getWidth();
		sl_ui_len heightLayout = layoutFrameContainer.getHeight();
		sl_ui_len widthContainer = widthLayout - getPaddingLeft() - getPaddingTop();
		sl_ui_len heightContainer = heightLayout - getPaddingTop() - getPaddingBottom();

		sl_ui_len widthCol = m_columnWidth;
		sl_ui_len heightRow = m_rowHeight;
		sl_uint32 nCols = m_nColumns;
		sl_uint32 nRows = m_nRows;

		sl_bool flagWrapX = isLastWidthWrapping();
		sl_bool flagWrapY = isLastHeightWrapping();
		sl_bool flagWrapCellX = sl_false;
		sl_bool flagWrapCellY = sl_false;

		if (widthCol <= 0) {
			if (nCols && !flagWrapX) {
				widthCol = (sl_uint32)widthContainer / nCols;
			} else {
				flagWrapCellX = sl_true;
			}
		}
		if (heightRow <= 0) {
			if (nRows && !flagWrapY) {
				heightRow = (sl_uint32)heightContainer / nRows;
			} else {
				flagWrapCellY = sl_true;
			}
		}
		float ratio = m_cellRatio;
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

		if (nCols < 1) {
			if (!flagWrapX) {
				nCols = widthContainer / widthCol;
			}
			if (nCols < 1) {
				nCols = 1;
			}
		}

		UpdateLayoutFrameParam updateLayoutParam;
		updateLayoutParam.flagUseLayout = sl_true;
		updateLayoutParam.flagHorizontal = sl_true;
		updateLayoutParam.flagVertical = sl_true;

		sl_ui_len x = getPaddingLeft();
		sl_ui_len y = getPaddingTop();
		sl_size n = 0;
		{
			for (sl_size i = 0; i < children.count; i++) {
				View* view = children[i].get();
				if (view && view->getVisibility() != Visibility::Gone) {
					updateLayoutParam.parentContentFrame.left = x;
					updateLayoutParam.parentContentFrame.top = y;
					updateLayoutParam.parentContentFrame.right = x + widthCol;
					updateLayoutParam.parentContentFrame.bottom = y + heightRow;
					view->setInvalidateLayoutFrameInParent();
					view->updateLayoutFrameInParent(updateLayoutParam);
					n++;
					if (n % nCols) {
						x += widthCol;
					} else {
						x = getPaddingLeft();
						y += heightRow;
					}
				}
			}
		}
		if (flagWrapX) {
			setLayoutWidth((sl_ui_len)nCols * widthCol + getPaddingLeft() + getPaddingRight());
		}
		if (flagWrapY) {
			if (n % nCols) {
				setLayoutHeight(y + heightRow + getPaddingBottom());
			} else {
				setLayoutHeight(y + getPaddingBottom());
			}
		}

		m_nCurrentColumns = nCols;
		m_nCurrentRows = nRows;
		m_currentColumnWidth = widthCol;
		m_currentRowHeight = heightRow;
	}

	void TileLayout::dispatchDraw(Canvas* canvas)
	{
		ViewGroup::dispatchDraw(canvas);

		Ref<Pen> grid = m_grid;
		if (grid.isNotNull()) {
			UIRect bounds = getBoundsInnerPadding();
			UIEdgeInsets padding = getPadding();
			sl_ui_len y = bounds.top;
			if (padding.top) {
				canvas->drawLine((sl_real)(bounds.left), (sl_real)y, (sl_real)(bounds.right), (sl_real)y, grid);
			}
			for (sl_uint32 row = 0; row + 1 < m_nCurrentRows; row++) {
				y += m_currentRowHeight;
				canvas->drawLine((sl_real)(bounds.left), (sl_real)y, (sl_real)(bounds.right), (sl_real)y, grid);
			}
			sl_ui_len x = bounds.left;
			if (padding.left) {
				canvas->drawLine((sl_real)x, (sl_real)(bounds.top), (sl_real)x, (sl_real)(bounds.bottom), grid);
			}
			for (sl_uint32 col = 0; col + 1 < m_nCurrentColumns; col++) {
				x += m_currentColumnWidth;
				canvas->drawLine((sl_real)x, (sl_real)(bounds.top), (sl_real)x, (sl_real)(bounds.bottom), grid);
			}
		}
	}

}
