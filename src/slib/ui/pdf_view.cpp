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

namespace slib
{

	SLIB_DEFINE_OBJECT(PdfView, View)

	PdfView::PdfView()
	{
		m_pageNo = 0;
	}

	PdfView::~PdfView()
	{
	}

	sl_bool PdfView::openFile(const StringParam& _filePath)
	{
		String filePath = _filePath.toString();
		ObjectLocker lock(this);
		if (m_filePath == filePath) {
			return sl_true;
		}
		Ref<PdfDocument> doc = PdfDocument::openFile(filePath);
		if (doc.isNotNull()) {
			m_filePath = filePath;
			_setDocument(doc.get());
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PdfView::openMemory(const Memory& mem)
	{
		ObjectLocker lock(this);
		Ref<PdfDocument> doc = PdfDocument::openMemory(mem);
		if (doc.isNotNull()) {
			_setDocument(doc.get());
			return sl_true;
		}
		return sl_false;
	}

	void PdfView::close()
	{
		ObjectLocker lock(this);
		m_filePath.setNull();
		m_doc.setNull();
	}

	Ref<PdfDocument> PdfView::getDocument()
	{
		return m_doc;
	}

	void PdfView::_setDocument(PdfDocument* doc)
	{
		m_doc = doc;
		m_pages.removeAll();
		invalidate();
	}

	Ref<PdfPage> PdfView::_getPage(sl_uint32 no)
	{
		Ref<PdfPage> ret;
		if (m_pages.get(no, &ret)) {
			return ret;
		}
		Ref<PdfDocument> doc = m_doc;
		if (doc.isNotNull()) {
			ret = doc->getPage(no);
			m_pages.put(no, ret);
			return ret;
		}
		return sl_null;
	}

	void PdfView::onDraw(Canvas* canvas)
	{
		Ref<PdfPage> page = _getPage(m_pageNo);
		if (page.isNotNull()) {
			page->render(canvas, getBoundsInnerPadding());
		}
	}

}
