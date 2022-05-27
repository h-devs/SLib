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

#define EXPIRE_DURATION_PDF_RESOURCES 5000
#define EXPIRE_DURATION_PAGE_CACHE 3000

namespace slib
{

	class PdfViewContext : public PdfRenderContext
	{
	public:
		Ref<PdfDocument> doc;
		String filePath;
		sl_uint32 nPages = 1;
		ExpiringMap< sl_uint32, Ref<PdfPage> > pages;
		ExpiringMap< sl_uint32, Ref<Bitmap> > pageCache;
		float defaultPageRatio = 1.0f;
		CHashMap<sl_uint32, float> mapPageRatio;

	public:
		PdfViewContext()
		{
			pageCache.setExpiringMilliseconds(EXPIRE_DURATION_PAGE_CACHE);
			pages.setExpiringMilliseconds(EXPIRE_DURATION_PDF_RESOURCES);
			fonts.setExpiringMilliseconds(EXPIRE_DURATION_PDF_RESOURCES);
			images.setExpiringMilliseconds(EXPIRE_DURATION_PDF_RESOURCES);
		}

	public:
		void initialize(const String& _filePath, PdfDocument* _doc, sl_uint32 _nPages)
		{
			filePath = _filePath;
			doc = _doc;
			nPages = _nPages;
			Ref<PdfPage> page = getPage(0);
			if (page.isNotNull()) {
				defaultPageRatio = PdfViewContext::getBoxRatio(page->getMediaBox());
				mapPageRatio.put(0, defaultPageRatio);
			}
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

		float getTotalHeight_NoLock()
		{
			float h = 0;
			for (sl_uint32 i = 0; i < nPages; i++) {
				float ratio = mapPageRatio.getValue_NoLock(i, defaultPageRatio);
				h += ratio;
			}
			return h;
		}

		float getTotalHeight()
		{
			ObjectLocker lock(&mapPageRatio);
			return getTotalHeight_NoLock();
		}

		void findFirstVisiblePage_NoLock(float sy, sl_uint32& pageNo, float& pageY)
		{
			float y = 0;
			for (sl_uint32 i = 0; i < nPages - 1; i++) {
				float ratio = mapPageRatio.getValue_NoLock(i, defaultPageRatio);
				if (sy <= y + ratio) {
					pageNo = i;
					pageY = y;
					return;
				}
				y += ratio;
			}
			pageNo = nPages - 1;
			pageY = y;
		}

		Ref<PdfPage> getPageAndGeometry_NoLock(sl_uint32 no, float& ratio, sl_bool& flagUpdateRatio)
		{
			Ref<PdfPage> page = getPage(no);
			if (page.isNotNull()) {
				Rectangle mediaBox = page->getMediaBox();
				ratio = getBoxRatio(mediaBox);
				float oldRatio = mapPageRatio.getValue_NoLock(no, defaultPageRatio);
				if (Math::isAlmostZero(oldRatio - ratio)) {
					return page;
				}
				mapPageRatio.put_NoLock(no, ratio);
				flagUpdateRatio = sl_true;
				return page;
			}
			ratio = defaultPageRatio;
			return sl_null;
		}

		float getPageY(sl_uint32 no)
		{
			ObjectLocker lock(&mapPageRatio);
			float y = 0;
			for (sl_uint32 i = 0; i < no; i++) {
				float ratio = mapPageRatio.getValue_NoLock(i, defaultPageRatio);
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
			return (float)(h / w);
		}

	};


	SLIB_DEFINE_OBJECT(PdfView, View)

	PdfView::PdfView()
	{
		m_widthOld = 0;

		setVerticalScrolling(sl_true, UIUpdateMode::Init);
		setFocusable();
	}

	PdfView::~PdfView()
	{
	}

	sl_bool PdfView::openFile(const StringParam& _filePath)
	{
		String filePath = _filePath.toString();
		Ref<PdfViewContext> context = m_context;
		if (context.isNotNull()) {
			if (context->filePath == filePath) {
				return sl_true;
			}
		}
		Ref<PdfDocument> doc = PdfDocument::openFile(filePath);
		if (doc.isNotNull()) {
			_setDocument(filePath, doc.get());
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PdfView::openMemory(const Memory& mem)
	{
		Ref<PdfDocument> doc = PdfDocument::openMemory(mem);
		if (doc.isNotNull()) {
			_setDocument(sl_null, doc.get());
			return sl_true;
		}
		return sl_false;
	}

	void PdfView::close()
	{
		m_context.setNull();
		invalidate();
	}

	Ref<PdfDocument> PdfView::getDocument()
	{
		Ref<PdfViewContext> context = m_context;
		if (context.isNotNull()) {
			return context->doc;
		}
		return sl_null;
	}

	void PdfView::goToPage(sl_uint32 pageNo, UIUpdateMode mode)
	{
		Ref<PdfViewContext> context = m_context;
		if (context.isNull()) {
			return;
		}
		sl_real width = (sl_real)(getWidth());
		if (Math::isAlmostZero(width)) {
			return;
		}
		float y = context->getPageY(pageNo) * width;
		scrollToY(y, mode);
	}

	void PdfView::_setDocument(const String& filePath, PdfDocument* doc)
	{
		sl_uint32 nPages = doc->getPagesCount();
		if (!nPages) {
			return;
		}
		Ref<PdfViewContext> context = new PdfViewContext;
		if (context.isNull()) {
			return;
		}
		context->initialize(filePath, doc, nPages);
		m_context = context;

		setScrollY(0, UIUpdateMode::None);
		setContentHeight(context->getTotalHeight() * (float)(getWidth()), UIUpdateMode::None);
		invalidate();
	}

	void PdfView::_saveCache(PdfViewContext* context, sl_uint32 pageNo)
	{
		if (m_context != context) {
			return;
		}
		Ref<PdfPage> page = context->getPage(pageNo);
		if (page.isNull()) {
			return;
		}
		sl_real width = (sl_real)(getWidth());
		if (width < 100) {
			width = 100;
		}
		if (width > 1000) {
			width = 1000;
		}
		Rectangle mediaBox = page->getMediaBox();
		sl_real ratio = (sl_real)(context->getBoxRatio(mediaBox));
		sl_real height = width * ratio;
		Ref<Bitmap> bitmap = Bitmap::create((sl_uint32)width, (sl_uint32)height);
		if (bitmap.isNull()) {
			return;
		}
		{
			Ref<Canvas> canvas = bitmap->getCanvas();
			if (canvas.isNull()) {
				return;
			}
			PdfRenderParam param;
			param.canvas = canvas.get();
			param.context = context;
			param.bounds.left = 0;
			param.bounds.top = 0;
			param.bounds.right = width;
			param.bounds.bottom = height;
			page->render(param);
		}
		context->pageCache.put(pageNo, bitmap);
	}

	void PdfView::onDraw(Canvas* canvas)
	{
		Ref<PdfViewContext> context = m_context;
		if (context.isNull()) {
			return;
		}

		UIRect bounds = getBounds();
		sl_real height = (sl_real)(bounds.getHeight());
		sl_real width = (sl_real)(bounds.getWidth());
		if (Math::isAlmostZero(width)) {
			return;
		}
		float sy = (float)(getScrollY());

		{
			ObjectLocker lockMap(&(context->mapPageRatio));
			sl_bool flagUpdateScrollRange = sl_false;
			sl_uint32 pageFirst = 0;
			float pageFirstY = 0;
			context->findFirstVisiblePage_NoLock(sy / width, pageFirst, pageFirstY);

			Ref<Pen> penBorder = Pen::createSolidPen(2, Color::Gray);

			PdfRenderParam param;
			param.canvas = canvas;
			param.context = m_context;
			param.bounds.left = 0;
			param.bounds.right = width;
			param.bounds.bottom = pageFirstY * width;

			for (sl_uint32 i = 0; i < 100; i++) {
				sl_uint32 pageNo = pageFirst + i;
				float ratio;
				Ref<PdfPage> page = context->getPageAndGeometry_NoLock(pageNo, ratio, flagUpdateScrollRange);
				if (page.isNull()) {
					break;
				}
				param.bounds.top = param.bounds.bottom;
				param.bounds.bottom += ratio * width;
				if (param.bounds.top >= sy + height) {
					break;
				}
				if (sy < param.bounds.bottom) {
					Ref<Bitmap> cache;
					if (context->pageCache.get(pageNo, &cache)) {
						if (cache.isNotNull()) {
							sl_bool flagAntialias = canvas->isAntiAlias();
							canvas->setAntiAlias(sl_true);
							canvas->draw(param.bounds, cache);
							canvas->setAntiAlias(flagAntialias);
						}
					} else {
						TimeCounter tc;
						page->render(param);
						sl_uint64 dt = tc.getElapsedMilliseconds();
						if (dt > 100) {
							WeakRef<PdfView> weak = this;
							Dispatch::dispatch([context, pageNo, weak, this]() {
								Ref<PdfView> thiz = weak;
								if (thiz.isNotNull()) {
									_saveCache(context.get(), pageNo);
								}
							});
						}
					}
					if (i) {
						canvas->drawLine(param.bounds.left, param.bounds.top - 1, param.bounds.right, param.bounds.top - 1, penBorder);
					}
				}
			}

			if (flagUpdateScrollRange) {
				setContentHeight(context->getTotalHeight_NoLock() * width, UIUpdateMode::Redraw);
			}
		}
	}

	void PdfView::onResize(sl_ui_len width, sl_ui_len height)
	{
		Ref<PdfViewContext> context = m_context;
		if (context.isNotNull()) {
			setContentHeight(context->getTotalHeight() * (float)width, UIUpdateMode::None);
			if (m_widthOld) {
				setScrollY(getScrollY() * (sl_scroll_pos)width / (sl_scroll_pos)m_widthOld);
			}
		}
		m_widthOld = width;
	}

}
