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

#ifndef CHECKHEADER_SLIB_CORE_WIN32_COM
#define CHECKHEADER_SLIB_CORE_WIN32_COM

#include "definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "windows.h"

#include "object.h"
#include "memory.h"
#include "string.h"
#include "list.h"

namespace slib
{

	class Win32_COM
	{
	public:
		static Memory readAllBytesFromStream(IStream* stream);

	};

	class GenericDataObject : public IDataObject
	{
	public:
		GenericDataObject();

		~GenericDataObject();

	public:
		ULONG STDMETHODCALLTYPE AddRef() override;

		ULONG STDMETHODCALLTYPE Release() override;

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

		HRESULT STDMETHODCALLTYPE GetData(FORMATETC *pFormatEtcIn, STGMEDIUM *pMedium) override;

		HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium) override;

		HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *pFormatEtc) override;

		HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *pFormatEctIn, FORMATETC *pFormatEtcOut) override;

		HRESULT STDMETHODCALLTYPE SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease) override;

		HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc) override;

		HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) override;

		HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) override;

		HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA **ppenumAdvise) override;

	public:
		void setText(const StringParam& text);

	protected:
		ULONG m_nRef;
#ifdef PRIV_SLIB_IMPLEMENT_WIN32_COM
		List< Ref<priv::win32_com::GenericDataElement> > m_mediums;
#else
		List< Ref<Referable> > m_mediums;
#endif

	};


}

#define SLIB_WIN32_COM_SAFE_RELEASE(x) {if (x) {x->Release(); x=NULL;}}

#endif

#endif
