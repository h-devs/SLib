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

#if defined(SLIB_GRAPHICS_IS_QUARTZ)

#include "slib/graphics/font.h"

#include "slib/graphics/platform.h"

#if defined(SLIB_PLATFORM_IS_MACOS)
typedef NSFontDescriptor UIFontDescriptor;
#endif

namespace slib
{

	namespace {

		class FontPlatformObject : public CRef
		{
		public:
			UIFont* m_font;
			sl_bool m_flagCreatedFont;
			CGFloat m_lastUIScaleFactor;

			SpinLock m_lock;

		public:
			FontPlatformObject()
			{
				m_font = nil;
				m_flagCreatedFont = sl_false;
				m_lastUIScaleFactor = 0;
			}

			~FontPlatformObject()
			{
				m_font = nil;
			}

		public:
			UIFont* _createFont(const FontDesc& desc, CGFloat scaleFactor)
			{
				SpinLocker lock(&m_lock);

				if (m_flagCreatedFont && m_lastUIScaleFactor == scaleFactor) {
					return m_font;
				}

				m_flagCreatedFont = sl_true;

				float size = desc.size / scaleFactor;
				NSString* familyName = Apple::getNSStringFromString(desc.familyName);
				uint32_t traits = 0;
				UIFontDescriptor* descriptor = [UIFontDescriptor fontDescriptorWithName:familyName size:size];
				if (descriptor == nil) {
					return nil;
				}
#if defined(SLIB_PLATFORM_IS_MACOS)
				if (desc.flagBold) {
					traits |= NSFontBoldTrait;
				}
				if (desc.flagItalic) {
					traits |= NSFontItalicTrait;
				}
#else
				if (desc.flagBold) {
					traits |= UIFontDescriptorTraitBold;
				}
				if (desc.flagItalic) {
					traits |= UIFontDescriptorTraitItalic;
				}
#endif
				if (traits) {
					UIFontDescriptor* descriptorWithTraits = [descriptor fontDescriptorWithSymbolicTraits:traits];
					if (descriptorWithTraits != nil) {
						descriptor = descriptorWithTraits;
					}
				}
				UIFont* font = [UIFont fontWithDescriptor:descriptor size:size];
				if (font == nil) {
					descriptor = [UIFontDescriptor fontDescriptorWithName:@"Arial" size:size];
					if (descriptor != nil) {
						if (traits) {
							UIFontDescriptor* descriptorWithTraits = [descriptor fontDescriptorWithSymbolicTraits:traits];
							if (descriptorWithTraits != nil) {
								descriptor = descriptorWithTraits;
							}
						}
						font = [UIFont fontWithDescriptor:descriptor size:size];
					}
				}
				m_font = font;
				m_lastUIScaleFactor = scaleFactor;
				return m_font;
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

			UIFont* getFontObject(CGFloat scaleFactor)
			{
				FontPlatformObject* po = getPlatformObject();
				if (po) {
					return po->_createFont(m_desc, scaleFactor);
				}
				return nil;
			}

		};

	}

	sl_bool Font::_getFontMetrics_PO(FontMetrics& _out)
	{
		UIFont* hFont = GraphicsPlatform::getNativeFont(this);
		if (hFont == nil) {
			return sl_false;
		}
		_out.ascent = (sl_real)(hFont.ascender);
		_out.descent = -((sl_real)(hFont.descender));
		_out.leading = (sl_real)(hFont.leading);
		return sl_true;
	}

	sl_bool Font::_measureChar_PO(sl_char32 _ch, TextMetrics& _out)
	{
		UIFont* hFont = GraphicsPlatform::getNativeFont(this);
		if (hFont == nil) {
			return sl_false;
		}
		CTFontRef font = (__bridge CTFontRef)hFont;
		CGGlyph glyph = 0;
		UniChar ch = (UniChar)_ch;
		CTFontGetGlyphsForCharacters(font, &ch, &glyph, 1);
		if (!glyph) {
			return sl_false;
		}
		CGFloat ascent = CTFontGetAscent(font);
		CGRect rect = CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationHorizontal, &glyph, sl_null, 1);
		if (Base::equalsMemory(&rect, &(CGRectZero), sizeof(CGRect))) {
			return sl_false;
		}
		_out.left = (sl_real)(rect.origin.x);
		_out.bottom = (sl_real)(ascent - rect.origin.y);
		_out.right = (sl_real)(rect.origin.x + rect.size.width);
		_out.top = (sl_real)(_out.bottom - rect.size.height);
		double advance = CTFontGetAdvancesForGlyphs(font, kCTFontOrientationHorizontal, &glyph, sl_null, 1);
		_out.advanceX = (sl_real)(advance);
		_out.advanceY = getFontHeight();
		return sl_true;
	}

	sl_bool Font::_measureText_PO(const StringParam& _text, TextMetrics& _out)
	{
		UIFont* hFont = GraphicsPlatform::getNativeFont(this);
		if (hFont == nil) {
			return sl_false;
		}
		NSString* text = Apple::getNSStringFromString(_text);
		if (text == nil) {
			return sl_false;
		}
		NSAttributedString* attrText = [[NSAttributedString alloc] initWithString:text attributes:@{NSFontAttributeName: hFont}];
		if (text == nil) {
			return sl_false;
		}
#if defined(SLIB_PLATFORM_IS_MACOS)
		NSRect bounds = [attrText boundingRectWithSize:NSMakeSize(CGFLOAT_MAX, CGFLOAT_MAX) options:0 context:nil];
#else
		CGRect bounds = [attrText boundingRectWithSize:CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX) options:0 context:nil];
#endif
		_out.left = (sl_real)(bounds.origin.x);
		_out.top = (sl_real)(getFontAscent() - (bounds.origin.y + bounds.size.height));
		_out.right = _out.left + (sl_real)(bounds.size.width);
		_out.bottom = _out.top + (sl_real)(bounds.size.height);
		return sl_true;
	}

	namespace
	{
		struct PathApplierInfo
		{
			GraphicsPath* path;
			sl_real x;
			sl_real y;
			sl_real ascent;
		};

		static Point ConvertPoint(PathApplierInfo* info, CGPoint& pt)
		{
			return { info->x + (sl_real)(pt.x), info->y + info->ascent - (sl_real)(pt.y) };
		}

		static void PathApplier(void* _info, const CGPathElement* element)
		{
			PathApplierInfo* info = (PathApplierInfo*)_info;
			switch (element->type) {
				case kCGPathElementMoveToPoint:
					info->path->moveTo(ConvertPoint(info, *(element->points)));
					break;
				case kCGPathElementAddLineToPoint:
					info->path->lineTo(ConvertPoint(info, *(element->points)));
					break;
				case kCGPathElementAddQuadCurveToPoint:
					info->path->conicTo(ConvertPoint(info, *(element->points)), ConvertPoint(info, element->points[1]));
					break;
				case kCGPathElementAddCurveToPoint:
					info->path->cubicTo(ConvertPoint(info, *(element->points)), ConvertPoint(info, element->points[1]), ConvertPoint(info, element->points[2]));
					break;
				case kCGPathElementCloseSubpath:
					info->path->closeSubpath();
					break;
				default:
					break;
			}
		}
	}

	sl_bool Font::_buildOutline_PO(Ref<GraphicsPath>& _out, sl_real x, sl_real y, sl_char32 _ch, sl_real& advanceX)
	{
		UIFont* hFont = GraphicsPlatform::getNativeFont(this);
		if (hFont == nil) {
			return sl_false;
		}
		CTFontRef font = (__bridge CTFontRef)hFont;
		CGGlyph glyph = 0;
		UniChar ch = (UniChar)_ch;
		CTFontGetGlyphsForCharacters(font, &ch, &glyph, 1);
		if (!glyph) {
			return sl_false;
		}
		CGPathRef path = CTFontCreatePathForGlyph(font, glyph, sl_null);
		if (!path) {
			return sl_false;
		}
		PathApplierInfo info;
		info.path = _out.get();
		info.x = x;
		info.y = y;
		info.ascent = (sl_real)(CTFontGetAscent(font));
		CGPathApply(path, &info, &PathApplier);
		double advance = CTFontGetAdvancesForGlyphs(font, kCTFontOrientationHorizontal, &glyph, sl_null, 1);
		advanceX = (sl_real)advance;
		return sl_true;
	}

	List<String> Font::getAllFamilyNames()
	{
		List<String> ret;
#if defined(SLIB_PLATFORM_IS_MACOS)
		NSArray *fonts = [[NSFontManager sharedFontManager] availableFontFamilies];
		for (NSString* name in fonts) {
			ret.add_NoLock(Apple::getStringFromNSString(name));
		}
#endif
		return ret;
	}

	UIFont* GraphicsPlatform::getNativeFont(Font* _font, CGFloat scaleFactor)
	{
		if (_font) {
			FontHelper* font = (FontHelper*)_font;
			return font->getFontObject(scaleFactor);
		}
		return nil;
	}

}

#endif
