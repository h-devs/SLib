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
#include "slib/core/dispatch_loop.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(PdfView, View)

	PdfView::PdfView()
	{
	}

	PdfView::~PdfView()
	{
		if (m_loop.isNotNull()) {
			m_loop->release();
		}
	}

	void PdfView::openFile(const StringParam& _filePath)
	{
		String filePath = _filePath.toString();
		ObjectLocker lock(this);
		if (m_filePath == filePath) {
			return;
		}
		close();
		if (m_loop.isNull()) {
			m_loop = DispatchLoop::create();
			if (m_loop.isNull()) {
				return;
			}
		}
		m_filePath = filePath;
		if (filePath.isNotEmpty()) {
			m_loop->dispatch(SLIB_BIND_WEAKREF(void(), this, _openFile, filePath));
		}
	}

	void PdfView::close()
	{
		ObjectLocker lock(this);
		m_filePath.setNull();
		m_doc.setNull();
	}

	void PdfView::onDraw(Canvas* canvas)
	{
	}

	void PdfView::_openFile(const String& filePath)
	{
		Shared<PdfDocument> doc = Shared<PdfDocument>::create();
		if (doc.isNull()) {
			close();
			return;
		}
		if (!(doc->openFile(filePath))) {
			close();
			return;
		}
		m_doc = doc;
	}

}
