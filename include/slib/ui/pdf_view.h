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

#ifndef CHECKHEADER_SLIB_UI_PDF_VIEW
#define CHECKHEADER_SLIB_UI_PDF_VIEW

#include "view.h"

#include "../core/string.h"

namespace slib
{

	class PdfDocument;
	class PdfViewContext;

	class SLIB_EXPORT PdfView : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfView();

		~PdfView();

	public:
		sl_bool openFile(const StringParam& filePath);

		sl_bool openMemory(const Memory& mem);

		void close();

		Ref<PdfDocument> getDocument();

		void goToPage(sl_uint32 pageNo, UIUpdateMode mode = UIUpdateMode::Redraw);
		
	protected:
		void onDraw(Canvas* canvas) override;

		void onResize(sl_ui_len width, sl_ui_len height) override;

	protected:
		void _setDocument(const String& path, PdfDocument* doc);

	protected:
		AtomicRef<PdfViewContext> m_context;
		sl_ui_len m_widthOld;

	};

}

#endif