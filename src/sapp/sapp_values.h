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

#ifndef CHECKHEADER_SLIB_SDEV_SAPP_VALUES
#define CHECKHEADER_SLIB_SDEV_SAPP_VALUES

#include "slib/core/string.h"
#include "slib/core/time.h"
#include "slib/math/vector2.h"
#include "slib/math/vector3.h"
#include "slib/math/vector4.h"
#include "slib/graphics/constants.h"
#include "slib/graphics/color.h"
#include "slib/graphics/font.h"
#include "slib/ui/constants.h"
#include "slib/ui/cursor.h"
#include "slib/data/xml.h"

namespace slib
{

	template <class T>
	class SAppStateMap
	{
	public:
		typedef T VALUE;
		HashMap<ViewState, T> values;

	public:
		void mergeDefault(SAppStateMap& base)
		{
			for (auto&& item : base.values) {
				if (!(values.find_NoLock(item.key))) {
					values.put_NoLock(item.key, item.value);
				}
			}
		}
	};

	class SAppDocument;
	class SAppLayoutXmlItem;

	class SAppStringValue
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_bool flagReferResource = sl_false;
		String valueOrName;
		String variant;
		Ref<XmlElement> referingElement;

	public:
		sl_bool parse(const String& str, const Ref<XmlElement>& element);

	};

	class SAppDimensionBaseValue
	{
	public:
		sl_bool flagDefined = sl_false;
		enum {
			FILL, MATCH_PARENT, WRAP, WEIGHT, // Special Units
			PX,
			SW, SH, SMIN, SMAX,
			VW, VH, VMIN, VMAX,
			SP,
			DP, PT, MM, CM, M, INCH,
			SBAR,
			SAFE_L, SAFE_T, SAFE_R, SAFE_B, SAFE_W, SAFE_H
		};
		int unit = PX;
		sl_real amount = 0;

	public:
		sl_bool parse(const String& str, SAppDocument* doc);

		sl_bool checkGlobal(sl_bool flagRoot = sl_false);

		sl_bool checkSP(sl_bool flagRoot = sl_false);

		sl_bool checkPosition(sl_bool flagRoot = sl_false);

		sl_bool checkSize(sl_bool flagRoot = sl_false);

		sl_bool checkScalarSize(sl_bool flagRoot = sl_false);

		sl_bool checkScalarSizeOrWeight(sl_bool flagRoot = sl_false);

		sl_bool checkMargin(sl_bool flagRoot = sl_false);

		sl_bool checkForWindow(sl_bool flagRoot = sl_false);

		sl_bool checkForWindowSize(sl_bool flagRoot = sl_false);

		sl_bool isNeededOnLayoutFunction();

		static sl_bool isSpecialUnit(int unit);

		static sl_bool isAbsoluteUnit(int unit);

		static sl_bool isGlobalUnit(int unit);

		static sl_bool isViewportUnit(int unit);

	};

	class SAppDimensionValue : public SAppDimensionBaseValue
	{
	public:
		String getAccessString() const;
	};

	class SAppDimensionFloatValue : public SAppDimensionBaseValue
	{
	public:
		String getAccessString() const;
	};

	class SAppBooleanValue
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_bool value = sl_false;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppFloatValue
	{
	public:
		sl_bool flagDefined = sl_false;
		float value = 0;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppInt32Value
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_int32 value = 0;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppUint32Value
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_uint32 value = 0;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppInt64Value
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_int64 value = 0;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppUint64Value
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_uint64 value = 0;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppChar8Value
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_char8 value = 0;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppVector2Value
	{
	public:
		sl_bool flagDefined = sl_false;
		Vector2 value;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppVector3Value
	{
	public:
		sl_bool flagDefined = sl_false;
		Vector3 value;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppVector4Value
	{
	public:
		sl_bool flagDefined = sl_false;
		Vector4 value;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppVisibilityValue
	{
	public:
		sl_bool flagDefined = sl_false;
		Visibility value = Visibility::Visible;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppPenStyleValue
	{
	public:
		sl_bool flagDefined = sl_false;
		PenStyle value = PenStyle::Solid;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	// scrollBars (none, horizontal, vertical, both)
	class SAppScrollBarsValue
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_bool horizontalScrollBar = sl_false;
		sl_bool verticalScrollBar = sl_false;

	public:
		sl_bool parse(const String& str);

	};

	class SAppNameValue
	{
	public:
		sl_bool flagDefined = sl_false;
		String value;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppColorValue
	{
	public:
		sl_bool flagDefined = sl_false;

		String resourceName;
		Color color;
		Ref<XmlElement> referingElement;

	public:
		sl_bool parse(const String& str, const Ref<XmlElement>& element);

	};

	class SAppTimeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		Time value;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppDrawableValue : public SAppColorValue
	{
	public:
		sl_bool flagNull = sl_false;
		sl_bool flagColor = sl_false;

		sl_bool flagWhole = sl_false;
		sl_real x = 0;
		sl_real y = 0;
		sl_real width = 0;
		sl_real height = 0;

		enum {
			FUNC_NONE, FUNC_NINEPATCH, FUNC_THREEPATCH_HORIZONTAL, FUNC_THREEPATCH_VERTICAL
		};
		int func = FUNC_NONE;
		SAppDimensionValue patchLeftWidthDst;
		SAppDimensionValue patchRightWidthDst;
		SAppDimensionValue patchTopHeightDst;
		SAppDimensionValue patchBottomHeightDst;
		sl_real patchLeftWidth = 0;
		sl_real patchRightWidth = 0;
		sl_real patchTopHeight = 0;
		sl_real patchBottomHeight = 0;

		Ref<XmlElement> referingElement;

	public:
		sl_bool parse(const String& str, SAppDocument* doc, const Ref<XmlElement>& element);

		sl_bool parseWhole(const String& str, const Ref<XmlElement>& element);

		sl_bool isAbsoluteUnit();

		sl_bool isGlobalUnit();

	};

	class SAppFontValue
	{
	public:
		sl_bool flagDefined = sl_false;
		SAppStringValue family;
		SAppDimensionFloatValue size;
		SAppBooleanValue bold;
		SAppBooleanValue italic;
		SAppBooleanValue underline;

	public:
		void inheritFrom(const SAppFontValue& parent);

		sl_bool parse(SAppLayoutXmlItem* xml, const StringView& name, const StringView& suffix, SAppDocument* doc, sl_bool flagRoot);

	};

	class SAppBorderValue
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_bool flagNull = sl_false;
		SAppPenStyleValue style;
		SAppDimensionFloatValue width;
		SAppColorValue color;

	public:
		void inheritFrom(const SAppBorderValue& parent);

		static void normalizeStateMap(SAppStateMap<SAppBorderValue>& map);

		sl_bool parse(SAppLayoutXmlItem* xml, const StringView& name, const StringView& suffix, SAppDocument* doc, sl_bool flagRoot);

	};

	class SAppMenuValue
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_bool flagNull = sl_false;

		String resourceName;
		Ref<XmlElement> referingElement;

	public:
		sl_bool parse(const String& str, const Ref<XmlElement>& element);

	};

	class SAppAlignLayoutValue
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_bool flagAlignParent = sl_false;
		String referingView;

	public:
		sl_bool parse(const String& str);

	};

	// Scrolling (horizontal, vertical, both)
	class SAppScrollingValue
	{
	public:
		sl_bool flagDefined = sl_false;
		sl_bool horizontal = sl_false;
		sl_bool vertical = sl_false;

	public:
		sl_bool parse(const String& str);

	};

	class SAppLayoutOrientationValue
	{
	public:
		sl_bool flagDefined = sl_false;
		LayoutOrientation value = LayoutOrientation::Vertical;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppAlignmentValue
	{
	public:
		sl_bool flagDefined = sl_false;
		Alignment value = Alignment::Default;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppScaleModeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		ScaleMode value = ScaleMode::None;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppBoundShapeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		BoundShape value = BoundShape::None;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppRedrawModeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		RedrawMode value = RedrawMode::Continuously;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppMultiLineModeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		MultiLineMode value = MultiLineMode::Single;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppUIReturnKeyTypeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		UIReturnKeyType value = UIReturnKeyType::Default;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppUIKeyboardTypeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		UIKeyboardType value = UIKeyboardType::Default;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppUIAutoCapitalizationTypeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		UIAutoCapitalizationType value = UIAutoCapitalizationType::None;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppRotationModeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		RotationMode value = RotationMode::Rotate0;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppFlipModeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		FlipMode value = FlipMode::None;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppEllipsizeModeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		EllipsizeMode value = EllipsizeMode::None;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppCursorValue
	{
	public:
		sl_bool flagDefined = sl_false;
		Ref<Cursor> value;

		enum Type
		{
			NONE,
			ARROW,
			IBEAM,
			CROSS,
			HAND,
			RESIZE_LEFT_RIGHT,
			RESIZE_UP_DOWN
		};
		int type = NONE;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppAntiAliasModeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		AntiAliasMode value = AntiAliasMode::Inherit;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

	class SAppGridSelectionModeValue
	{
	public:
		sl_bool flagDefined = sl_false;
		GridView::SelectionMode value = GridView::SelectionMode::Cell;

	public:
		String getAccessString() const;

		sl_bool parse(const String& str);

	};

}

#endif
