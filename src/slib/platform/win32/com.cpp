/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/platform/definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#define PRIV_SLIB_IMPLEMENT_WIN32_COM

namespace slib
{
	namespace priv
	{
		namespace win32_com
		{
			class GenericDataElement;
		}
	}
}

#include "slib/platform/win32/com.h"

namespace slib
{

	namespace priv
	{
		namespace win32_com
		{

			class GenericDataElement : public Referable
			{
			public:
				FORMATETC format;
				STGMEDIUM medium;

			public:
				GenericDataElement(const FORMATETC& _format, const STGMEDIUM& _medium) : format(_format), medium(_medium)
				{
				}

				~GenericDataElement()
				{
					ReleaseStgMedium(&medium);
				}

			};

			class CEnumFormatEtc : public IEnumFORMATETC
			{
			private:
				ULONG m_nRef;

				List< Ref<GenericDataElement> > m_list;
				ULONG m_index;

			public:
				CEnumFormatEtc(const List< Ref<GenericDataElement> >& list) : m_list(list)
				{
					m_nRef = 0;
					m_index = 0;
				}

			public:
				ULONG STDMETHODCALLTYPE AddRef() override
				{
					return ++m_nRef;
				}

				ULONG STDMETHODCALLTYPE Release() override
				{
					ULONG nRef = --m_nRef;
					if (!nRef) {
						delete this;
					}
					return nRef;
				}

				HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
				{
					if (!ppvObject) {
						return E_POINTER;
					}
					if (riid == __uuidof(IEnumFORMATETC) || riid == __uuidof(IUnknown)) {
						*ppvObject = this;
						AddRef();
						return S_OK;
					}
					return E_NOINTERFACE;
				}

				HRESULT STDMETHODCALLTYPE Next(ULONG celt, FORMATETC *lpFormatEtc, ULONG *pCeltFetched) override
				{
					if (pCeltFetched) {
						*pCeltFetched = 0;
					}

					if (!celt || !lpFormatEtc || m_index + celt >= m_list.getCount()) {
						return S_FALSE;
					}
					if (!pCeltFetched && celt != 1) {
						// pCeltFetched can be NULL only for 1 item request
						return S_FALSE;
					}

					for (ULONG i = 0; i < celt; i++) {
						*lpFormatEtc = m_list[m_index]->format;
						m_index++;
						lpFormatEtc++;
					}
					if (pCeltFetched) {
						*pCeltFetched = celt;
					}

					return S_OK;
				}

				HRESULT STDMETHODCALLTYPE Skip(ULONG celt) override
				{
					if (m_index + celt >= m_list.getCount()) {
						return S_FALSE;
					}
					m_index += celt;
					return S_OK;
				}

				HRESULT STDMETHODCALLTYPE Reset() override
				{
					m_index = 0;
					return S_OK;
				}

				HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC **ppEnum) override
				{
					if (!ppEnum) {
						return E_POINTER;
					}
					CEnumFormatEtc* newEnum = new CEnumFormatEtc(m_list);
					if (!newEnum) {
						return E_OUTOFMEMORY;
					}
					newEnum->AddRef();
					newEnum->m_index = m_index;
					*ppEnum = newEnum;
					return S_OK;
				}

			};

			static void CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc)
			{
				switch (pMedSrc->tymed) {
				case TYMED_HGLOBAL:
					pMedDest->hGlobal = (HGLOBAL)(OleDuplicateData(pMedSrc->hGlobal, pFmtSrc->cfFormat, 0));
					break;
				case TYMED_GDI:
					pMedDest->hBitmap = (HBITMAP)(OleDuplicateData(pMedSrc->hBitmap, pFmtSrc->cfFormat, 0));
					break;
				case TYMED_MFPICT:
					pMedDest->hMetaFilePict = (HMETAFILEPICT)(OleDuplicateData(pMedSrc->hMetaFilePict, pFmtSrc->cfFormat, 0));
					break;
				case TYMED_ENHMF:
					pMedDest->hEnhMetaFile = (HENHMETAFILE)(OleDuplicateData(pMedSrc->hEnhMetaFile, pFmtSrc->cfFormat, 0));
					break;
				case TYMED_FILE:
					pMedSrc->lpszFileName = (LPOLESTR)(OleDuplicateData(pMedSrc->lpszFileName, pFmtSrc->cfFormat, 0));
					break;
				case TYMED_ISTREAM:
					pMedDest->pstm = pMedSrc->pstm;
					pMedSrc->pstm->AddRef();
					break;
				case TYMED_ISTORAGE:
					pMedDest->pstg = pMedSrc->pstg;
					pMedSrc->pstg->AddRef();
					break;
				case TYMED_NULL:
				default:
					break;
				}
				pMedDest->tymed = pMedSrc->tymed;
				pMedDest->pUnkForRelease = NULL;
				if (pMedSrc->pUnkForRelease != NULL) {
					pMedDest->pUnkForRelease = pMedSrc->pUnkForRelease;
					pMedSrc->pUnkForRelease->AddRef();
				}
			}

		}
	}

	using namespace priv::win32_com;

	namespace win32
	{

		Memory COM::readAllBytesFromStream(IStream* stream)
		{
			Memory ret;
			HRESULT hr;

			STATSTG stat;
			Base::zeroMemory(&stat, sizeof(stat));
			hr = stream->Stat(&stat, STATFLAG_NONAME);
			if (hr == S_OK) {
				LARGE_INTEGER n;
				n.QuadPart = 0;
				stream->Seek(n, STREAM_SEEK_SET, NULL);
				ULONG size = (ULONG)(stat.cbSize.QuadPart);
				if (size > 0) {
					ret = Memory::create(size);
					if (ret.isNotNull()) {
						ULONG nRead = 0;
						hr = stream->Read(ret.getData(), size, &nRead);
						if (hr == S_OK && nRead == size) {
							return ret;
						}
					}
					ret.setNull();
				}
			}
			return ret;
		}


		GenericDataObject::GenericDataObject()
		{
			m_nRef = 0;
		}

		GenericDataObject::~GenericDataObject()
		{
		}

		ULONG STDMETHODCALLTYPE GenericDataObject::AddRef()
		{
			return ++m_nRef;
		}

		ULONG STDMETHODCALLTYPE GenericDataObject::Release()
		{
			ULONG nRef = --m_nRef;
			if (!nRef) {
				delete this;
			}
			return nRef;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::QueryInterface(REFIID riid, void **ppvObject)
		{
			if (!ppvObject) {
				return E_POINTER;
			}
			if (riid == __uuidof(IDataObject) || riid == __uuidof(IUnknown)) {
				*ppvObject = this;
				AddRef();
				return S_OK;
			}
			return E_NOINTERFACE;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::GetData(FORMATETC *pFormatEtcIn, STGMEDIUM *pMedium)
		{
			if (!pFormatEtcIn || !pMedium) {
				return E_INVALIDARG;
			}
			pMedium->hGlobal = NULL;
			ListElements< Ref<GenericDataElement> > list(m_mediums);
			for (sl_size i = 0; i < list.count; i++) {
				if ((pFormatEtcIn->tymed & list[i]->format.tymed) &&
					pFormatEtcIn->dwAspect == list[i]->format.dwAspect &&
					pFormatEtcIn->cfFormat == list[i]->format.cfFormat
					) {
					CopyMedium(pMedium, &(list[i]->medium), &(list[i]->format));
					return S_OK;
				}
			}
			return DV_E_FORMATETC;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
		{
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::QueryGetData(FORMATETC *pFormatEtc)
		{
			if (!pFormatEtc) {
				return E_INVALIDARG;
			}
			ListElements< Ref<GenericDataElement> > list(m_mediums);
			HRESULT hr = E_UNEXPECTED;
			for (sl_size i = 0; i < list.count; i++) {
				if (pFormatEtc->dwAspect & list[i]->format.dwAspect) {
					if (pFormatEtc->tymed & list[i]->format.tymed) {
						if (pFormatEtc->cfFormat == list[i]->format.cfFormat) {
							return S_OK;
						} else {
							hr = DV_E_CLIPFORMAT;
						}
					} else {
						hr = DV_E_TYMED;
					}
				} else {
					hr = DV_E_DVASPECT;
				}
			}
			return hr;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::GetCanonicalFormatEtc(FORMATETC *pFormatEctIn, FORMATETC *pFormatEtcOut)
		{
			return DATA_S_SAMEFORMATETC;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease)
		{
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
		{
			if (!ppEnumFormatEtc) {
				return E_POINTER;
			}

			*ppEnumFormatEtc = NULL;
			switch (dwDirection) {
			case DATADIR_GET:
				*ppEnumFormatEtc = new CEnumFormatEtc(m_mediums);
				if (!(*ppEnumFormatEtc)) {
					return E_OUTOFMEMORY;
				}
				(*ppEnumFormatEtc)->AddRef();
				break;
			case DATADIR_SET:
			default:
				return E_NOTIMPL;
			}
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
		{
			return OLE_E_ADVISENOTSUPPORTED;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::DUnadvise(DWORD dwConnection)
		{
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE GenericDataObject::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
		{
			return OLE_E_ADVISENOTSUPPORTED;
		}

		void GenericDataObject::setText(const StringParam& _text)
		{
			StringData16 text(_text);

			FORMATETC fmt = { 0 };
			fmt.cfFormat = CF_UNICODETEXT;
			fmt.dwAspect = DVASPECT_CONTENT;
			fmt.lindex = -1;
			fmt.tymed = TYMED_HGLOBAL;

			STGMEDIUM medium = { 0 };
			medium.tymed = TYMED_HGLOBAL;

			sl_size len = text.getLength();
			sl_size size = (len + 1) << 1;
			medium.hGlobal = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)size);
			sl_char16* pMem = (sl_char16*)GlobalLock(medium.hGlobal);
			Base::copyMemory(pMem, text.getData(), size);
			pMem[len] = 0;
			GlobalUnlock(medium.hGlobal);

			Ref<GenericDataElement> element = new GenericDataElement(fmt, medium);
			if (element.isNotNull()) {
				m_mediums.add_NoLock(Move(element));
			}
		}

	}

}

#endif
