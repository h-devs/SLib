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

#include "slib/ui/pdf_view.h"

#include "slib/doc/pdf.h"
#include "slib/graphics/bitmap.h"
#include "slib/ui/scroll_bar.h"

#define EXPIRE_DURATION_PAGE 5000
#define EXPIRE_DURATION_FONT 10000
#define EXPIRE_DURATION_XOBJECT 500
#define EXPIRE_DURATION_PAGE_CACHE 7000

#define BACKGROUND_COLOR Color::White
#define BORDER_COLOR Color::Gray

#define MIN_PAGE_RATIO 0.1f
#define MAX_PAGE_RATIO 5.0f

namespace slib
{

	class PdfViewContext : public PdfResourceCache, public Lockable
	{
	public:
		Ref<PdfDocument> doc;
		String filePath;
		sl_uint32 nPages = 1;
		ExpiringMap< sl_uint32, Ref<PdfPage> > pages;
		ExpiringMap< sl_uint32, Ref<Bitmap> > pageCache;
		CList<float> pageRatios;
		float defaultPageRatio = 1.0f;

	public:
		PdfViewContext()
		{
			pageCache.setExpiringMilliseconds(EXPIRE_DURATION_PAGE_CACHE);
			pages.setExpiringMilliseconds(EXPIRE_DURATION_PAGE);
			fonts.setExpiringMilliseconds(EXPIRE_DURATION_FONT);
			xObjects.setExpiringMilliseconds(EXPIRE_DURATION_XOBJECT);
		}

	public:
		sl_bool initialize(const String& _filePath, PdfDocument* _doc, sl_uint32 _nPages)
		{
			doc = _doc;
			filePath = _filePath;
			nPages = _nPages;
			if (nPages < 1) {
				return sl_false;
			}
			if (!(pageRatios.setCount_NoLock(nPages))) {
				return sl_false;
			}
			Ref<PdfPage> pageFirst = getPage(0);
			if (pageFirst.isNull()) {
				return sl_false;
			}
			defaultPageRatio = PdfViewContext::getBoxRatio(pageFirst->getMediaBox());
			{
				ListElements<float> items(pageRatios);
				for (sl_size i = 0; i < nPages; i++) {
					items[i] = defaultPageRatio;
				}
			}
			return sl_true;
		}

		Ref<PdfPage> getPage(sl_uint32 no)
		{
			if (no == 2) {
				no = no;
			}
			Ref<PdfPage> ret;
			if (pages.get(no, &ret)) {
				return ret;
			}
			ret = doc->getPage(no);
			pages.put(no, ret);
			return ret;
		}

		double getTotalHeight()
		{
			double h = 0;
			for (sl_uint32 i = 0; i < nPages; i++) {
				float ratio = pageRatios.getValueAt_NoLock(i);
				h += ratio;
			}
			return h;
		}

		void findFirstVisiblePage(double sy, sl_uint32& pageNo, double& pageY)
		{
			if (!nPages) {
				pageNo = 0;
				pageY = 0;
				return;
			}
			double y = 0;
			for (sl_uint32 i = 0; i < nPages - 1; i++) {
				float ratio = pageRatios.getValueAt_NoLock(i);
				if (sy + SLIB_EPSILON < y + ratio) {
					pageNo = i;
					pageY = y;
					return;
				}
				y += ratio;
			}
			pageNo = nPages - 1;
			pageY = y;
		}

		Ref<PdfPage> getPageAndGeometry(sl_uint32 no, float& ratio, sl_bool& flagUpdateRatio)
		{
			Ref<PdfPage> page = getPage(no);
			if (page.isNotNull()) {
				Rectangle mediaBox = page->getMediaBox();
				ratio = getBoxRatio(mediaBox);
				float oldRatio = pageRatios.getValueAt_NoLock(no);
				if (!(Math::isAlmostZero(oldRatio - ratio))) {
					pageRatios.setAt_NoLock(no, ratio);
					flagUpdateRatio = sl_true;
				}
				return page;
			}
			ratio = defaultPageRatio;
			return sl_null;
		}

		double getPageY(sl_uint32 no)
		{
			double y = 0;
			for (sl_uint32 i = 0; i < no; i++) {
				float ratio = pageRatios.getValueAt_NoLock(i);
				y += ratio;
			}
			return y;
		}

		float getBoxRatio(const Rectangle& box)
		{
			sl_real w = box.getWidth();
			sl_real h = box.getHeight();
			if (Math::isAlmostZero(w) || Math::isAlmostZero(h)) {
				return defaultPageRatio;
			}
			return Math::clamp((float)(h / w), MIN_PAGE_RATIO, MAX_PAGE_RATIO);
		}

		void invalidate()
		{
			pageCache.removeAll();
			pages.removeAll();
			nPages = doc->getPagesCount();
		}

	};


	SLIB_DEFINE_OBJECT(PdfView, View)

	PdfView::PdfView()
	{
		setCreatingInstance();
		setScrolling(sl_false, sl_true, UIUpdateMode::Init);
		setPageHeight(1.0, UIUpdateMode::Init);
		setCanvasScrolling(sl_false);
		setAutoHideScrollBar(sl_false);
		setFocusable();

		setUsingPageCache(sl_true, UIUpdateMode::Init);
	}

	PdfView::~PdfView()
	{
	}

	sl_bool PdfView::openFile(const StringParam& _filePath, UIUpdateMode mode)
	{
		String filePath = _filePath.toString();
		Ref<PdfViewContext> context(m_context);
		if (context.isNotNull()) {
			if (context->filePath == filePath) {
				return sl_true;
			}
		}
		Ref<PdfDocument> doc = PdfDocument::openFile(filePath);
		if (doc.isNotNull()) {
			return _setDocument(filePath, doc.get(), mode);
		}
		return sl_false;
	}

	sl_bool PdfView::openMemory(const Memory& mem, UIUpdateMode mode)
	{
		Ref<PdfDocument> doc = PdfDocument::openMemory(mem);
		if (doc.isNotNull()) {
			return _setDocument(sl_null, doc.get(), mode);
		}
		return sl_false;
	}

	void PdfView::close(UIUpdateMode mode)
	{
		m_context.setNull();
		_invalidateChanges(mode);
	}

	sl_bool PdfView::isUsingPageCache()
	{
		return m_flagUsePageCache;
	}

	void PdfView::setUsingPageCache(sl_bool flag, UIUpdateMode mode)
	{
		m_flagUsePageCache = flag;
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (flag) {
			setDoubleBuffer(sl_false);
			setOpaque(sl_true, mode != UIUpdateMode::Init ? UIUpdateMode::None : UIUpdateMode::Init);
			setBackground(sl_null, mode);
		} else {
			if (mode != UIUpdateMode::Init) {
				Ref<ScrollBar> sbar = getVerticalScrollBar();
				if (sbar.isNotNull()) {
					sbar->setBackground(sl_null, UIUpdateMode::None);
					sbar->setLayer(sl_false, UIUpdateMode::None);
				}
			}
			setDoubleBuffer(sl_true);
			setOpaque(sl_false, mode != UIUpdateMode::Init ? UIUpdateMode::None : UIUpdateMode::Init);
			setBackgroundColor(BACKGROUND_COLOR, mode);
		}
#else
		if (flag) {
			setBackground(sl_null, mode);
		} else {
			setBackgroundColor(BACKGROUND_COLOR, mode);
		}
#endif
	}

	Ref<PdfDocument> PdfView::getDocument()
	{
		Ref<PdfViewContext> context(m_context);
		if (context.isNotNull()) {
			return context->doc;
		}
		return sl_null;
	}

	sl_uint32 PdfView::getCurrentPage()
	{
		Ref<PdfViewContext> context(m_context);
		if (context.isNull()) {
			return 0;
		}
		ObjectLocker lock(context.get());
		sl_uint32 pageNo;
		double pageY;
		context->findFirstVisiblePage(getScrollY(), pageNo, pageY);
		return pageNo;
	}

	void PdfView::goToPage(sl_uint32 pageNo, UIUpdateMode mode)
	{
		Ref<PdfViewContext> context(m_context);
		if (context.isNull()) {
			return;
		}
		ObjectLocker lock(context.get());
		double y = context->getPageY(pageNo);
		scrollToY(y, mode);
	}

	sl_bool PdfView::deletePage(sl_uint32 pageNo, UIUpdateMode mode)
	{
		Ref<PdfViewContext> context(m_context);
		if (context.isNull()) {
			return sl_false;
		}
		ObjectLocker lock(context.get());
		if (context->nPages < 2) {
			return sl_false;
		}
		if (!(context->doc->deletePage(pageNo))) {
			return sl_false;
		}
		context->pageRatios.removeAt_NoLock(pageNo);
		context->invalidate();
		_invalidateChanges(mode);
		return sl_true;
	}

	sl_bool PdfView::_setDocument(const String& filePath, PdfDocument* doc, UIUpdateMode mode)
	{
		sl_uint32 nPages = doc->getPagesCount();
		if (!nPages) {
			return sl_false;
		}

		Ref<PdfViewContext> context(new PdfViewContext);
		if (context.isNull()) {
			return sl_false;
		}
		if (!(context->initialize(filePath, doc, nPages))) {
			return sl_false;
		}
		m_context = Move(context);

		setScrollY(0, UIUpdateMode::None);
		_invalidateChanges(mode);
		
		return sl_true;
	}

	void PdfView::_invalidateChanges(UIUpdateMode mode)
	{
		Ref<PdfViewContext> context(m_context);
		if (context.isNotNull()) {
			setContentHeight(context->getTotalHeight(), UIUpdateMode::None);
		} else {
			setContentHeight(0, UIUpdateMode::None);
		}
		invalidate(mode);
	}

	Ref<Bitmap> PdfView::_saveCache(PdfViewContext* context, sl_uint32 pageNo, sl_int32 iWidth, sl_int32 iHeight)
	{
		Ref<PdfPage> page = context->getPage(pageNo);
		if (page.isNull()) {
			return sl_null;
		}
		Ref<Bitmap> bitmap = Bitmap::create(iWidth, iHeight);
		if (bitmap.isNull()) {
			return sl_null;
		}
		bitmap->resetPixels(BACKGROUND_COLOR);
		{
			Ref<Canvas> canvas = bitmap->getCanvas();
			if (canvas.isNull()) {
				return sl_null;
			}
			PdfRenderParam param;
			param.canvas = canvas.get();
			param.cache = context;
			param.bounds.left = 0;
			param.bounds.top = 0;
			param.bounds.right = (sl_real)iWidth;
			param.bounds.bottom = (sl_real)iHeight;
			page->render(param);
			if (pageNo) {
				canvas->fillRectangle(0, 0, (sl_real)iWidth, 2.0f, BORDER_COLOR);
			}
		}
		if (context->pageCache.getCount() > 6) {
			context->pageCache.removeAll();
		}
		context->pageCache.put(pageNo, bitmap);
		return bitmap;
	}

	void PdfView::onDraw(Canvas* canvas)
	{
		UIRect bounds = getBounds();
		sl_int32 iWidth = bounds.getWidth();
		sl_int32 iHeight = bounds.getHeight();
		if (iWidth <= 0 || iHeight <= 0) {
			return;
		}

		Ref<PdfViewContext> context(m_context);
		if (context.isNull()) {
			if (m_flagUsePageCache) {
				canvas->fillRectangle(bounds, BACKGROUND_COLOR);
			}
			return;
		}
		ObjectLocker lock(context.get());

#if defined(SLIB_PLATFORM_IS_WIN32)
		if (m_flagUsePageCache) {
			Ref<ScrollBar> sbar = getVerticalScrollBar();
			if (sbar.isNotNull()) {
				iWidth -= sbar->getWidth();
				if (sbar->getBackground().isNull()) {
					sbar->setBackgroundColor(Color::White, UIUpdateMode::None);
					sbar->setLayer(sl_true, UIUpdateMode::None);
				}
				if (!(isVerticalScrollBarVisible())) {
					canvas->fillRectangle((sl_real)iWidth, 0.0f, (sl_real)(sbar->getWidth()), (sl_real)iHeight, BACKGROUND_COLOR);
				}
			}
		}
#endif

		sl_real width = (sl_real)iWidth;
		if (Math::isAlmostZero(width)) {
			return;
		}
		double sy = (double)(getScrollY());

		{
			sl_bool flagUpdateScrollRange = sl_false;
			sl_uint32 pageFirst = 0;
			double pageFirstY = 0;
			context->findFirstVisiblePage(sy, pageFirst, pageFirstY);

			sl_int32 bottomPage = (sl_int32)((pageFirstY - sy) * width);
			for (sl_uint32 i = 0; i < 100; i++) {
				if (bottomPage >= iHeight) {
					break;
				}
				sl_uint32 pageNo = pageFirst + i;
				float ratio;
				Ref<PdfPage> page = context->getPageAndGeometry(pageNo, ratio, flagUpdateScrollRange);
				if (page.isNull()) {
					break;
				}
				sl_int32 topPage = bottomPage;
				sl_int32 iHeighPage = (sl_int32)(ratio * width);
				if (iHeighPage < 1) {
					iHeighPage = 1;
				}
				bottomPage += iHeighPage;
				if (bottomPage < 0) {
					continue;
				}
				if (m_flagUsePageCache) {
					Ref<Bitmap> cache;
					if (context->pageCache.get(pageNo, &cache)) {
						if (cache->getWidth() != (sl_uint32)iWidth && cache->getHeight() != (sl_uint32)iHeighPage) {
							cache.setNull();
						}
					}
					if (cache.isNull()) {
						cache = _saveCache(context.get(), pageNo, iWidth, iHeighPage);
					}
					sl_int32 dy1 = topPage;
					sl_int32 dy2 = bottomPage;
					if (dy1 < 0) {
						dy1 = 0;
					}
					if (dy2 > iHeight) {
						dy2 = iHeight;
					}
					if (dy2 > dy1) {
						if (cache.isNotNull()) {
							sl_bool flagAntialias = canvas->isAntiAlias();
							canvas->setAntiAlias(sl_false);
							Rectangle rcSrc(0, (sl_real)(dy1 - topPage), width, (sl_real)(dy2 - topPage));
							canvas->draw(Rectangle(0, (sl_real)dy1, width, (sl_real)dy2), cache, rcSrc);
							canvas->setAntiAlias(flagAntialias);
						} else {
							canvas->fillRectangle(Rectangle(0, (sl_real)dy1, width, (sl_real)dy2), BACKGROUND_COLOR);
						}
					}
				} else {
					PdfRenderParam param;
					param.canvas = canvas;
					param.cache = m_context;
					param.bounds.left = 0;
					param.bounds.right = width;
					param.bounds.top = (sl_real)topPage;
					param.bounds.bottom = (sl_real)bottomPage;
					page->render(param);
					if (i) {
						canvas->fillRectangle(0, (sl_real)topPage, width, 2, BORDER_COLOR);
					}
				}
			}

			if (flagUpdateScrollRange) {
				setContentHeight(context->getTotalHeight(), UIUpdateMode::Redraw);
			}

			if (m_flagUsePageCache) {
				if (bottomPage < iHeight) {
					canvas->fillRectangle(0, (sl_real)bottomPage, width, (sl_real)(iHeight - bottomPage), BACKGROUND_COLOR);
				}
			}
		}
	}

	void PdfView::onResize(sl_ui_len width, sl_ui_len height)
	{
		if (width && height) {
			setPageHeight((sl_scroll_pos)height / (sl_scroll_pos)width);
		} else {
			setPageHeight(1.0);
		}
	}

}
