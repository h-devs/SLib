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

#ifndef CHECKHEADER_SLIB_SDEV_SAPP_RESOURCES
#define CHECKHEADER_SLIB_SDEV_SAPP_RESOURCES

#include "sapp_values.h"

#include "slib/core/locale.h"
#include "slib/core/property.h"
#include "slib/data/xml.h"
#include "slib/graphics/image.h"
#include "slib/ui/event.h"
#include "slib/ui/resource.h"

namespace slib
{

	enum SAppLayoutOperation
	{
		Parse = 0,
		Generate = 1,
		SimulateInit = 2,
		SimulateLayout = 3
	};

	class SAppStringResourceItem
	{
	public:
		String defaultValue;
		HashMap<Locale, String> values;

	public:
		String get(const Locale& locale, const String& def);
	};

	class SAppStringResource : public CRef, public SAppStringResourceItem
	{
	public:
		String name;
		HashMap<String, SAppStringResourceItem> variants;
	};

	class SAppColorResource : public CRef
	{
	public:
		String name;
		Color value;
	};

	class SAppRawResource : public CRef
	{
	public:
		String name;
		String resourcePath;
		String filePath;

		Map< String, Ref<SAppRawResource> > sub;
	};

	class SAppDrawableResourceFileItem : public Object
	{
	public:
		String fileName;
		String filePath;
		String rawName;
		AtomicRef<Drawable> drawable;

	public:
		Ref<Drawable> load();
	};

	class SAppDrawableResourceFileAttributes : public CRef
	{
	public:
		List< Ref<SAppDrawableResourceFileItem> > defaultFiles;
		CHashMap< Locale, List< Ref<SAppDrawableResourceFileItem> > > files;

	public:
		SAppDrawableResourceFileAttributes();
	};

	class SAppDrawableResourceNinePiecesAttributes : public CRef
	{
	public:
		String localNamespace;

		SAppDimensionValue leftWidth;
		SAppDimensionValue rightWidth;
		SAppDimensionValue topHeight;
		SAppDimensionValue bottomHeight;

		SAppDrawableValue topLeft;
		SAppDrawableValue top;
		SAppDrawableValue topRight;
		SAppDrawableValue left;
		SAppDrawableValue center;
		SAppDrawableValue right;
		SAppDrawableValue bottomLeft;
		SAppDrawableValue bottom;
		SAppDrawableValue bottomRight;
	};

	class SAppDrawableResourceNinePatchAttributes : public CRef
	{
	public:
		String localNamespace;

		SAppDimensionValue dstLeftWidth;
		SAppDimensionValue dstRightWidth;
		SAppDimensionValue dstTopHeight;
		SAppDimensionValue dstBottomHeight;

		SAppDrawableValue src;
		sl_real leftWidth;
		sl_real rightWidth;
		sl_real topHeight;
		sl_real bottomHeight;
	};

	class SAppDrawableResource : public CRef
	{
	public:
		String name;
		int type;
		enum {
			typeUnknown = 0,
			typeFile = 1,
			typeNinePieces = 2,
			typeNinePatch = 3
		};

		Ref<SAppDrawableResourceFileAttributes> fileAttrs;
		Ref<SAppDrawableResourceNinePiecesAttributes> ninePiecesAttrs;
		Ref<SAppDrawableResourceNinePatchAttributes> ninePatchAttrs;

	public:
		SAppDrawableResource();
	};


	class SAppMenuResourceItem : public CRef
	{
	public:
		Ref<XmlElement> element;
		String name;
		int type;
		enum {
			typeSubmenu = 0,
			typeItem = 1,
			typeSeparator = 2
		};
		int platformFlags;
		enum {
			mac = 1,
			windows = 2,
			linux = 4,
			all_platforms = 0xFFFF,
			no_mac = 0xFFFE,
			no_windows = 0xFFFD,
			no_linux = 0xFFFB
		};
		SAppStringValue title;
		KeycodeAndModifiers shortcutKey;
		KeycodeAndModifiers macShortcutKey;
		SAppBooleanValue checked;
		SAppDrawableValue icon;
		SAppDrawableValue checkedIcon;
		CList< Ref<SAppMenuResourceItem> > children;
	};

	class SAppMenuResource : public CRef
	{
	public:
		String name;
		String localNamespace;
		sl_bool flagPopup;
		CList< Ref<SAppMenuResourceItem> > children;
		CHashMap<String, Ref<SAppMenuResourceItem> > itemsWindows;
		CHashMap<String, Ref<SAppMenuResourceItem> > itemsLinux;
		CHashMap<String, Ref<SAppMenuResourceItem> > itemsMac;
	};

	class SAppLayoutResourceItem;
	
	class SAppLayoutViewAttributes : public CRef
	{
	public:
		SAppStringValue id;

		SAppDimensionValue width;
		SAppDimensionValue height;

		PositionMode leftMode;
		PositionMode topMode;
		PositionMode rightMode;
		PositionMode bottomMode;
		String leftReferingView;
		String topReferingView;
		String rightReferingView;
		String bottomReferingView;

		SAppDimensionValue left;
		SAppDimensionValue top;

		SAppDimensionValue minWidth;
		SAppDimensionValue maxWidth;
		SAppDimensionValue minHeight;
		SAppDimensionValue maxHeight;

		SAppFloatValue aspectRatio;

		SAppDimensionValue marginLeft;
		SAppDimensionValue marginTop;
		SAppDimensionValue marginRight;
		SAppDimensionValue marginBottom;

		SAppDimensionValue paddingLeft;
		SAppDimensionValue paddingTop;
		SAppDimensionValue paddingRight;
		SAppDimensionValue paddingBottom;

		SAppVisibilityValue visibility;
		SAppBooleanValue visible;
		SAppBooleanValue enabled;
		SAppBooleanValue clipping;
		SAppBooleanValue drawing;

		SAppStateMap<SAppDrawableValue> background;
		SAppScaleModeValue backgroundScale;
		SAppAlignmentValue backgroundAlign;
		SAppBooleanValue nativeBorder;
		SAppStateMap<SAppBorderValue> border;
		SAppBoundShapeValue boundShape;
		SAppDimensionFloatValue boundRadiusX;
		SAppDimensionFloatValue boundRadiusY;
		SAppDimensionFloatValue boundRadius;
		SAppBoundShapeValue contentShape;
		SAppDimensionFloatValue contentRadiusX;
		SAppDimensionFloatValue contentRadiusY;
		SAppDimensionFloatValue contentRadius;
		SAppStateMap<SAppColorValue> paddingColor;

		SAppFontValue font;

		SAppBooleanValue opaque;
		SAppFloatValue alpha;
		SAppColorValue colorKey;
		SAppBooleanValue antiAlias;
		SAppBooleanValue layer;

		SAppFloatValue shadowOpacity;
		SAppDimensionFloatValue shadowRadius;
		SAppDimensionFloatValue shadowOffsetX;
		SAppDimensionFloatValue shadowOffsetY;
		SAppColorValue shadowColor;

		SAppScrollingValue scrolling;
		SAppScrollBarsValue scrollBars;
		SAppStateMap<SAppDrawableValue> hscrollThumb;
		SAppStateMap<SAppDrawableValue> hscrollTrack;
		SAppStateMap<SAppDrawableValue> vscrollThumb;
		SAppStateMap<SAppDrawableValue> vscrollTrack;
		SAppDimensionValue contentWidth;
		SAppDimensionValue contentHeight;
		SAppBooleanValue paging;
		SAppDimensionValue pageWidth;
		SAppDimensionValue pageHeight;
		SAppBooleanValue scrollingByMouse;
		SAppBooleanValue scrollingByTouch;
		SAppBooleanValue scrollingByMouseWheel;
		SAppBooleanValue scrollingByKeyboard;
		SAppBooleanValue autoHideScrollBar;
		SAppBooleanValue smoothScrolling;

		SAppBooleanValue childFocusedState;
		SAppBooleanValue focusable;
		SAppBooleanValue focus;
		SAppBooleanValue hitTest;
		SAppBooleanValue touchMultipleChildren;
		SAppBooleanValue tabStop;
		String nextTabStop;
		String previousTabStop;
		SAppCursorValue cursor;
		SAppStringValue toolTip;
		SAppBooleanValue ime;

		SAppBooleanValue instance;
		SAppBooleanValue childInstances;
		SAppBooleanValue nativeWidget;
		SAppBooleanValue largeContent;
		SAppBooleanValue emptyContent;
		SAppBooleanValue nativeLayer;

		SAppBooleanValue okCancelEnabled;
		SAppBooleanValue ok;
		SAppBooleanValue cancel;
		String sendFocus;
		SAppChar8Value mnemonicKey;
		SAppBooleanValue keepKeyboard;
		SAppBooleanValue playSoundOnClick;
		SAppBooleanValue clientEdge;

	public:
		SAppLayoutViewAttributes();

	public:
		void resetLayout();

	protected:
		sl_bool isNotRequiredNative(sl_bool flagCheckBackgroundColor);

	};

	class SAppLayoutWindowAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppMenuValue menu;
		SAppStringValue title;

		SAppDimensionValue left;
		SAppDimensionValue top;
		SAppDimensionValue width;
		SAppDimensionValue height;

		SAppDimensionValue minWidth;
		SAppDimensionValue maxWidth;
		SAppDimensionValue minHeight;
		SAppDimensionValue maxHeight;
		SAppFloatValue minAspectRatio;
		SAppFloatValue maxAspectRatio;
		SAppFloatValue aspectRatio;

		SAppBooleanValue minimized;
		SAppBooleanValue maximized;
		SAppBooleanValue visible;
		SAppBooleanValue alwaysOnTop;
		SAppBooleanValue closeButton;
		SAppBooleanValue minimizeButton;
		SAppBooleanValue maximizeButton;
		SAppBooleanValue fullScreenButton;
		SAppBooleanValue resizable;
		SAppBooleanValue layered;
		SAppFloatValue alpha;
		SAppColorValue colorKey;
		SAppBooleanValue transparent;
		SAppBooleanValue taskbar;
		SAppBooleanValue excludeFromCapture;
		SAppColorValue backgroundColor;

		SAppBooleanValue modal;
		SAppBooleanValue dialog;
		SAppBooleanValue borderless;
		SAppBooleanValue titleBar;
		SAppBooleanValue fullScreen;
		SAppBooleanValue centerScreen;
		SAppAlignmentValue gravity;
		SAppDimensionValue marginLeft;
		SAppDimensionValue marginTop;
		SAppDimensionValue marginRight;
		SAppDimensionValue marginBottom;
		SAppBooleanValue closeOnOK;
		SAppBooleanValue closeOnCancel;
	};

	typedef SAppLayoutViewAttributes SAppLayoutPageAttributes;

	typedef SAppLayoutViewAttributes SAppLayoutViewGroupAttributes;

	class SAppLayoutImportAttributes : public SAppLayoutViewAttributes
	{
	public:
		String layout;
	};

	class SAppLayoutButtonCategory
	{
	public:
		SAppStateMap<SAppColorValue> textColor;
		SAppStateMap<SAppDrawableValue> background;
		SAppStateMap<SAppDrawableValue> icon;
		SAppStateMap<SAppBorderValue> border;
		SAppStateMap<SAppColorValue> colorOverlay;
	};

	class SAppLayoutButtonAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppStringValue text;
		SAppStringValue hyperText;
		SAppAlignmentValue gravity;
		SAppMultiLineModeValue multiLine;
		SAppEllipsizeModeValue ellipsize;
		SAppUint32Value lines;
		SAppBooleanValue mnemonic;

		SAppBooleanValue defaultButton;
		SAppDimensionValue iconWidth;
		SAppDimensionValue iconHeight;
		SAppAlignmentValue iconAlign;
		SAppAlignmentValue textAlign;
		SAppBooleanValue textBeforeIcon;
		SAppBooleanValue extendTextFrame;
		SAppLayoutOrientationValue orientation;
		SAppDimensionValue iconMarginLeft;
		SAppDimensionValue iconMarginTop;
		SAppDimensionValue iconMarginRight;
		SAppDimensionValue iconMarginBottom;
		SAppDimensionValue textMarginLeft;
		SAppDimensionValue textMarginTop;
		SAppDimensionValue textMarginRight;
		SAppDimensionValue textMarginBottom;
		SAppBooleanValue defaultColorFilter;
		SAppBooleanValue focusedColorFilter;

		SAppStateMap<SAppColorValue> textColor;
		SAppStateMap<SAppDrawableValue> icon;
		SAppStateMap<SAppColorValue> colorOverlay;
		SAppLayoutButtonCategory categories[4];

	public:
		sl_bool isNotRequiredNative(sl_bool flagCheckBackgroundColor);
	};

	class SAppLayoutLabelAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppStringValue text;
		SAppStringValue hyperText;
		SAppStateMap<SAppColorValue> textColor;
		SAppAlignmentValue gravity;
		SAppMultiLineModeValue multiLine;
		SAppEllipsizeModeValue ellipsize;
		SAppUint32Value lines;
		SAppBooleanValue linksInText;
		SAppColorValue linkColor;
		SAppColorValue lineColor;
		SAppBooleanValue mnemonic;
		SAppBooleanValue contextMenu;
	};

	class SAppLayoutLineAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppLayoutOrientationValue orientation;
		SAppDimensionFloatValue thickness;
		SAppColorValue lineColor;
		SAppPenStyleValue lineStyle;
		SAppAlignmentValue gravity;
	};

	class SAppLayoutCheckAttributes : public SAppLayoutButtonAttributes
	{
	public:
		SAppBooleanValue checked;
	};

	class SAppLayoutRadioAttributes : public SAppLayoutCheckAttributes
	{
	public:
		String group;
		SAppStringValue value;
	};

	class SAppLayoutEditAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppStringValue text;
		SAppAlignmentValue gravity;
		SAppColorValue textColor;
		SAppStringValue hintText;
		SAppAlignmentValue hintGravity;
		SAppColorValue hintTextColor;
		SAppFontValue hintFont;
		SAppBooleanValue readOnly;
		SAppBooleanValue password;
		SAppBooleanValue number;
		SAppBooleanValue lowercase;
		SAppBooleanValue uppercase;
		SAppMultiLineModeValue multiLine;
		SAppUIReturnKeyTypeValue returnKey;
		SAppUIKeyboardTypeValue keyboard;
		SAppUIAutoCapitalizationTypeValue autoCap;
		SAppBooleanValue focusNextOnReturnKey;
		SAppBooleanValue popup;
	};

	typedef SAppLayoutEditAttributes SAppLayoutPasswordAttributes;
	typedef SAppLayoutEditAttributes SAppLayoutTextAreaAttributes;

	class SAppLayoutImageAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppDrawableValue src;
		SAppScaleModeValue scale;
		SAppAlignmentValue gravity;
		SAppFloatValue minAspectRatio;
		SAppFloatValue maxAspectRatio;
		SAppStringValue url;
	};

	class SAppLayoutSelectItem
	{
	public:
		SAppStringValue title;
		SAppStringValue value;
		SAppBooleanValue selected;
	};

	class SAppLayoutSelectAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppColorValue textColor;
		SAppAlignmentValue gravity;

		CList<SAppLayoutSelectItem> items;

	public:
		sl_bool isNotRequiredNative(sl_bool flagCheckBackgroundColor);
	};

	class SAppLayoutSelectSwitchAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppColorValue textColor;
		SAppAlignmentValue gravity;

		CList<SAppLayoutSelectItem> items;

		SAppDimensionValue iconWidth;
		SAppDimensionValue iconHeight;
		SAppDrawableValue leftIcon;
		SAppDrawableValue rightIcon;
	};

	class SAppLayoutComboBoxAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppStringValue text;
		CList<SAppLayoutSelectItem> items;

	public:
		sl_bool isNotRequiredNative(sl_bool flagCheckBackgroundColor);
	};

	class SAppLayoutScrollAttributes : public SAppLayoutViewAttributes
	{
	public:
		Ref<SAppLayoutResourceItem> content;
	};

	class SAppLayoutLinearAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppLayoutOrientationValue orientation;
	};

	class SAppLayoutListAttributes : public SAppLayoutViewAttributes
	{
	public:
		String itemLayout;
	};

	class SAppLayoutCollectionAttributes : public SAppLayoutViewAttributes
	{
	public:
		String itemLayout;
	};

	class SAppLayoutTableColumn
	{
	public:
		SAppNameValue name;
		SAppDimensionValue width;
		SAppDimensionValue minWidth;
		SAppDimensionValue maxWidth;
		SAppDimensionValue margin;
		SAppDimensionValue marginLeft;
		SAppDimensionValue marginRight;
		SAppDimensionValue padding;
		SAppDimensionValue paddingLeft;
		SAppDimensionValue paddingRight;
		SAppDrawableValue background;
		SAppAlignmentValue align;
		SAppBooleanValue visible;
	};

	class SAppLayoutTableCell
	{
	public:
		Ref<SAppLayoutResourceItem> view;
		SAppUint32Value colspan;
		SAppUint32Value rowspan;
	};

	class SAppLayoutTableRow
	{
	public:
		SAppNameValue name;
		SAppDimensionValue height;
		SAppDimensionValue minHeight;
		SAppDimensionValue maxHeight;
		SAppDimensionValue margin;
		SAppDimensionValue marginTop;
		SAppDimensionValue marginBottom;
		SAppDimensionValue padding;
		SAppDimensionValue paddingTop;
		SAppDimensionValue paddingBottom;
		SAppDrawableValue background;
		SAppAlignmentValue align;
		SAppBooleanValue visible;

		List<SAppLayoutTableCell> cells;
	};

	class SAppLayoutTableAttributes : public SAppLayoutViewAttributes
	{
	public:
		CList<SAppLayoutTableColumn> columns;
		CList<SAppLayoutTableRow> rows;
		SAppBorderValue grid;
		SAppBorderValue horizontalGrid;
		SAppBorderValue verticalGrid;
	};

	struct SAppLayoutListControlColumn
	{
		SAppStringValue title;
		SAppDimensionValue width;
		SAppAlignmentValue align;
		SAppAlignmentValue headerAlign;
	};

	class SAppLayoutListControlAttributes : public SAppLayoutViewAttributes
	{
	public:
		CList<SAppLayoutListControlColumn> columns;
	};

	class SAppLayoutRenderAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppRedrawModeValue redraw;
	};

	struct SAppLayoutTabItem
	{
		SAppStringValue label;
		SAppDrawableValue icon;
		SAppBooleanValue selected;
		Ref<SAppLayoutResourceItem> view;
	};

	class SAppLayoutTabAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppLayoutOrientationValue orientation;
		SAppDimensionFloatValue tabWidth;
		SAppDimensionFloatValue tabHeight;
		SAppDrawableValue barBackground;
		SAppDrawableValue contentBackground;
		SAppStateMap<SAppDrawableValue> tabBackground;
		SAppStateMap<SAppColorValue> labelColor;
		SAppFontValue labelFont;
		SAppAlignmentValue tabAlign;
		SAppDimensionValue tabPaddingLeft;
		SAppDimensionValue tabPaddingTop;
		SAppDimensionValue tabPaddingRight;
		SAppDimensionValue tabPaddingBottom;
		SAppDimensionValue tabSpaceSize;
		SAppDimensionValue iconWidth;
		SAppDimensionValue iconHeight;

		CList<SAppLayoutTabItem> items;

	public:
		sl_bool isNotRequiredNative(sl_bool flagCheckBackgroundColor);
	};

	struct SAppLayoutTreeItemAttributes : public CRef
	{
		SAppStringValue id;
		SAppStringValue text;
		SAppFontValue font;
		SAppStateMap<SAppDrawableValue> background;
		SAppStateMap<SAppDrawableValue> icon;
		SAppStateMap<SAppDrawableValue> openedIcon;
		SAppStateMap<SAppDrawableValue> closedIcon;
		SAppStateMap<SAppColorValue> textColor;
		SAppDimensionValue iconSize;
		SAppDimensionValue iconWidth;
		SAppDimensionValue iconHeight;
		SAppDimensionValue height;
		SAppBooleanValue opened;
		SAppBooleanValue selected;
	};

	class SAppLayoutTreeAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppStateMap<SAppDrawableValue> itemIcon;
		SAppStateMap<SAppDrawableValue> openedIcon;
		SAppStateMap<SAppDrawableValue> closedIcon;
		SAppDrawableValue collapsedIcon;
		SAppDrawableValue expandedIcon;
		SAppStateMap<SAppDrawableValue> itemBackground;
		SAppStateMap<SAppColorValue> textColor;
		SAppDimensionValue itemIconSize;
		SAppDimensionValue itemIconWidth;
		SAppDimensionValue itemIconHeight;
		SAppDimensionValue itemHeight;
		SAppDimensionValue itemPadding;
		SAppDimensionValue itemIndent;
		SAppDimensionValue textIndent;
	};

	struct SAppLayoutSplitItem
	{
		Ref<SAppLayoutResourceItem> view;
		SAppFloatValue weight;
		SAppFloatValue minWeight;
		SAppFloatValue maxWeight;
		SAppDimensionValue minSize;
		SAppDimensionValue maxSize;
		SAppDimensionValue dividerWidth;
		SAppDrawableValue dividerBackground;
		SAppColorValue dividerColor;
	};

	class SAppLayoutSplitAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppLayoutOrientationValue orientation;
		SAppDimensionValue dividerWidth;
		SAppDrawableValue dividerBackground;
		SAppColorValue dividerColor;
		SAppDimensionValue cursorMargin;

		CList<SAppLayoutSplitItem> items;
	};

	class SAppLayoutWebAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppStringValue url;
		SAppStringValue html;
	};

	class SAppLayoutProgressAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppLayoutOrientationValue orientation;
		SAppFloatValue min;
		SAppFloatValue max;
		SAppFloatValue value;
		SAppFloatValue value2;
		SAppBooleanValue dual;
		SAppBooleanValue discrete;
		SAppFloatValue step;
		SAppBooleanValue reversed;
		SAppDrawableValue track;
		SAppDrawableValue progress;
		SAppDrawableValue progress2;
	};

	class SAppLayoutSliderAttributes : public SAppLayoutProgressAttributes
	{
	public:
		SAppStateMap<SAppDrawableValue> thumb;
		SAppDimensionValue thumbWidth;
		SAppDimensionValue thumbHeight;
	};

	class SAppLayoutSwitchAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppBooleanValue value;
		SAppBooleanValue textInButton;

		SAppStringValue text;
		SAppStringValue texts[2];
		SAppColorValue textColor;
		SAppColorValue textColors[2];

		SAppStateMap<SAppDrawableValue> thumb;
		SAppStateMap<SAppDrawableValue> thumbs[2];
		SAppStateMap<SAppDrawableValue> track;
		SAppStateMap<SAppDrawableValue> tracks[2];
	};

	class SAppLayoutPickerAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppColorValue textColor;
		SAppUint32Value lineCount;
		SAppBooleanValue circular;

		CList<SAppLayoutSelectItem> items;

	public:
		sl_bool isNotRequiredNative(sl_bool flagCheckBackgroundColor);
	};

	class SAppLayoutDatePickerAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppTimeValue date;

	public:
		sl_bool isNotRequiredNative(sl_bool flagCheckBackgroundColor);
	};

	struct SAppLayoutPagerItem
	{
		SAppBooleanValue selected;
		Ref<SAppLayoutResourceItem> view;
	};

	class SAppLayoutPagerAttributes : public SAppLayoutViewAttributes
	{
	public:
		CList<SAppLayoutPagerItem> items;

		SAppBooleanValue loop;
	};

	class SAppLayoutNavigationAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppBooleanValue swipe;
	};

	class SAppLayoutVideoAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppStringValue src;
		SAppBooleanValue repeat;
		SAppRotationModeValue rotation;
		SAppFlipModeValue flip;
		SAppScaleModeValue scale;
		SAppAlignmentValue gravity;
		SAppBooleanValue controls;
	};

	class SAppLayoutCameraAttributes : public SAppLayoutVideoAttributes
	{
	public:
		SAppStringValue device;
		SAppBooleanValue autoStart;
		SAppBooleanValue touchFocus;
	};

	class SAppLayoutDrawerAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppDimensionValue drawerSize;
		SAppDimensionValue dragEdgeSize;
		SAppAlignmentValue gravity;
	};

	class SAppLayoutRefreshAttributes : public SAppLayoutViewAttributes
	{
	public:
	};

	class SAppLayoutListBoxAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppUint64Value itemCount;
		SAppDimensionValue itemHeight;
		SAppBooleanValue multipleSelection;
		SAppStateMap<SAppDrawableValue> itemBackground;
	};

	class SAppLayoutLabelListAttributes : public SAppLayoutListBoxAttributes
	{
	public:
		SAppFloatValue lineHeightWeight;
		SAppStateMap<SAppColorValue> textColor;
		SAppAlignmentValue gravity;
		SAppEllipsizeModeValue ellipsize;
		SAppMultiLineModeValue multiLine;

		SAppDimensionValue itemPaddingLeft;
		SAppDimensionValue itemPaddingTop;
		SAppDimensionValue itemPaddingRight;
		SAppDimensionValue itemPaddingBottom;

		CList<SAppLayoutSelectItem> items;
	};

	class SAppLayoutTileLayoutAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppUint32Value columns;
		SAppUint32Value rows;
		SAppDimensionValue columnWidth;
		SAppDimensionValue rowHeight;
		SAppFloatValue cellRatio;
	};

	class SAppLayoutPdfAttributes : public SAppLayoutViewAttributes
	{
	public:
	};

	class SAppLayoutGroupBoxAttributes : public SAppLayoutViewAttributes
	{
	public:
		SAppStringValue label;
		SAppColorValue labelColor;
		SAppFontValue labelFont;
	};

	class SAppLayoutGridCellAttributes
	{
	public:
		enum class Creator {
			None,
			Text,
			HyperText,
			Numero,
			Sort,
			Icon,
			Button
		};
		Creator creator = Creator::None;

		SAppStringValue field;
		SAppStringValue text;
		SAppAlignmentValue align;
		SAppFontValue font;
		SAppCursorValue cursor;
		SAppStringValue toolTip;
		SAppDimensionValue padding;
		SAppDimensionValue paddingLeft;
		SAppDimensionValue paddingTop;
		SAppDimensionValue paddingRight;
		SAppDimensionValue paddingBottom;
		SAppMultiLineModeValue multiLine;
		SAppEllipsizeModeValue ellipsize;
		SAppUint32Value lineCount;
		SAppBooleanValue selectable;
		SAppBooleanValue editable;
		SAppBooleanValue defaultColorFilter;
		SAppDimensionValue iconWidth;
		SAppDimensionValue iconMargin;
		SAppDimensionValue iconMarginLeft;
		SAppDimensionValue iconMarginTop;
		SAppDimensionValue iconMarginRight;
		SAppDimensionValue iconMarginBottom;
		SAppScaleModeValue iconScale;
		SAppAlignmentValue iconAlign;
		SAppStateMap<SAppDrawableValue> background;
		SAppStateMap<SAppColorValue> textColor;
		SAppStateMap<SAppDrawableValue> icon;

		struct NumeroParam
		{
			SAppInt64Value start;
		} numero;
	};

	class SAppLayoutGridCell : public SAppLayoutGridCellAttributes
	{
	public:
		SAppUint32Value rowspan;
		SAppUint32Value colspan;
	};

	class SAppLayoutGridColumn : public SAppLayoutGridCellAttributes
	{
	public:
		SAppNameValue name;
		SAppDimensionValue width;
		SAppDimensionValue minWidth;
		SAppDimensionValue maxWidth;
		SAppBooleanValue fixed;
		SAppBooleanValue visible;
		SAppBooleanValue resizable;
		SAppBooleanValue verticalGrid;
		SAppBooleanValue bodyVerticalGrid;
		SAppBooleanValue headerVerticalGrid;
		SAppBooleanValue footerVerticalGrid;

		SAppLayoutGridCellAttributes bodyAttrs;
		SAppLayoutGridCellAttributes headerAttrs;
		SAppLayoutGridCellAttributes footerAttrs;
	};

	class SAppLayoutGridRow : public SAppLayoutGridCellAttributes
	{
	public:
		SAppNameValue name;
		SAppDimensionValue height;
		SAppBooleanValue visible;
		SAppBooleanValue horizontalGrid;

		List<SAppLayoutGridCell> cells;
	};

	class SAppLayoutGridSection : public SAppLayoutGridCellAttributes
	{
	public:
		CList<SAppLayoutGridRow> rows;
		SAppDimensionValue rowHeight;

		SAppBorderValue grid;
		SAppBooleanValue verticalGrid;
		SAppBooleanValue horizontalGrid;
	};

	class SAppLayoutGridAttributes : public SAppLayoutViewAttributes
	{
	public:
		CList<SAppLayoutGridColumn> columns;
		sl_uint32 nLeftColumns = 0;
		sl_uint32 nRightColumns = 0;

		SAppUint64Value recordCount;
		SAppDimensionValue columnWidth;
		SAppDimensionValue minColumnWidth;
		SAppDimensionValue maxColumnWidth;
		SAppBooleanValue resizableColumn;
		SAppDimensionValue rowHeight;
		SAppBorderValue grid;
		SAppBorderValue leftGrid;
		SAppBorderValue rightGrid;
		SAppGridSelectionModeValue selection;
		SAppBorderValue selectionBorder;
		SAppBooleanValue sort;
		SAppDrawableValue ascendingIcon;
		SAppDrawableValue descendingIcon;
		SAppDimensionValue sortIconSize;
		SAppBooleanValue verticalGrid;
		SAppBooleanValue horizontalGrid;

		SAppCursorValue cellCursor;
		SAppDimensionValue cellPadding;
		SAppDimensionValue cellPaddingLeft;
		SAppDimensionValue cellPaddingTop;
		SAppDimensionValue cellPaddingRight;
		SAppDimensionValue cellPaddingBottom;
		SAppMultiLineModeValue multiLine;
		SAppEllipsizeModeValue ellipsize;
		SAppUint32Value lineCount;
		SAppAlignmentValue cellAlign;
		SAppBooleanValue selectable;
		SAppBooleanValue editable;
		SAppBooleanValue defaultColorFilter;
		SAppDimensionValue iconWidth;
		SAppDimensionValue iconMargin;
		SAppDimensionValue iconMarginLeft;
		SAppDimensionValue iconMarginTop;
		SAppDimensionValue iconMarginRight;
		SAppDimensionValue iconMarginBottom;
		SAppScaleModeValue iconScale;
		SAppAlignmentValue iconAlign;
		SAppStateMap<SAppDrawableValue> cellBackground;
		SAppStateMap<SAppColorValue> textColor;

		SAppLayoutGridSection body;
		SAppLayoutGridSection header;
		SAppLayoutGridSection footer;
	};

	class SAppLayoutStyle : public CRef
	{
	public:
		Ref<XmlElement> element;
		String name;
		List< Ref<SAppLayoutStyle> > inherit;

	public:
		String getXmlAttribute(const String& name);
	};

	class SAppLayoutInclude : public CRef
	{
	public:
		Ref<XmlElement> element;
		String name;
	};

	enum class SAppLayoutType
	{
		Window = 0,
		Page = 1,
		View = 2
	};

	enum class SAppLayoutItemType
	{
		Unknown = 0,

		View = 0x0200,
		ViewGroup = 0x0201,
		Import = 0x0202,

		Button = 0x0210,
		Label = 0x0211,
		Check = 0x0212,
		Radio = 0x0213,
		Edit = 0x0214,
		Password = 0x0215,
		TextArea = 0x0216,
		Image = 0x0217,
		Select = 0x0218,

		Scroll = 0x0230,
		Linear = 0x0231,
		List = 0x0232,
		Collection = 0x0233,
		ListControl = 0x0234,
		Render = 0x0235,
		Tab = 0x0236,
		Tree = 0x0237,
		Web = 0x0239,
		Split = 0x023A,
		Table = 0x023B,
		ListBox = 0x023C,
		LabelList = 0x023D,
		TileLayout = 0x023E,
		GroupBox = 0x023F,

		Progress = 0x0260,
		Slider = 0x0261,
		Switch = 0x0262,
		Picker = 0x0264,
		Pager = 0x0265,
		Navigation = 0x0266,
		Video = 0x0267,
		Camera = 0x0268,
		Drawer = 0x026a,
		Refresh = 0x026c,
		DatePicker = 0x026d,
		Line = 0x026e,
		SelectSwitch = 0x026f,
		ComboBox = 0x0270,
		Pdf = 0x0271,
		Grid = 0x0272,

		NoView = 0xF000,
		TreeItem = 0xF001

	};

	class SAppLayoutXmlItem
	{
	public:
		Ref<XmlElement> element;
		List< Ref<SAppLayoutStyle> > styles;

	public:
		SAppLayoutXmlItem();
		~SAppLayoutXmlItem();
		SAppLayoutXmlItem(const Ref<XmlElement>& _element);

	public:
		void init();

		String getTagName()
		{
			return element->getName();
		}

		String getXmlText()
		{
			return element->getText();
		}

		String getXmlAttribute(const String& name);

		String getXmlAttributeWithoutStyle(const String& name);

		String getVariableValue(const String& name);

	private:
		String _getXmlAttribute(const String& name);

		String _resolveVariables(const String& name, const String& value);
	};

	class SAppLayoutResourceItem : public CRef, public SAppLayoutXmlItem
	{
	public:
		SAppLayoutItemType itemType;
		String itemTypeName;
		String name;
		sl_bool flagGeneratedName;
		String arrayName;
		sl_int32 arrayIndex;
		sl_bool flagSkipParseChildren;
		sl_bool flagSkipGenerateChildren;
		sl_bool flagSkipSimulateChildren;

		String className;

		Ref<CRef> attrs;
		CList< Ref<SAppLayoutResourceItem> > children;

	public:
		SAppLayoutResourceItem();

	};

	class SAppLayoutResource : public SAppLayoutResourceItem
	{
	public:
		String filePath;
		SAppLayoutType layoutType;

		String baseClassName;
		SAppDimensionFloatValue sp;

		CHashMap< String, Ref<SAppLayoutResourceItem> > itemsByName;
		CMap<String, sl_bool> customClasses;
		CMap<String, sl_bool> radioGroups;
		CMap<String, sl_bool> otherNames;
		struct ItemArrayDesc
		{
			String className;
			sl_uint32 itemCount;
		};
		CMap<String, ItemArrayDesc> itemArrays;

		sl_uint32 nAutoIncreaseNameView = 0;
		sl_uint32 nAutoIncreaseNameViewGroup = 0;
		sl_uint32 nAutoIncreaseNameImport = 0;
		sl_uint32 nAutoIncreaseNameButton = 0;
		sl_uint32 nAutoIncreaseNameLabel = 0;
		sl_uint32 nAutoIncreaseNameLine = 0;
		sl_uint32 nAutoIncreaseNameCheck = 0;
		sl_uint32 nAutoIncreaseNameRadio = 0;
		sl_uint32 nAutoIncreaseNameEdit = 0;
		sl_uint32 nAutoIncreaseNamePassword = 0;
		sl_uint32 nAutoIncreaseNameTextArea = 0;
		sl_uint32 nAutoIncreaseNameImage = 0;
		sl_uint32 nAutoIncreaseNameSelect = 0;
		sl_uint32 nAutoIncreaseNameSelectSwitch = 0;
		sl_uint32 nAutoIncreaseNameComboBox = 0;
		sl_uint32 nAutoIncreaseNameScroll = 0;
		sl_uint32 nAutoIncreaseNameLinear = 0;
		sl_uint32 nAutoIncreaseNameList = 0;
		sl_uint32 nAutoIncreaseNameCollection = 0;
		sl_uint32 nAutoIncreaseNameTable = 0;
		sl_uint32 nAutoIncreaseNameListControl = 0;
		sl_uint32 nAutoIncreaseNameRender = 0;
		sl_uint32 nAutoIncreaseNameTab = 0;
		sl_uint32 nAutoIncreaseNameTree = 0;
		sl_uint32 nAutoIncreaseNameTreeItem = 0;
		sl_uint32 nAutoIncreaseNameWeb = 0;
		sl_uint32 nAutoIncreaseNameSplit = 0;
		sl_uint32 nAutoIncreaseNameProgress = 0;
		sl_uint32 nAutoIncreaseNameSlider = 0;
		sl_uint32 nAutoIncreaseNameSwitch = 0;
		sl_uint32 nAutoIncreaseNamePicker = 0;
		sl_uint32 nAutoIncreaseNameDatePicker = 0;
		sl_uint32 nAutoIncreaseNamePager = 0;
		sl_uint32 nAutoIncreaseNameNavigation = 0;
		sl_uint32 nAutoIncreaseNameVideo = 0;
		sl_uint32 nAutoIncreaseNameCamera = 0;
		sl_uint32 nAutoIncreaseNameDrawer = 0;
		sl_uint32 nAutoIncreaseNameRefresh = 0;
		sl_uint32 nAutoIncreaseNameListBox = 0;
		sl_uint32 nAutoIncreaseNameLabelList = 0;
		sl_uint32 nAutoIncreaseNameTileLayout = 0;
		sl_uint32 nAutoIncreaseNamePdf = 0;
		sl_uint32 nAutoIncreaseNameGroupBox = 0;
		sl_uint32 nAutoIncreaseNameGrid = 0;

	public:
		SAppLayoutResource();

	public:
		String getAutoIncreasingName(SAppLayoutItemType type);

		static SAppLayoutItemType getTypeFromName(const String& strType);
	};


	class SAppLayoutSimulationParams
	{
	public:
		sl_ui_len screenWidth;
		sl_ui_len screenHeight;
		sl_ui_len viewportWidth;
		sl_ui_len viewportHeight;
		sl_real sp;
		sl_bool flagResizeScreen;

	public:
		SAppLayoutSimulationParams();
	};

	class SAppDocument;
	class SAppLayoutSimulationWindow;

	class SAppLayoutSimulator
	{
	public:
		Ref<CRef> getRef();

		Ref<CRef> getViewItemByName(const String& name);

		void registerViewItemByName(const String& name, const Ref<CRef>& item);

		Ref<RadioGroup> getRadioGroup(const String& name);

		Ref<SAppDocument> getDocument();

		Ref<SAppLayoutResource> getLayoutResource();

		Ref<SAppLayoutSimulationWindow> getSimulationWindow();

		Ref<View> getSimulationContentView();

	protected:
		AtomicWeakRef<CRef> m_refer;
		AtomicWeakRef<SAppDocument> m_document;
		AtomicRef<SAppLayoutResource> m_layoutResource;
		AtomicWeakRef<SAppLayoutSimulationWindow> m_simulationWindow;
		AtomicWeakRef<View> m_simulationContentView;
		CHashMap< String, Ref<CRef> > m_viewItems;
		CHashMap< String, Ref<RadioGroup> > m_radioGroups;
	};

	class SAppLayoutSimulationWindow : public WindowLayout, public SAppLayoutSimulator
	{
	public:
		SAppLayoutSimulationWindow();

	protected:
		void init() override;

	public:
		sl_bool open(SAppDocument* doc, SAppLayoutResource* layout);

	protected:
		void onResize(sl_ui_len width, sl_ui_len height) override;

		void layoutViews(sl_ui_len width, sl_ui_len height) override;

		void onDestroy() override;

	public:
		SLIB_BOOLEAN_PROPERTY(SavingPageSize)
	};

	class SAppLayoutImportView : public ViewLayout, public SAppLayoutSimulator
	{
	public:
		SAppLayoutImportView();

	protected:
		void init() override;

	public:
		void initialize(SAppLayoutSimulator* simulator, SAppLayoutResource* layout);
		void layoutViews(sl_ui_len width, sl_ui_len height) override;
	};

}

#endif
