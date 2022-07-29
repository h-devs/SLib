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

#include "slib/graphics/definition.h"

#if defined(SLIB_GRAPHICS_IS_GDI)

#include "slib/graphics/font.h"

#include "slib/core/hash_map.h"
#include "slib/core/memory_reader.h"
#include "slib/core/safe_static.h"
#include "slib/core/shared.h"
#include "slib/graphics/platform.h"

namespace slib
{

	namespace priv
	{
		namespace gdi
		{

			class FontStaticContext
			{
			public:
				Gdiplus::Bitmap* image;
				Gdiplus::Graphics* graphics;
				CHashMap< String, SharedPtr<Gdiplus::PrivateFontCollection> > fontCollections;

			public:
				FontStaticContext()
				{
					graphics = sl_null;

					GraphicsPlatform::startGdiplus();
					image = new Gdiplus::Bitmap(1, 1, PixelFormat24bppRGB);
					if (image) {
						graphics = new Gdiplus::Graphics(image);
					}
				}

				~FontStaticContext()
				{
					if (graphics) {
						delete graphics;
					}
					if (image) {
						delete image;
					}
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

			class FontPlatformObject : public Referable
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
					if (m_flagCreatedGdiplus) {
						return;
					}

					FontStaticContext* context = GetFontStaticContext();
					if (!context) {
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
	}

	using namespace priv::gdi;

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

	Size Font::_measureText_PO(const StringParam& _text)
	{
		Gdiplus::Font* handle = GraphicsPlatform::getGdiplusFont(this);
		if (!handle) {
			return Size::zero();
		}

		FontStaticContext* fs = GetFontStaticContext();
		if (!fs) {
			return Size::zero();
		}

		Size ret(0, 0);
		if (fs->graphics) {
			StringData16 text(_text);
			Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
			format.SetFormatFlags(format.GetFormatFlags() | Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
			Gdiplus::RectF bound;
			Gdiplus::PointF origin(0, 0);
			Gdiplus::Status result = fs->graphics->MeasureString((WCHAR*)(text.getData()), (INT)(text.getLength()), handle, origin, &format, &bound);
			if (result == Gdiplus::Ok) {
				ret.x = bound.Width;
				ret.y = bound.Height;
			}
		}
		return ret;

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
