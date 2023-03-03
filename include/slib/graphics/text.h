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

#ifndef CHECKHEADER_SLIB_GRAPHICS_TEXT
#define CHECKHEADER_SLIB_GRAPHICS_TEXT

#include "constants.h"
#include "canvas.h"

#include "../core/string.h"
#include "../core/function.h"
#include "../math/rectangle.h"

namespace slib
{

	enum class TextItemType
	{
		Word = 0,
		Space = 10,
		Tab = 11,
		LineBreak = 20,
		JoinedChar = 50,
		Attach = 100
	};

	typedef sl_size sl_text_pos;

#define SLIB_TEXT_RANGE_NOT_FOUND SLIB_SIZE_MAX

	class SLIB_EXPORT TextRange
	{
	public:
		sl_text_pos location;
		sl_text_pos length;

	public:
		SLIB_CONSTEXPR TextRange() noexcept : location(SLIB_TEXT_RANGE_NOT_FOUND), length(0) {}

		SLIB_CONSTEXPR TextRange(sl_null_t) noexcept : location(SLIB_TEXT_RANGE_NOT_FOUND), length(0) {}

		SLIB_CONSTEXPR TextRange(sl_text_pos _location, sl_text_pos _length) noexcept : location(_location), length(_length) {}

		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(TextRange)

	public:
		SLIB_CONSTEXPR sl_bool operator==(const TextRange& other) const noexcept
		{
			return location == other.location && length == other.length;
		}

		SLIB_CONSTEXPR sl_bool operator!=(const TextRange& other) const noexcept
		{
			return location != other.location || length != other.length;
		}

		SLIB_CONSTEXPR sl_bool isNotFound() const noexcept
		{
			return location == SLIB_TEXT_RANGE_NOT_FOUND;
		}

	};

	class SLIB_EXPORT TextStyle : public Referable
	{
	public:
		Ref<Font> font;
		String joinedCharFamilyName;
		sl_bool flagDefinedUnderline : 1;
		sl_bool flagUnderline : 1;
		sl_bool flagOverline : 1;
		sl_bool flagLineThrough : 1;
		sl_bool flagLink : 1;
		Color textColor;
		Color backgroundColor;
		String href;
		sl_real lineHeight;
		sl_real yOffset;

	public:
		TextStyle() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TextStyle)

	public:
		Ref<TextStyle> duplicate() const noexcept;

	};

	class SLIB_EXPORT TextItem : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		class DrawParam
		{
		public:
			Color textColor;
			Color backgroundColor;

			sl_real shadowOpacity;
			sl_real shadowRadius;
			Color shadowColor;
			Point shadowOffset;

			sl_real lineThickness;

			sl_bool flagDrawSelection;
			sl_reg selectionStart;
			sl_reg selectionEnd;
			Color selectedTextColor;
			Color selectedBackgroundColor;

		public:
			DrawParam() noexcept;
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DrawParam)

		public:
			void fixSelectionRange() noexcept;

		};

	protected:
		TextItem(TextItemType type) noexcept;

		~TextItem() noexcept;

	public:
		TextItemType getType() noexcept;

		Ref<TextStyle> getStyle() noexcept;

		void setStyle(const Ref<TextStyle>& style) noexcept;

		Ref<Font> getFont() noexcept;

		Ref<Font> getJoinedCharFont() noexcept;

		Point getLayoutPosition() noexcept;

		void setLayoutPosition(const Point& pt) noexcept;

		Size getLayoutSize() noexcept;

		void setLayoutSize(const Size& size) noexcept;

		Rectangle getLayoutFrame() noexcept;

	protected:
		TextItemType m_type;
		AtomicRef<TextStyle> m_style;
		Point m_layoutPosition;
		Size m_layoutSize;
		AtomicRef<Font> m_joinedCharFont;
		AtomicRef<Font> m_joinedCharFontBase;

	};

	class SLIB_EXPORT TextWordItem : public TextItem
	{
		SLIB_DECLARE_OBJECT

	private:
		TextWordItem() noexcept;

		~TextWordItem() noexcept;

	public:
		static Ref<TextWordItem> create(const String16& text, const Ref<TextStyle>& style, sl_bool flagEnabledHyperlinksInPlainText = sl_false) noexcept;

	public:
		String16 getText() noexcept;

		void setText(const String16& text) noexcept;

		Size getSize() noexcept;

		void draw(Canvas* canvas, sl_real x, sl_real y, const DrawParam& param);

	private:
		String16 m_text;

		Ref<Font> m_fontCached;
		String16 m_textCached;
		sl_real m_widthCached;
		sl_real m_heightCached;

	};

	class SLIB_EXPORT TextJoinedCharItem : public TextItem
	{
		SLIB_DECLARE_OBJECT

	private:
		TextJoinedCharItem() noexcept;

		~TextJoinedCharItem() noexcept;

	public:
		static Ref<TextJoinedCharItem> create(const String16& text, const Ref<TextStyle>& style) noexcept;

	public:
		Size getSize() noexcept;

		void draw(Canvas* canvas, sl_real x, sl_real y, const DrawParam& param);

	private:
		String16 m_text;

		Ref<Font> m_fontCached;
		sl_real m_widthCached;
		sl_real m_heightCached;

	};

	class SLIB_EXPORT TextSpaceItem : public TextItem
	{
		SLIB_DECLARE_OBJECT

	private:
		TextSpaceItem() noexcept;

		~TextSpaceItem() noexcept;

	public:
		static Ref<TextSpaceItem> create(const Ref<TextStyle>& style) noexcept;

	public:
		Size getSize() noexcept;

	};

	class SLIB_EXPORT TextTabItem : public TextItem
	{
		SLIB_DECLARE_OBJECT

	private:
		TextTabItem() noexcept;

		~TextTabItem() noexcept;

	public:
		static Ref<TextTabItem> create(const Ref<TextStyle>& style) noexcept;

	public:
		sl_real getHeight() noexcept;

	};

	class SLIB_EXPORT TextLineBreakItem : public TextItem
	{
		SLIB_DECLARE_OBJECT

	private:
		TextLineBreakItem() noexcept;

		~TextLineBreakItem() noexcept;

	public:
		static Ref<TextLineBreakItem> create(const Ref<TextStyle>& style) noexcept;

	public:
		sl_real getHeight() noexcept;

	};


	class SLIB_EXPORT TextAttachItem : public TextItem
	{
		SLIB_DECLARE_OBJECT

	public:
		TextAttachItem() noexcept;

		~TextAttachItem() noexcept;

	public:
		virtual Size getSize() noexcept = 0;

		virtual void setPosition(const Point& pos) noexcept = 0;

	};

	class XmlNodeGroup;
	class XmlElement;

	class SLIB_EXPORT TextParagraph : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		TextParagraph() noexcept;

		~TextParagraph() noexcept;

	public:
		void addText(const StringParam& text, const Ref<TextStyle>& style, sl_bool flagEnabledHyperlinksInPlainText = sl_false, sl_bool flagMnemonic = sl_false) noexcept;

		void addHyperTextNodeGroup(const Ref<XmlNodeGroup>& group, const Ref<TextStyle>& style) noexcept;

		void addHyperTextElement(const Ref<XmlElement>& element, const Ref<TextStyle>& style) noexcept;

		void addHyperText(const StringParam& text, const Ref<TextStyle>& style) noexcept;

		class LayoutParam
		{
		public:
			sl_real width;
			sl_real tabWidth;
			sl_real tabMargin;
			Alignment align;
			MultiLineMode multiLineMode;
			EllipsizeMode ellipsisMode;
			sl_uint32 lineCount;

		public:
			LayoutParam() noexcept;
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LayoutParam)
		};

		void layout(const LayoutParam& param) noexcept;

		class DrawParam : public TextItem::DrawParam
		{
		public:
			Color linkColor;

		public:
			DrawParam() noexcept;
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DrawParam)
		};

		void draw(Canvas* canvas, sl_real left, sl_real right, sl_real y, const DrawParam& param) noexcept;

		Ref<TextItem> getTextItemAtPosition(sl_real x, sl_real y, sl_real left, sl_real right) noexcept;

		sl_real getContentWidth() noexcept;

		sl_real getContentHeight() noexcept;

		Alignment getAlignment() noexcept;

		sl_real getPositionLength() noexcept;

	public:
		static const Color& getDefaultLinkColor();

		static void setDefaultLinkColor(const Color& color);

		static sl_bool isDefaultLinkUnderline();

		static void setDefaultLinkUnderline(sl_bool flag);

	protected:
		CList< Ref<TextItem> > m_items;
		CList< Ref<TextItem> > m_layoutItems;
		sl_real m_contentWidth;
		sl_real m_contentHeight;
		sl_real m_positionLength;
		Alignment m_align;

	};

	class SLIB_EXPORT SimpleTextBoxParam
	{
	public:
		Ref<Font> font;
		String text;
		sl_bool flagHyperText;
		sl_bool flagMnemonic;
		sl_real width;
		MultiLineMode multiLineMode;
		EllipsizeMode ellipsizeMode;
		sl_uint32 lineCount;
		Alignment align;
		sl_bool flagEnabledHyperlinksInPlainText;

	public:
		SimpleTextBoxParam() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SimpleTextBoxParam)

	};

	class SLIB_EXPORT SimpleTextBox : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		SimpleTextBox() noexcept;

		~SimpleTextBox() noexcept;

	public:
		void update(const SimpleTextBoxParam& param) noexcept;

		class DrawParam : public TextParagraph::DrawParam
		{
		public:
			Rectangle frame;

		public:
			DrawParam() noexcept;
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DrawParam)
		};

		void draw(Canvas* canvas, const DrawParam& param) const noexcept;

		Ref<TextItem> getTextItemAtPosition(sl_real x, sl_real y, const Rectangle& frame) const noexcept;

		sl_real getContentWidth() const noexcept;

		sl_real getContentHeight() const noexcept;

	public:
		Ref<Font> getFont() const noexcept;

		String getText() const noexcept;

		MultiLineMode getMultiLineMode() const noexcept;

		EllipsizeMode getEllipsizeMode() const noexcept;

		Alignment getAlignment() const noexcept;

	protected:
		Ref<TextParagraph> m_paragraph;
		Ref<TextStyle> m_style;

		Ref<Font> m_font;
		String m_text;
		sl_bool m_flagHyperText;
		MultiLineMode m_multiLineMode;
		EllipsizeMode m_ellipsisMode;
		sl_uint32 m_lineCount;
		Alignment m_alignHorizontal;
		Alignment m_alignVertical;
		sl_real m_width;

		sl_real m_contentWidth;
		sl_real m_contentHeight;

	};

}

#endif
