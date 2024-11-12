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

#include "slib/graphics/definition.h"

#if defined(SLIB_GRAPHICS_IS_GDI)

#include "slib/graphics/font.h"

#include "slib/io/memory_reader.h"
#include "slib/core/hash_map.h"
#include "slib/core/scoped_buffer.h"
#include "slib/core/safe_static.h"
#include "slib/core/shared.h"
#include "slib/graphics/platform.h"

namespace slib
{

	namespace
	{
		class FontStaticContext
		{
		public:
			CHashMap< String, SharedPtr<Gdiplus::PrivateFontCollection> > fontCollections;

		public:
			FontStaticContext()
			{
				GraphicsPlatform::startGdiplus();
			}

		public:
			void addCollection(const SharedPtr<Gdiplus::PrivateFontCollection>& collection)
			{
				INT n = collection->GetFamilyCount();
				Gdiplus::FontFamily* families = new Gdiplus::FontFamily[n];
				if (families) {
					if (collection->GetFamilies(n, families, &n) == Gdiplus::Ok) {
						for (INT i = 0; i < n; i++) {
							WCHAR name[LF_FACESIZE];
							if (families[i].GetFamilyName(name) == Gdiplus::Ok) {
								fontCollections.add(String::from(name), collection);
							}
						}
					}
					delete[] families;
				}
			}
		};

		SLIB_SAFE_STATIC_GETTER(FontStaticContext, GetFontStaticContext)

		class FontPlatformObject : public CRef
		{
		public:
			Gdiplus::Font* m_fontGdiplus;
			sl_bool m_flagCreatedGdiplus;

			HFONT m_fontGDI;
			sl_bool m_flagCreatedGDI;

			SpinLock m_lock;

		public:
			FontPlatformObject()
			{
				m_fontGdiplus = sl_null;
				m_flagCreatedGdiplus = sl_false;

				m_fontGDI = NULL;
				m_flagCreatedGDI = sl_false;
			}

			~FontPlatformObject()
			{
				if (m_fontGdiplus) {
					delete m_fontGdiplus;
				}
				if (m_fontGDI) {
					DeleteObject(m_fontGDI);
				}
			}

		public:
			void _createGdiplus(const FontDesc& desc)
			{
				FontStaticContext* context = GetFontStaticContext();
				if (!context) {
					return;
				}
				if (m_flagCreatedGdiplus) {
					return;
				}
				SpinLocker lock(&m_lock);
				if (m_flagCreatedGdiplus) {
					return;
				}
				m_flagCreatedGdiplus = sl_true;

				int style = 0;
				if (desc.flagBold) {
					style |= Gdiplus::FontStyleBold;
				}
				if (desc.flagItalic) {
					style |= Gdiplus::FontStyleItalic;
				}
				if (desc.flagUnderline) {
					style |= Gdiplus::FontStyleUnderline;
				}
				if (desc.flagStrikeout) {
					style |= Gdiplus::FontStyleStrikeout;
				}

				StringCstr16 fontName(desc.familyName);
				SharedPtr<Gdiplus::PrivateFontCollection> collection = context->fontCollections.getValue(desc.familyName);

				m_fontGdiplus = new Gdiplus::Font(
					(LPCWSTR)(fontName.getData()),
					desc.size,
					style,
					Gdiplus::UnitPixel,
					collection.get()
				);
			}

			void _createGDI(const FontDesc& desc)
			{
				if (m_flagCreatedGDI) {
					return;
				}
				SpinLocker lock(&m_lock);
				if (m_flagCreatedGDI) {
					return;
				}
				m_flagCreatedGDI = sl_true;

				int height = -(int)(desc.size);
				int weight;
				if (desc.flagBold) {
					weight = 700;
				} else {
					weight = 400;
				}
				DWORD bItalic;
				if (desc.flagItalic) {
					bItalic = TRUE;
				} else {
					bItalic = FALSE;
				}
				DWORD bUnderline;
				if (desc.flagUnderline) {
					bUnderline = TRUE;
				} else {
					bUnderline = FALSE;
				}
				DWORD bStrikeout;
				if (desc.flagStrikeout) {
					bStrikeout = TRUE;
				} else {
					bStrikeout = FALSE;
				}
				StringCstr16 fontName = desc.familyName;
				HFONT hFont = CreateFontW(height, 0, 0, 0, weight, bItalic, bUnderline, bStrikeout,
					DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					ANTIALIASED_QUALITY,
					DEFAULT_PITCH,
					(LPCWSTR)(fontName.getData()));
				m_fontGDI = hFont;
			}
		};

		class FontHelper : public Font
		{
		public:
			FontPlatformObject* getPlatformObject()
			{
				if (m_platformObject.isNull()) {
					SpinLocker lock(&m_lock);
					if (m_platformObject.isNull()) {
						m_platformObject = new FontPlatformObject;
					}
				}
				return (FontPlatformObject*)(m_platformObject.get());;
			}

			Gdiplus::Font* getGdiplus()
			{
				FontPlatformObject* po = getPlatformObject();
				if (po) {
					po->_createGdiplus(m_desc);
					return po->m_fontGdiplus;
				}
				return sl_null;
			}

			HFONT getGDI()
			{
				FontPlatformObject* po = getPlatformObject();
				if (po) {
					po->_createGDI(m_desc);
					return po->m_fontGDI;
				}
				return sl_null;
			}
		};
	}

	sl_bool Font::_getFontMetrics_PO(FontMetrics& _out)
	{
		Gdiplus::Font* handle = GraphicsPlatform::getGdiplusFont(this);
		if (handle) {
			sl_uint8 bufFamily[sizeof(Gdiplus::FontFamily)] = { 0 };
			Gdiplus::FontFamily& family = *((Gdiplus::FontFamily*)bufFamily);
			if (handle->GetFamily(&family) == Gdiplus::Ok) {
				int style = handle->GetStyle();
				sl_real ratio = (sl_real)(handle->GetSize()) / (sl_real)(family.GetEmHeight(style));
				_out.ascent = (sl_real)(family.GetCellAscent(style)) * ratio;
				_out.descent = (sl_real)(family.GetCellDescent(style)) * ratio;
				_out.leading = (sl_real)(family.GetLineSpacing(style)) * ratio - _out.ascent - _out.descent;
				return sl_true;
			}
		}
		return sl_false;
	}

	namespace
	{
		static void MakeIdentity(MAT2& mat)
		{
			mat.eM11.value = 1;
			mat.eM11.fract = 0;
			mat.eM12.value = 0;
			mat.eM12.fract = 0;
			mat.eM21.value = 0;
			mat.eM21.fract = 0;
			mat.eM22.value = 1;
			mat.eM22.fract = 0;
		}
	}

	sl_bool Font::_measureChar_PO(sl_char32 ch, TextMetrics& _out)
	{
		HFONT hFont = GraphicsPlatform::getGdiFont(this);
		if (!hFont) {
			return sl_false;
		}
		sl_bool bRet = sl_false;
		HDC hdc = CreateCompatibleDC(NULL);
		if (hdc) {
			HFONT hFontOld = (HFONT)(SelectObject(hdc, hFont));
			UINT16 index = 0xffff;
			if (!(ch >> 16)) {
				GetGlyphIndicesW(hdc, (WCHAR*)&ch, 1, &index, GGI_MARK_NONEXISTING_GLYPHS);
			}
			if (index != 0xffff) {
				MAT2 mat;
				MakeIdentity(mat);
				GLYPHMETRICS gm;
				DWORD dwRet = GetGlyphOutlineW(hdc, (UINT)ch, GGO_METRICS, &gm, 0, sl_null, &mat);
				if (dwRet != GDI_ERROR) {
					FontMetrics fm;
					if (getFontMetrics(fm)) {
						_out.left = (sl_real)(gm.gmptGlyphOrigin.x);
						_out.top = fm.ascent + 1.0f - (sl_real)(gm.gmptGlyphOrigin.y);
						_out.right = _out.left + (sl_real)(gm.gmBlackBoxX);
						_out.bottom = _out.top + (sl_real)(gm.gmBlackBoxY);
						_out.advanceX = (sl_real)(gm.gmCellIncX);
						_out.advanceY = fm.ascent + fm.descent + fm.leading;
						bRet = sl_true;
					}
				}
			}
			if (!bRet) {
				String s = String::create(&ch, 1);
				if (s.isNotNull()) {
					if (measureText(s, _out)) {
						bRet = sl_true;
					}
				}
			}
			SelectObject(hdc, hFontOld);
			DeleteDC(hdc);
		}
		return bRet;
	}

	sl_bool Font::_measureText_PO(const StringParam& _text, TextMetrics& _out)
	{
		FontStaticContext* fs = GetFontStaticContext();
		if (!fs) {
			return sl_false;
		}
		Gdiplus::Font* handle = GraphicsPlatform::getGdiplusFont(this);
		if (!handle) {
			return sl_false;
		}
		Gdiplus::Bitmap bitmap(1, 1, PixelFormat24bppRGB);
		Gdiplus::Graphics graphics(&bitmap);
		graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
		StringData16 text(_text);
		Gdiplus::RectF bound;
		Gdiplus::PointF origin(0.0f, 0.0f);
		Gdiplus::Status result = graphics.MeasureString((WCHAR*)(text.getData()), (INT)(text.getLength()), handle, origin, Gdiplus::StringFormat::GenericTypographic(), &bound);
		if (result != Gdiplus::Ok) {
			return sl_false;
		}
		_out.left = (sl_real)(bound.X);
		_out.top = (sl_real)(bound.Y);
		_out.right = (sl_real)(bound.X + bound.Width);
		_out.bottom = (sl_real)(bound.Y + bound.Height);
		return sl_true;
	}

	namespace
	{
		static sl_real ToRealValue(const FIXED& f)
		{
			return (sl_real)(f.value) + (sl_real)(f.fract) / 65536.0f;
		}

		static Point ToPoint(const POINTFX& pt, sl_real x, sl_real ascent)
		{
			return { x + ToRealValue(pt.x), ascent - ToRealValue(pt.y) };
		}
	}

	sl_bool Font::_buildOutline_PO(Ref<GraphicsPath>& path, sl_real x, sl_real y, sl_char32 ch, sl_real& advanceX)
	{
		FontMetrics fm;
		if (!getFontMetrics(fm)) {
			return sl_false;
		}
		HFONT hFont = GraphicsPlatform::getGdiFont(this);
		if (!hFont) {
			return sl_false;
		}
		sl_bool bRet = sl_false;
		sl_real ascent = y + fm.ascent + 1.0f;
		HDC hdc = CreateCompatibleDC(NULL);
		if (hdc) {
			HFONT hFontOld = (HFONT)(SelectObject(hdc, hFont));
			MAT2 mat;
			MakeIdentity(mat);
			GLYPHMETRICS gm;
			DWORD dwRet = GetGlyphOutlineW(hdc, (UINT)ch, GGO_BEZIER, &gm, 0, sl_null, &mat);
			if (dwRet != GDI_ERROR && dwRet) {
				SLIB_SCOPED_BUFFER(sl_uint8, 4096, data, dwRet)
				if (data) {
					if (GetGlyphOutlineW(hdc, (UINT)ch, GGO_BEZIER, &gm, dwRet, data, &mat) != GDI_ERROR) {
						sl_uint8* endOutline = data + dwRet;
						while (data < endOutline) {
							TTPOLYGONHEADER* header = (TTPOLYGONHEADER*)data;
							path->moveTo(ToPoint(header->pfxStart, x, ascent));
							sl_uint8* endContour = data + header->cb;
							data += sizeof(TTPOLYGONHEADER);
							while (data < endContour) {
								TTPOLYCURVE* curve = (TTPOLYCURVE*)data;
								WORD i = 0;
								switch (curve->wType) {
									case TT_PRIM_LINE:
										for (i = 0; i < curve->cpfx; i++) {
											path->lineTo(ToPoint(curve->apfx[i], x, ascent));
										}
										break;
									case TT_PRIM_QSPLINE:
										for (i = 0; i < curve->cpfx; i += 2) {
											path->conicTo(ToPoint(curve->apfx[i], x, ascent), ToPoint(curve->apfx[i + 1], x, ascent));
										}
										break;
									case TT_PRIM_CSPLINE:
										for (i = 0; i < curve->cpfx; i += 3) {
											path->cubicTo(ToPoint(curve->apfx[i], x, ascent), ToPoint(curve->apfx[i + 1], x, ascent), ToPoint(curve->apfx[i + 2], x, ascent));
										}
										break;
									default:
										break;
								}
								data = (sl_uint8*)(curve->apfx + curve->cpfx);
							}
							path->closeSubpath();
						}
						advanceX = (sl_real)(gm.gmCellIncX);
						bRet = sl_true;
					}
				}
			} else {
				dwRet = GetGlyphOutlineW(hdc, (UINT)ch, GGO_METRICS, &gm, 0, sl_null, &mat);
				if (dwRet != GDI_ERROR) {
					advanceX = (sl_real)(gm.gmCellIncX);
				}
			}
			SelectObject(hdc, hFontOld);
			DeleteDC(hdc);
		}
		return bRet;
	}

	Gdiplus::Font* GraphicsPlatform::getGdiplusFont(Font* _font)
	{
		if (_font) {
			FontHelper* font = (FontHelper*)_font;
			return font->getGdiplus();
		}
		return NULL;
	}

	HFONT GraphicsPlatform::getGdiFont(Font* _font)
	{
		if (_font) {
			FontHelper* font = (FontHelper*)_font;
			return font->getGDI();
		}
		return NULL;
	}

	namespace
	{
		int CALLBACK EnumFontFamilyNamesProc(
			const LOGFONT* plf,
			const TEXTMETRIC *lpntme,
			DWORD FontType,
			LPARAM lParam
		)
		{
			ENUMLOGFONTEXW& elf = *((ENUMLOGFONTEXW*)plf);
			HashMap<String, sl_bool>& map = *((HashMap<String, sl_bool>*)lParam);
			map.put_NoLock(String::create(elf.elfLogFont.lfFaceName), sl_true);
			return TRUE;
		}
	}

	List<String> Font::getAllFamilyNames()
	{
		HDC hDC = GetDC(NULL);
		if (hDC) {
			HashMap<String, sl_bool> map;
			LOGFONTW lf;
			Base::zeroMemory(&lf, sizeof(lf));
			lf.lfCharSet = DEFAULT_CHARSET;
			EnumFontFamiliesExW(hDC, &lf, EnumFontFamilyNamesProc, (LPARAM)&map, 0);
			ReleaseDC(NULL, hDC);
			return map.getAllKeys();
		}
		return sl_null;
	}

	sl_bool Font::addResource(const StringParam& _filePath)
	{
		FontStaticContext* context = GetFontStaticContext();
		if (!context) {
			return sl_false;
		}
		StringCstr16 filePath(_filePath);
		// Register Font Collection
		{
			SharedPtr<Gdiplus::PrivateFontCollection> collection = new Gdiplus::PrivateFontCollection;
			if (collection.isNotNull()) {
				if (collection->AddFontFile((WCHAR*)(filePath.getData())) != Gdiplus::Ok) {
					return sl_false;
				}
				context->addCollection(collection);
			}
		}
		int nFonts = AddFontResourceExW((WCHAR*)(filePath.getData()), FR_PRIVATE, NULL);
		if (nFonts <= 0) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool Font::addResource(const void* data, sl_size size)
	{
		FontStaticContext* context = GetFontStaticContext();
		if (!context) {
			return sl_false;
		}
		// Register Font Collection
		{
			SharedPtr<Gdiplus::PrivateFontCollection> collection = new Gdiplus::PrivateFontCollection;
			if (collection.isNotNull()) {
				if (collection->AddMemoryFont(data, (INT)size) != Gdiplus::Ok) {
					return sl_false;
				}
				context->addCollection(collection);
			}
		}
		DWORD dwFonts = 0;
		HANDLE handle = AddFontMemResourceEx((void*)data, (DWORD)size, NULL, &dwFonts);
		if (!handle) {
			return sl_false;
		}
		return sl_true;
	}

}

#endif
