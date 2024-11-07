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

#include "slib/ui.h"

#include "sapp.h"

namespace slib
{

	String SAppStringResourceItem::get(const Locale& locale, const String& def)
	{
		{
			for (auto&& item : values) {
				if (item.key == locale) {
					return item.value;
				}
			}
		}
		{
			Locale localeCompare = Locale(locale.getLanguage(), locale.getCountry());
			for (auto&& item : values) {
				if (item.key == localeCompare) {
					return item.value;
				}
			}
		}
		{
			Locale localeCompare(locale.getLanguage(), locale.getScript(), Country::Unknown);
			for (auto&& item : values) {
				if (item.key == localeCompare) {
					return item.value;
				}
			}
		}
		{
			Locale localeCompare(locale.getLanguage());
			for (auto&& item : values) {
				if (item.key == localeCompare) {
					return item.value;
				}
			}
		}
		return def;
	}

	Ref<Drawable> SAppDrawableResourceFileItem::load()
	{
		ObjectLocker lock(this);
		if (drawable.isNull()) {
			drawable = Drawable::loadFromFile(filePath);
		}
		return drawable;
	}

	SAppDrawableResourceFileAttributes::SAppDrawableResourceFileAttributes()
	{
		defaultFiles = List< Ref<SAppDrawableResourceFileItem> >::create();
	}


	SAppLayoutViewAttributes::SAppLayoutViewAttributes()
	{
	}

	void SAppLayoutViewAttributes::resetLayout()
	{
		width.flagDefined = sl_false;
		height.flagDefined = sl_false;

		leftMode = PositionMode::Free;
		topMode = PositionMode::Free;
		rightMode = PositionMode::Free;
		bottomMode = PositionMode::Free;

		left.flagDefined = sl_false;
		top.flagDefined = sl_false;

		minWidth.flagDefined = sl_false;
		maxWidth.flagDefined = sl_false;
		minHeight.flagDefined = sl_false;
		maxHeight.flagDefined = sl_false;

		aspectRatio.flagDefined = sl_false;

		marginLeft.flagDefined = sl_false;
		marginTop.flagDefined = sl_false;
		marginRight.flagDefined = sl_false;
		marginBottom.flagDefined = sl_false;
	}

	sl_bool SAppLayoutViewAttributes::isNotRequiredNative(sl_bool flagCheckBackgroundColor)
	{
		for (auto&& item : background.values) {
			if (item.key == ViewState::All) {
				if (flagCheckBackgroundColor) {
					return sl_true;
				} else {
					if (!(item.value.flagColor)) {
						return sl_true;
					}
				}
			} else {
				return sl_true;
			}
		}
		if (border.values.isNotNull()) {
			return sl_true;
		}
		return sl_false;
	}

	sl_bool SAppLayoutButtonAttributes::isNotRequiredNative(sl_bool flagCheckBackgroundColor)
	{
		if (SAppLayoutViewAttributes::isNotRequiredNative(flagCheckBackgroundColor)) {
			return sl_true;
		}
		if (iconWidth.flagDefined) {
			return sl_true;
		}
		if (iconHeight.flagDefined) {
			return sl_true;
		}
		if (iconAlign.flagDefined) {
			return sl_true;
		}
		if (textAlign.flagDefined) {
			return sl_true;
		}
		if (textBeforeIcon.flagDefined) {
			return sl_true;
		}
		if (orientation.flagDefined) {
			return sl_true;
		}
		if (iconMarginLeft.flagDefined) {
			return sl_true;
		}
		if (iconMarginTop.flagDefined) {
			return sl_true;
		}
		if (iconMarginRight.flagDefined) {
			return sl_true;
		}
		if (iconMarginBottom.flagDefined) {
			return sl_true;
		}
		if (textMarginLeft.flagDefined) {
			return sl_true;
		}
		if (textMarginTop.flagDefined) {
			return sl_true;
		}
		if (textMarginRight.flagDefined) {
			return sl_true;
		}
		if (textMarginBottom.flagDefined) {
			return sl_true;
		}
		if (defaultColorFilter.flagDefined) {
			return sl_true;
		}
		if (focusedColorFilter.flagDefined) {
			return sl_true;
		}
		if (textColor.values.isNotNull()) {
			return sl_true;
		}
		if (icon.values.isNotNull()) {
			return sl_true;
		}
		if (colorOverlay.values.isNotNull()) {
			return sl_true;
		}
		for (sl_size i = 0; i < CountOfArray(categories); i++) {
			SAppLayoutButtonCategory& category = categories[i];
			if (category.textColor.values.isNotNull()) {
				return sl_true;
			}
			if (category.icon.values.isNotNull()) {
				return sl_true;
			}
			if (category.background.values.isNotNull()) {
				return sl_true;
			}
			if (category.border.values.isNotNull()) {
				return sl_true;
			}
			if (category.colorOverlay.values.isNotNull()) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool SAppLayoutTabAttributes::isNotRequiredNative(sl_bool flagCheckBackgroundColor)
	{
		if (SAppLayoutViewAttributes::isNotRequiredNative(flagCheckBackgroundColor)) {
			return sl_true;
		}
		if (orientation.flagDefined) {
			return sl_true;
		}
		if (tabWidth.flagDefined) {
			return sl_true;
		}
		if (tabWidth.flagDefined) {
			return sl_true;
		}
		if (barBackground.flagDefined) {
			return sl_true;
		}
		if (contentBackground.flagDefined) {
			return sl_true;
		}
		if (tabBackground.values.isNotNull()) {
			return sl_true;
		}
		if (labelColor.values.isNotNull()) {
			return sl_true;
		}
		if (tabAlign.flagDefined) {
			return sl_true;
		}
		if (tabPaddingLeft.flagDefined) {
			return sl_true;
		}
		if (tabPaddingTop.flagDefined) {
			return sl_true;
		}
		if (tabPaddingRight.flagDefined) {
			return sl_true;
		}
		if (tabPaddingBottom.flagDefined) {
			return sl_true;
		}
		if (tabSpaceSize.flagDefined) {
			return sl_true;
		}
		if (iconWidth.flagDefined) {
			return sl_true;
		}
		if (iconHeight.flagDefined) {
			return sl_true;
		}
		return sl_false;
	}

	sl_bool SAppLayoutSelectAttributes::isNotRequiredNative(sl_bool flagCheckBackgroundColor)
	{
		return SAppLayoutViewAttributes::isNotRequiredNative(flagCheckBackgroundColor);
	}

	sl_bool SAppLayoutComboBoxAttributes::isNotRequiredNative(sl_bool flagCheckBackgroundColor)
	{
		return SAppLayoutViewAttributes::isNotRequiredNative(flagCheckBackgroundColor);
	}

	sl_bool SAppLayoutPickerAttributes::isNotRequiredNative(sl_bool flagCheckBackgroundColor)
	{
		if (SAppLayoutViewAttributes::isNotRequiredNative(flagCheckBackgroundColor)) {
			return sl_true;
		}
		if (textColor.flagDefined) {
			return sl_true;
		}
		return sl_false;
	}

	sl_bool SAppLayoutDatePickerAttributes::isNotRequiredNative(sl_bool flagCheckBackgroundColor)
	{
		return SAppLayoutViewAttributes::isNotRequiredNative(flagCheckBackgroundColor);
	}

	String SAppLayoutStyledElement::getXmlAttribute(const String& name)
	{
		String value = element->getAttribute(name);
		if (value.isNotNull()) {
			return value;
		}
		ListLocker< Ref<SAppLayoutStyle> > _styles(styles);
		for (sl_size i = 0; i < _styles.count; i++) {
			Ref<SAppLayoutStyle> style = _styles[_styles.count - 1 - i];
			if (style.isNotNull()) {
				String value = style->getXmlAttribute(name);
				if (value.isNotNull()) {
					return value;
				}
			}
		}
		return sl_null;
	}

	void SAppLayoutStyledElement::getEventMapping(const StringView& prefix, HashMap<String, String>& mapping)
	{
		ListLocker< Ref<SAppLayoutStyle> > _styles(styles);
		for (sl_size i = 0; i < _styles.count; i++) {
			Ref<SAppLayoutStyle> style = _styles[i];
			if (style.isNotNull()) {
				style->getEventMapping(mapping);
			}
		}
		sl_size n = element->getAttributeCount();
		for (sl_size i = 0; i < n; i++) {
			XmlAttribute attr;
			if (element->getAttribute(i, &attr)) {
				if (attr.name.startsWith(prefix)) {
					mapping.put_NoLock(attr.name.substring(prefix.getLength()), attr.value);
				}
			}
		}
	}

	void SAppLayoutStyledElement::getEventMapping(HashMap<String, String>& mapping)
	{
		getEventMapping(StringView::literal("on"), mapping);
	}


	SAppLayoutXmlItem::SAppLayoutXmlItem(const Ref<XmlElement>& _element)
	{
		element = _element;
	}

	String SAppLayoutXmlItem::getXmlAttribute(const String& name)
	{
		sl_bool flagElementValue = sl_false;
		String value = element->getAttribute(name);
		if (value.isNotNull()) {
			value = _resolveVariables(name, value);
			if (value != StringView::literal("@inherit")) {
				return value;
			}
		}
		{
			ListLocker< Ref<SAppLayoutStyle> > _styles(styles);
			for (sl_size i = 0; i < _styles.count; i++) {
				Ref<SAppLayoutStyle> style = _styles[_styles.count - 1 - i];
				if (style.isNotNull()) {
					String value = style->getXmlAttribute(name);
					if (value.isNotNull()) {
						value = _resolveVariables(name, value);
						if (value != StringView::literal("@inherit")) {
							return value;
						}
					}
				}
			}
		}
		return _resolveDefaultValue(name);
	}

	String SAppLayoutXmlItem::getXmlAttributeWithoutStyle(const String& name)
	{
		return _resolveVariables(name, element->getAttribute(name));
	}

	String SAppLayoutXmlItem::getVariableValue(const String& name)
	{
		String vName = StringView::literal(":") + name;
		Ref<XmlElement> e = element;
		do {
			Ref<CRef> refStyles = e->getProperty(sStyles).getRef();
			if (refStyles.isNotNull()) {
				SAppLayoutXmlItem item(e);
				item.styles = Ref< CList< Ref<SAppLayoutStyle> > >::cast(refStyles);
				String value = item.getXmlAttribute(vName);
				if (value.isNotNull()) {
					return value;
				}
			} else {
				String value = e->getAttribute(vName);
				if (value.isNotNull()) {
					return value;
				}
			}
			RefT<SAppLayoutXmlItem> caller = RefT<SAppLayoutXmlItem>::cast(e->getProperty(sCaller).getRef());
			if (caller.isNotNull()) {
				String value = caller->getXmlAttribute(name);
				if (value.isNotNull()) {
					return value;
				}
				return caller->getVariableValue(name);
			}
			e = e->getParentElement();
		} while (e.isNotNull());
		return sl_null;
	}

	String SAppLayoutXmlItem::_resolveVariables(const String& name, const String& value)
	{
		sl_size len = value.getLength();
		if (!len) {
			if (value.isNull()) {
				return _resolveDefaultValue(name);
			}
			return value;
		}
		if (len == 1) {
			return value;
		}
		sl_char8* data = value.getData();
		if (*data == '$') {
			String attr(data + 1, len - 1);
			for (sl_size i = 0; i < 100; i++) {
				String value = getXmlAttribute(attr);
				if (value.isNotNull()) {
					return value;
				}
				if (value.startsWith('$')) {
					attr = value.substring(1);
				} else {
					break;
				}
			}
		}
		sl_char8* s = data;
		sl_char8* e = s + len;
		sl_char8* p = s;
		StringBuffer buf;
		while (p < e) {
			if (*p == ':') {
				sl_char8* t = p;
				p++;
				if (p >= e) {
					break;
				}
				sl_char8 ch = *p;
				if (ch == '\\') {
					// Ignore this char
					buf.addStatic(s, p - s);
					p++;
					s = p;
				} else if (ch == ':') {
					p++;
				} else if (ch == '{') {
					if (t > s) {
						buf.addStatic(s, t - s);
					}
					p++;
					sl_char8* n = p;
					sl_char8* m = e;
					while (n < e) {
						ch = *n;
						if (ch == '}') {
							if (m == e) {
								m = n;
							}
							break;
						} else {
							if (m == e && ch == '#') {
								m = n;
							}
							n++;
						}
					}
					if (n >= e) {
						break;
					}
					String var = getVariableValue(String(p, m - p));
					if (var.isNotNull()) {
						buf.add(var);
					} else {
						m++;
						if (m < n) {
							buf.add(String(m, n - m));
						}
					}
					p = n + 1;
					s = p;
				} else if (SLIB_CHAR_IS_ALPHA(ch) || ch == '_') {
					if (t > s) {
						buf.addStatic(s, t - s);
					}
					sl_char8* n = p + 1;
					while (n < e) {
						ch = *n;
						if (SLIB_CHAR_IS_ALNUM(ch) || ch == '_') {
							n++;
						} else {
							break;
						}
					}
					String var = getVariableValue(String(p, n - p));
					if (var.isNotNull()) {
						buf.add(var);
					}
					p = n;
					s = p;
				}
			} else {
				p++;
			}
		}
		if (s == data) {
			return value;
		}
		if (e > s) {
			buf.addStatic(s, e - s);
		}
		return buf.merge();
	}

	String SAppLayoutXmlItem::_resolveDefaultValue(const String& name)
	{
		if (!(name.startsWith(':')) && element->getProperty(sInherit).getBoolean()) {
			RefT<SAppLayoutXmlItem> caller = RefT<SAppLayoutXmlItem>::cast(element->getProperty(sCaller).getRef());
			if (caller.isNotNull()) {
				return caller->getXmlAttribute(name);
			}
		}
		return sl_null;
	}


	SAppLayoutResourceItem::SAppLayoutResourceItem(const Ref<XmlElement>& _element): SAppLayoutXmlItem(_element)
	{
		arrayIndex = -1;
		itemType = SAppLayoutItemType::Unknown;
		flagGeneratedName = sl_false;
		flagSkipParseChildren = sl_false;
		flagSkipGenerateChildren = sl_false;
		flagSkipSimulateChildren = sl_false;
	}

	SAppLayoutResource::SAppLayoutResource(const Ref<XmlElement>& _element): SAppLayoutResourceItem(_element)
	{
		layoutType = SAppLayoutType::View;
		itemType = SAppLayoutItemType::ViewGroup;
	}

	String SAppLayoutResource::getAutoIncreasingName(SAppLayoutItemType type)
	{
		String prefix;
		sl_uint32* pN;
		switch (type) {
			case SAppLayoutItemType::Unknown:
				return String::null();
			case SAppLayoutItemType::View:
				prefix = "view";
				pN = &nAutoIncreaseNameView;
				break;
			case SAppLayoutItemType::ViewGroup:
				prefix = "group";
				pN = &nAutoIncreaseNameViewGroup;
				break;
			case SAppLayoutItemType::Import:
				prefix = "import";
				pN = &nAutoIncreaseNameImport;
				break;
			case SAppLayoutItemType::Button:
				prefix = "button";
				pN = &nAutoIncreaseNameButton;
				break;
			case SAppLayoutItemType::Label:
				prefix = "label";
				pN = &nAutoIncreaseNameLabel;
				break;
			case SAppLayoutItemType::Line:
				prefix = "line";
				pN = &nAutoIncreaseNameLine;
				break;
			case SAppLayoutItemType::Check:
				prefix = "check";
				pN = &nAutoIncreaseNameCheck;
				break;
			case SAppLayoutItemType::Radio:
				prefix = "radio";
				pN = &nAutoIncreaseNameRadio;
				break;
			case SAppLayoutItemType::Edit:
				prefix = "edit";
				pN = &nAutoIncreaseNameEdit;
				break;
			case SAppLayoutItemType::Password:
				prefix = "password";
				pN = &nAutoIncreaseNamePassword;
				break;
			case SAppLayoutItemType::TextArea:
				prefix = "textArea";
				pN = &nAutoIncreaseNameTextArea;
				break;
			case SAppLayoutItemType::Image:
				prefix = "image";
				pN = &nAutoIncreaseNameImage;
				break;
			case SAppLayoutItemType::Select:
				prefix = "select";
				pN = &nAutoIncreaseNameSelect;
				break;
			case SAppLayoutItemType::SelectSwitch:
				prefix = "selectSwitch";
				pN = &nAutoIncreaseNameSelectSwitch;
				break;
			case SAppLayoutItemType::ComboBox:
				prefix = "comboBox";
				pN = &nAutoIncreaseNameComboBox;
				break;
			case SAppLayoutItemType::Scroll:
				prefix = "scroll";
				pN = &nAutoIncreaseNameScroll;
				break;
			case SAppLayoutItemType::Linear:
			case SAppLayoutItemType::LinearIterate:
				prefix = "linear";
				pN = &nAutoIncreaseNameLinear;
				break;
			case SAppLayoutItemType::TileLayout:
			case SAppLayoutItemType::TileIterate:
				prefix = "tileLayout";
				pN = &nAutoIncreaseNameTileLayout;
				break;
			case SAppLayoutItemType::List:
				prefix = "list";
				pN = &nAutoIncreaseNameList;
				break;
			case SAppLayoutItemType::Collection:
				prefix = "collection";
				pN = &nAutoIncreaseNameCollection;
				break;
			case SAppLayoutItemType::Table:
				prefix = "table";
				pN = &nAutoIncreaseNameTable;
				break;
			case SAppLayoutItemType::ListControl:
				prefix = "listControl";
				pN = &nAutoIncreaseNameListControl;
				break;
			case SAppLayoutItemType::Render:
				prefix = "render";
				pN = &nAutoIncreaseNameRender;
				break;
			case SAppLayoutItemType::Tab:
				prefix = "tab";
				pN = &nAutoIncreaseNameTab;
				break;
			case SAppLayoutItemType::Tree:
				prefix = "tree";
				pN = &nAutoIncreaseNameTree;
				break;
			case SAppLayoutItemType::TreeItem:
				prefix = "treeItem";
				pN = &nAutoIncreaseNameTreeItem;
				break;
			case SAppLayoutItemType::Web:
				prefix = "web";
				pN = &nAutoIncreaseNameWeb;
				break;
			case SAppLayoutItemType::Split:
				prefix = "split";
				pN = &nAutoIncreaseNameSplit;
				break;
			case SAppLayoutItemType::Progress:
				prefix = "progress";
				pN = &nAutoIncreaseNameProgress;
				break;
			case SAppLayoutItemType::Slider:
				prefix = "slider";
				pN = &nAutoIncreaseNameSlider;
				break;
			case SAppLayoutItemType::Switch:
				prefix = "switch";
				pN = &nAutoIncreaseNameSwitch;
				break;
			case SAppLayoutItemType::Picker:
				prefix = "picker";
				pN = &nAutoIncreaseNamePicker;
				break;
			case SAppLayoutItemType::DatePicker:
				prefix = "datePicker";
				pN = &nAutoIncreaseNameDatePicker;
				break;
			case SAppLayoutItemType::Pager:
				prefix = "pager";
				pN = &nAutoIncreaseNamePager;
				break;
			case SAppLayoutItemType::Navigation:
				prefix = "navigation";
				pN = &nAutoIncreaseNameNavigation;
				break;
			case SAppLayoutItemType::Audio:
				prefix = "audio";
				pN = &nAutoIncreaseNameAudio;
				break;
			case SAppLayoutItemType::Video:
				prefix = "video";
				pN = &nAutoIncreaseNameVideo;
				break;
			case SAppLayoutItemType::Camera:
				prefix = "camera";
				pN = &nAutoIncreaseNameCamera;
				break;
			case SAppLayoutItemType::Drawer:
				prefix = "drawer";
				pN = &nAutoIncreaseNameDrawer;
				break;
			case SAppLayoutItemType::Refresh:
				prefix = "refresh";
				pN = &nAutoIncreaseNameRefresh;
				break;
			case SAppLayoutItemType::ListBox:
				prefix = "listBox";
				pN = &nAutoIncreaseNameListBox;
				break;
			case SAppLayoutItemType::LabelList:
				prefix = "labelList";
				pN = &nAutoIncreaseNameLabelList;
				break;
			case SAppLayoutItemType::Pdf:
				prefix = "tileLayout";
				pN = &nAutoIncreaseNamePdf;
				break;
			case SAppLayoutItemType::GroupBox:
				prefix = "groupBox";
				pN = &nAutoIncreaseNameGroupBox;
				break;
			case SAppLayoutItemType::Map:
				prefix = "map";
				pN = &nAutoIncreaseNameMap;
				break;
			case SAppLayoutItemType::Grid:
				prefix = "grid";
				pN = &nAutoIncreaseNameGrid;
				break;
			default:
				return String::null();
		}
		for (;;) {
			(*pN)++;
			String name = String::format(StringView::literal("_%s%d"), prefix, *pN);
			if (!(itemsByName.find(name))) {
				return name;
			}
		}
		return String::null();
	}

	SAppLayoutItemType SAppLayoutResource::getTypeFromName(const String& _strType)
	{
		String strType = _strType.toLower();
		SAppLayoutItemType type = SAppLayoutItemType::Unknown;
		if (strType == "view") {
			type = SAppLayoutItemType::View;
		} else if (strType == "group") {
			type = SAppLayoutItemType::ViewGroup;
		} else if (strType == "import") {
			type = SAppLayoutItemType::Import;
		} else if (strType == "button") {
			type = SAppLayoutItemType::Button;
		} else if (strType == "label") {
			type = SAppLayoutItemType::Label;
		} else if (strType == "line" || strType == "hline" || strType == "vline") {
			type = SAppLayoutItemType::Line;
		} else if (strType == "check") {
			type = SAppLayoutItemType::Check;
		} else if (strType == "radio") {
			type = SAppLayoutItemType::Radio;
		} else if (strType == "edit") {
			type = SAppLayoutItemType::Edit;
		} else if (strType == "password") {
			type = SAppLayoutItemType::Password;
		} else if (strType == "textarea" || strType == "text-area") {
			type = SAppLayoutItemType::TextArea;
		} else if (strType == "image") {
			type = SAppLayoutItemType::Image;
		} else if (strType == "select") {
			type = SAppLayoutItemType::Select;
		} else if (strType == "selectswitch" || strType == "select-switch") {
			type = SAppLayoutItemType::SelectSwitch;
		} else if (strType == "combobox" || strType == "combo-box") {
			type = SAppLayoutItemType::ComboBox;
		} else if (strType == "scroll" || strType == "hscroll" || strType == "vscroll") {
			type = SAppLayoutItemType::Scroll;
		} else if (strType == "linear" || strType == "hlinear" || strType == "vlinear") {
			type = SAppLayoutItemType::Linear;
		} else if (strType == "linear-iterate" || strType == "hlinear-iterate" || strType == "vlinear-iterate") {
			type = SAppLayoutItemType::LinearIterate;
		} else if (strType == "tile") {
			type = SAppLayoutItemType::TileLayout;
		} else if (strType == "tile-iterate") {
			type = SAppLayoutItemType::TileIterate;
		} else if (strType == "list") {
			type = SAppLayoutItemType::List;
		} else if (strType == "collection") {
			type = SAppLayoutItemType::Collection;
		} else if (strType == "table") {
			type = SAppLayoutItemType::Table;
		} else if (strType == "listcontrol" || strType == "list-control") {
			type = SAppLayoutItemType::ListControl;
		} else if (strType == "render") {
			type = SAppLayoutItemType::Render;
		} else if (strType == "tab") {
			type = SAppLayoutItemType::Tab;
		} else if (strType == "tree") {
			type = SAppLayoutItemType::Tree;
		} else if (strType == "web") {
			type = SAppLayoutItemType::Web;
		} else if (strType == "split" || strType == "hsplit" || strType == "vsplit") {
			type = SAppLayoutItemType::Split;
		} else if (strType == "progress") {
			type = SAppLayoutItemType::Progress;
		} else if (strType == "slider") {
			type = SAppLayoutItemType::Slider;
		} else if (strType == "switch") {
			type = SAppLayoutItemType::Switch;
		} else if (strType == "picker") {
			type = SAppLayoutItemType::Picker;
		} else if (strType == "datepicker" || strType == "date-picker") {
			type = SAppLayoutItemType::DatePicker;
		} else if (strType == "pager") {
			type = SAppLayoutItemType::Pager;
		} else if (strType == "navigation") {
			type = SAppLayoutItemType::Navigation;
		} else if (strType == "audio") {
			type = SAppLayoutItemType::Audio;
		} else if (strType == "video") {
			type = SAppLayoutItemType::Video;
		} else if (strType == "camera") {
			type = SAppLayoutItemType::Camera;
		} else if (strType == "drawer") {
			type = SAppLayoutItemType::Drawer;
		} else if (strType == "refresh") {
			type = SAppLayoutItemType::Refresh;
		} else if (strType == "listbox" || strType == "list-box") {
			type = SAppLayoutItemType::ListBox;
		} else if (strType == "labellist" || strType == "label-list") {
			type = SAppLayoutItemType::LabelList;
		} else if (strType == "pdf") {
			type = SAppLayoutItemType::Pdf;
		} else if (strType == "groupbox" || strType == "group-box") {
			type = SAppLayoutItemType::GroupBox;
		} else if (strType == "map") {
			type = SAppLayoutItemType::Map;
		} else if (strType == "grid") {
			type = SAppLayoutItemType::Grid;
		}
		return type;
	}


	SAppLayoutSimulationParams::SAppLayoutSimulationParams()
	{
		screenWidth = 0;
		screenHeight = 0;
		viewportWidth = 0;
		viewportHeight = 0;
		sp = 1;
		flagResizeScreen = sl_false;
	}


	Ref<CRef> SAppLayoutSimulator::getRef()
	{
		return m_refer;
	}

	Ref<CRef> SAppLayoutSimulator::getViewItemByName(const String& name)
	{
		return m_viewItems.getValue(name, Ref<View>::null());
	}

	void SAppLayoutSimulator::registerViewItemByName(const String& name, const Ref<CRef>& item)
	{
		m_viewItems.put(name, item);
	}

	Ref<RadioGroup> SAppLayoutSimulator::getRadioGroup(const String& name)
	{
		return m_radioGroups.getValue(name, Ref<RadioGroup>::null());
	}

	Ref<SAppDocument> SAppLayoutSimulator::getDocument()
	{
		return m_document;
	}

	Ref<SAppLayoutResource> SAppLayoutSimulator::getLayoutResource()
	{
		return m_layoutResource;
	}

	Ref<SAppLayoutSimulationWindow> SAppLayoutSimulator::getSimulationWindow()
	{
		return m_simulationWindow;
	}

	Ref<View> SAppLayoutSimulator::getSimulationContentView()
	{
		return m_simulationContentView;
	}

	namespace
	{
		class UILayoutResourceHelper : public UILayoutResource
		{
		public:
			using UILayoutResource::setInitialized;
		};
	}

	sl_bool SAppLayoutSimulator::doInitialize(SAppLayoutSimulationWindow* window, UILayoutResource* res, SAppDocument* doc, SAppLayoutResource* layout, View* contentView)
	{
		m_document = doc;
		m_layoutResource = layout;
		m_simulationContentView = contentView;
		{
			ListElements<String> radioGroups(layout->radioGroups.getAllKeys());
			for (sl_size i = 0; i < radioGroups.count; i++) {
				Ref<RadioGroup> group = new RadioGroup;
				if (group.isNotNull()) {
					m_radioGroups.put(radioGroups[i], group);
				}
			}
		}
		Ref<View> viewContent = CastRef<View>(doc->_simulateLayoutCreateOrLayoutItem(this, layout, sl_null, sl_null, SAppLayoutOperation::SimulateInit));
		((UILayoutResourceHelper*)res)->setInitialized();
		if (viewContent.isNotNull()) {
			if (window) {
				if (layout->layoutType != SAppLayoutType::Window) {
					if (viewContent->getBackground().isNull()) {
						viewContent->setBackgroundColor(Color::White, UIUpdateMode::Init);
					}
					window->setBackgroundColor(Color::Black);
					window->addView(viewContent);
				}
				doc->_registerLayoutSimulationWindow(window);
			}
			doc->_simulateLayoutCreateOrLayoutItem(this, layout, sl_null, sl_null, SAppLayoutOperation::SimulateLayout);
			return sl_true;
		}
		return sl_false;
	}

	void SAppLayoutSimulator::doLayoutViews(sl_ui_len width, sl_ui_len height)
	{
		Ref<SAppDocument> doc = m_document;
		Ref<SAppLayoutResource> layout = m_layoutResource;
		if (doc.isNotNull() && layout.isNotNull()) {
			doc->_simulateLayoutCreateOrLayoutItem(this, layout.get(), sl_null, sl_null, SAppLayoutOperation::SimulateLayout);
		}
	}


	SAppLayoutSimulationWindow::SAppLayoutSimulationWindow()
	{
		setSavingPageSize(sl_true);
	}

	void SAppLayoutSimulationWindow::init()
	{
		WindowLayout::init();
		m_simulationWindow = this;
		m_refer = this;
	}

	sl_bool SAppLayoutSimulationWindow::open(SAppDocument* doc, SAppLayoutResource* layout)
	{
		Ref<View> viewContent;
		if (layout->layoutType == SAppLayoutType::Window) {
			viewContent = getContentView();
		} else {
			setCenterScreen(sl_true);
			setResizable(sl_true);
			viewContent = new ViewGroup;
		}
		if (doInitialize(this, this, doc, layout, viewContent.get())) {
			create();
			return sl_true;
		}
		return sl_false;
	}

	void SAppLayoutSimulationWindow::onResize(sl_ui_len width, sl_ui_len height)
	{
		WindowLayout::onResize(width, height);
		layoutViews(width, height);
	}

	void SAppLayoutSimulationWindow::layoutViews(sl_ui_len width, sl_ui_len height)
	{
		doLayoutViews(width, height);
	}

	void SAppLayoutSimulationWindow::onDestroy()
	{
		Ref<SAppDocument> doc = m_document;
		if (doc.isNotNull()) {
			doc->_removeLayoutSimulationWindow(this);
		}
	}


	SAppLayoutImportView::SAppLayoutImportView()
	{
		setImportView(sl_true);
	}

	void SAppLayoutImportView::init()
	{
		ViewLayout::init();
		m_refer = this;
	}

	void SAppLayoutImportView::initialize(SAppLayoutSimulator* simulator, SAppLayoutResource* layout)
	{
		Ref<SAppDocument> document = simulator->getDocument();
		m_simulationWindow = simulator->getSimulationWindow();
		doInitialize(sl_null, this, document.get(), layout, this);
	}

	void SAppLayoutImportView::layoutViews(sl_ui_len width, sl_ui_len height)
	{
		doLayoutViews(width, height);
	}


	SAppLayoutImportPage::SAppLayoutImportPage()
	{
		setImportView(sl_true);
	}

	void SAppLayoutImportPage::init()
	{
		PageLayout::init();
		m_refer = this;
	}

	void SAppLayoutImportPage::initialize(SAppLayoutSimulator* simulator, SAppLayoutResource* layout)
	{
		Ref<SAppDocument> document = simulator->getDocument();
		m_simulationWindow = simulator->getSimulationWindow();
		doInitialize(sl_null, this, document.get(), layout, this);
	}

	void SAppLayoutImportPage::layoutViews(sl_ui_len width, sl_ui_len height)
	{
		doLayoutViews(width, height);
	}

}
