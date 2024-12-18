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

namespace slib
{

	sl_bool SAppDocument::_parseLayoutStyle(const String& fileNamespace, const String& parentTheme, const Ref<XmlElement>& element)
	{
		if (element.isNull()) {
			return sl_false;
		}

		Ref<SAppLayoutStyle> style = new SAppLayoutStyle;
		if (style.isNull()) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		style->element = element;

		String theme = element->getAttribute(sTheme).trim();
		if (theme.isEmpty()) {
			theme = parentTheme;
		}
		String name = element->getAttribute(sName).trim();
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_layout_name_is_empty);
			return sl_false;
		}
		name = getGlobalStyleName(fileNamespace, theme, name);
		style->name = name;

		sl_bool flagOverride = element->getAttribute(sOverride).equals_IgnoreCase(StringView::literal("true"));
		if (!flagOverride) {
			if (m_layoutStyles.find(name)) {
				logError(element, g_str_error_resource_layout_name_redefined, name);
				return sl_false;
			}
		}

		String strInherit = element->getAttribute(sInherit).trim();
		if (strInherit.isNotEmpty()) {
			ListElements<String> arr(strInherit.split(','));
			for (sl_size i = 0; i < arr.count; i++) {
				String s = arr[i].trim();
				Ref<SAppLayoutStyle> inheritStyle = _lookupLayoutStyle(fileNamespace, theme, s);
				if (inheritStyle.isNotNull()) {
					if (!(style->styles.add_NoLock(Move(inheritStyle)))) {
						logError(element, g_str_error_out_of_memory);
						return sl_false;
					}
				} else {
					logError(element, g_str_error_layout_style_not_found, s);
					return sl_false;
				}
			}
		}

		if (!(m_layoutStyles.put(name, style))) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		return sl_true;
	}

	sl_bool SAppDocument::_parseLayoutInclude(const String& fileNamespace, const Ref<XmlElement>& element)
	{
		if (element.isNull()) {
			return sl_false;
		}

		Ref<SAppLayoutInclude> include = new SAppLayoutInclude;
		if (include.isNull()) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		include->element = element;

		String name = element->getAttribute(sName).trim();
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_layout_name_is_empty);
			return sl_false;
		}

		name = getGlobalName(fileNamespace, name);

		sl_bool flagOverride = element->getAttribute(sOverride).equals_IgnoreCase(StringView::literal("true"));
		if (!flagOverride) {
			if (m_layoutIncludes.find(name)) {
				logError(element, g_str_error_resource_layout_name_redefined, name);
				return sl_false;
			}
		}
		include->name = name;

		if (!(m_layoutIncludes.put(name, include))) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		return sl_true;
	}

	sl_bool SAppDocument::_parseLayoutUnit(const String& fileNamespace, const Ref<XmlElement>& element)
	{
		if (element.isNull()) {
			return sl_false;
		}

		String name = element->getAttribute(sName).trim();
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_layout_name_is_empty);
			return sl_false;
		}

		name = getGlobalName(fileNamespace, name);

		sl_bool flagOverride = element->getAttribute(sOverride).equals_IgnoreCase(StringView::literal("true"));
		if (!flagOverride) {
			if (m_layoutUnits.find(name)) {
				logError(element, g_str_error_resource_layout_name_redefined, name);
				return sl_false;
			}
		}

		String strValue = element->getText();

		SAppDimensionValue value;
		if (!(value.parse(strValue, this))) {
			logError(element, g_str_error_resource_layout_value_invalid, strValue);
			return sl_false;
		}

		if (!(m_layoutUnits.put(name, value))) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		return sl_true;
	}

	Ref<SAppLayoutResource> SAppDocument::_parseLayoutResource(const String& filePath, const String& fileNamespace, const Ref<XmlElement>& element, const String16& source, SAppLayoutResource* parent, String* outChildLayoutName, sl_bool* outFlagGeneratedName)
	{
		if (outFlagGeneratedName) {
			*outFlagGeneratedName = sl_false;
		}
		if (element.isNull()) {
			return sl_null;
		}
		Ref<SAppLayoutResource> layout = new SAppLayoutResource(element);
		if (layout.isNull()) {
			logError(element, g_str_error_out_of_memory);
			return sl_null;
		}
		layout->filePath = filePath;
		String name = element->getAttribute(sName).trim();
		if (name.isEmpty()) {
			if (outChildLayoutName) {
				parent->nAutoIncreaseNameChildLayout++;
				name = String::format(StringView::literal("GeneratedLayout%d"), parent->nAutoIncreaseNameChildLayout);
				*outChildLayoutName = name;
				name = getGlobalName(fileNamespace, name);
				if (outFlagGeneratedName) {
					*outFlagGeneratedName = sl_true;
				}
			} else {
				if (parent) {
					logError(element, g_str_error_resource_layout_name_is_empty, sName);
					return sl_null;
				} else {
					name = fileNamespace;
				}
			}
		} else {
			if (outChildLayoutName) {
				*outChildLayoutName = name;
			}
			name = getGlobalName(fileNamespace, name);
		}

		sl_bool flagOverride = element->getAttribute(sOverride).equals_IgnoreCase(StringView::literal("true"));
		if (!flagOverride) {
			if (m_layouts.find(name)) {
				logError(element, g_str_error_resource_layout_name_redefined, name);
				return sl_null;
			}
		}
		layout->name = name;
		layout->fileNamespace = fileNamespace;

		if (!(_parseLayoutResourceItem(layout.get(), layout.get(), sl_null, source))) {
			return sl_null;
		}

		if (!(m_layouts.put(name, layout))) {
			logError(element, g_str_error_out_of_memory);
			return sl_null;
		}

		return layout;
	}

	Ref<SAppLayoutResource> SAppDocument::_openLayoutResource(SAppLayoutResource* parent, const String& name)
	{
		Ref<SAppLayoutResource> ret;
		getItemFromMap(m_layouts, parent->fileNamespace, name, sl_null, &ret);
		if (ret.isNotNull()) {
			return ret;
		}
		if (!(m_layouts.get(name, &ret))) {
			_openUiResourceByName(name);
			m_layouts.emplace(name, sl_null);
		}
		return m_layouts.getValue(name);
	}

	sl_bool SAppDocument::_checkLayoutResourceItemName(SAppLayoutResource* layout, const String& name, const Ref<XmlElement>& element, sl_bool flagRadioGroup)
	{
		if (layout->itemsByName.find(name)) {
			logError(element, g_str_error_resource_layout_name_redefined, name);
			return sl_false;
		}
		if (!flagRadioGroup) {
			if (layout->radioGroups.find(name)) {
				logError(element, g_str_error_resource_layout_name_redefined, name);
				return sl_false;
			}
		}
		if (layout->otherNames.find(name)) {
			logError(element, g_str_error_resource_layout_name_redefined, name);
			return sl_false;
		}
		return sl_true;
	}

	sl_bool SAppDocument::_parseLayoutResourceItem(SAppLayoutResource* layout, SAppLayoutResourceItem* item, SAppLayoutResourceItem* parent, const String16& source)
	{
		if (item->element.isNull()) {
			return sl_false;
		}

		if (!(_parseStyleAttribute(layout->fileNamespace, parent ? parent->theme : String::null(), item))) {
			return sl_false;
		}
		item->theme = layout->getXmlAttribute(sTheme);
		if (item->theme.isNull()) {
			if (parent) {
				item->theme = parent->theme;
			}
		}

		if (layout == item) {
			String strType = layout->getXmlAttribute(sType);
			Ref<XmlElement> originalElement = layout->element;
			Ref<SAppLayoutInclude> include;
			getItemFromMap(m_layoutIncludes, layout->fileNamespace, strType, sl_null, &include);
			if (include.isNotNull()) {
				List< Ref<XmlElement> > list;
				RefT<SAppLayoutXmlItem> xml = new CRefT<SAppLayoutXmlItem>(layout->element);
				if (xml.isNull()) {
					logError(layout->element, g_str_error_out_of_memory);
					return sl_false;
				}
				if (!(_parseStyleAttribute(layout->fileNamespace, item->theme, xml.get()))) {
					return sl_false;
				}
				if (!_addXmlChildElements(list, layout->fileNamespace, item->theme, include->element, xml, sl_null)) {
					return sl_false;
				}
				if (list.getCount() != 1) {
					logError(include->element, g_str_error_resource_layout_must_contain_one_child);
					return sl_false;
				}
				layout->element = list.getValueAt_NoLock(0);
				if (!(_parseStyleAttribute(layout->fileNamespace, item->theme, layout))) {
					return sl_false;
				}
				strType = layout->getXmlAttribute(sType);
				layout->theme = layout->getXmlAttribute(sTheme);
			}
			SAppLayoutType type;
			if (strType.isEmpty() || strType == StringView::literal("view")) {
				type = SAppLayoutType::View;
			} else if (strType == StringView::literal("window")) {
				type = SAppLayoutType::Window;
			} else if (strType == StringView::literal("page")) {
				type = SAppLayoutType::Page;
			} else {
				logError(layout->element, g_str_error_resource_layout_type_invalid, strType);
				return sl_false;
			}
			layout->layoutType = type;
			layout->baseClassName = layout->getXmlAttribute(sBase);

			{
				ListElements< Ref<XmlElement> > styles(originalElement->getChildElements(sStyle));
				for (sl_size i = 0; i < styles.count; i++) {
					Ref<XmlElement>& eStyle = styles[i];
					if (!(_parseLayoutStyle(layout->fileNamespace, layout->theme, eStyle))) {
						return sl_false;
					}
					originalElement->removeChild(eStyle);
				}
			}

		} else {
			String strType = item->getXmlAttribute(sType);
			if (strType.isEmpty()) {
				strType = item->element->getName();
			}
			SAppLayoutItemType type = SAppLayoutResource::getTypeFromName(strType);
			if (type == SAppLayoutItemType::Unknown && parent) {
				if (strType == StringView::literal("item")) {
					switch (parent->itemType) {
						case SAppLayoutItemType::Tree:
						case SAppLayoutItemType::TreeItem:
							type = SAppLayoutItemType::TreeItem;
							break;
						default:
							break;
					}
				}
			}
			if (type == SAppLayoutItemType::Unknown) {
				logError(item->element, g_str_error_resource_layout_type_invalid, strType);
				return sl_false;
			}
			item->itemType = type;
			item->itemTypeName = strType;
			item->fileNamespace = layout->fileNamespace;
		}

		const Ref<XmlElement>& element = item->element;
		if (parent) {
			SAppLayoutXmlItem xml(element);
			String name = xml.getXmlAttributeWithoutStyle(sName);
			String arrayName;
			sl_int32 arrayIndex = -1;
			if (name.isNotEmpty()) {
				if (!(SAppUtil::checkNameOrArrayMember(name, &arrayName, &arrayIndex))) {
					logError(element, g_str_error_resource_layout_name_invalid, name);
					return sl_false;
				}
				if (!(_checkLayoutResourceItemName(layout, name, element))) {
					return sl_false;
				}
			} else {
				name = layout->getAutoIncreasingName(item->itemType);
				item->flagGeneratedName = sl_true;
			}
			item->name = name;
			item->arrayName = arrayName;
			item->arrayIndex = arrayIndex;

			if (!(layout->itemsByName.put(item->name, item))) {
				logError(element, g_str_error_out_of_memory);
				return sl_false;
			}
		}

		LayoutControlProcessParams pp;
		pp.op = SAppLayoutOperation::Parse;
		pp.source = source;
		pp.resource = layout;
		pp.resourceItem = item;
		pp.parentResourceItem = parent;
		pp.name = item->name;
		if (!(_processLayoutResourceControl(&pp))) {
			return sl_false;
		}

		String customClassName = item->getXmlAttribute(sClass).trim();
		if (customClassName.isNotEmpty()) {
			item->className = customClassName;
			if (!(layout->customClasses.put(customClassName, sl_true))) {
				logError(element, g_str_error_out_of_memory);
				return sl_false;
			}
		}

		if (!parent) {
			String strSP = layout->getXmlAttribute(sSp);
			if (!(layout->sp.parse(strSP, this))) {
				logError(element, g_str_error_resource_layout_attribute_invalid, sSp, strSP);
			}
			if (!(layout->sp.checkSP())) {
				logError(element, g_str_error_resource_layout_attribute_invalid, sSp, strSP);
				return sl_false;
			}
		}

		if (item->arrayIndex >= 0) {
			sl_uint32 n = item->arrayIndex + 1;
			SAppLayoutResource::ItemArrayDesc desc;
			if (layout->itemArrays.get(item->arrayName, &desc)) {
				if (desc.className != item->className) {
					logError(element, g_str_error_resource_layout_name_array_item_class_different, item->name);
				}
				if (desc.itemCount < n) {
					desc.itemCount = n;
					layout->itemArrays.put(item->arrayName, desc);
				}
			} else {
				desc.className = item->className;
				desc.itemCount = n;
				layout->itemArrays.put(item->arrayName, desc);
			}
		}

		{
			item->getEventMapping(item->eventMappings);
			for (auto&& mapping : item->eventMappings) {
				if (!(mapping.value.startsWith(StringView::literal("on")))) {
					logError(item->element, g_str_error_event_invalid_prefix, mapping.value);
					return sl_false;
				}
				_registerLayoutCustomEvent(layout, mapping.value, item->name, mapping.key, sl_false);
			}
		}

		return sl_true;
	}

	void SAppDocument::_registerLayoutCustomEvent(SAppLayoutResource* layout, const String& customEvent, const String& itemName, const String& itemEvent, sl_bool flagIterate)
	{
		List<SAppLayoutResource::ItemEvent> events = layout->customEvents.getValue(customEvent);
		if (events.isNull()) {
			events = List<SAppLayoutResource::ItemEvent>::create();
			layout->customEvents.put(customEvent, events);
		}
		SAppLayoutResource::ItemEvent ev;
		ev.name = itemName;
		ev.event = itemEvent;
		ev.flagIterate = flagIterate;
		events.add(ev);
	}

	Ref<SAppLayoutResourceItem> SAppDocument::_parseLayoutResourceItemChild(SAppLayoutResource* layout, SAppLayoutResourceItem* parentItem, const Ref<XmlElement>& element, const String16& source)
	{
		Ref<SAppLayoutResourceItem> childItem = new SAppLayoutResourceItem(element);
		if (childItem.isNull()) {
			logError(element, g_str_error_out_of_memory);
			return sl_null;
		}
		if (!(_parseLayoutResourceItem(layout, childItem.get(), parentItem, source))) {
			return sl_null;
		}
		return childItem;
	}

	sl_bool SAppDocument::_generateLayoutsCpp(const String& targetPath)
	{
		log(g_str_log_generate_cpp_layouts_begin);

		if (!(File::isDirectory(targetPath + StringView::literal("/ui")))) {
			File::createDirectory(targetPath + StringView::literal("/ui"));
			if (!(File::isDirectory(targetPath + StringView::literal("/ui")))) {
				log(g_str_error_directory_create_failed, targetPath + StringView::literal("/ui"));
				return sl_false;
			}
		}

		StringBuffer sbHeader, sbHeaderBase, sbCpp;
		sbHeaderBase.addStatic("#pragma once\r\n\r\n#include <slib/ui/resource.h>\r\n#include \"menus.h\"\r\n");
		sbHeader.addStatic("#pragma once\r\n\r\n");

		{
			ListElements<String> includes(m_conf.generate_cpp.layout.include_headers);
			for (sl_size i = 0; i < includes.count; i++) {
				if (includes[i].isNotEmpty()) {
					sbHeaderBase.add(String::format(StringView::literal("#include \"%s\"%n"), includes[i]));
				}
			}
		}

		sbCpp.add(String::format(StringView::literal(
								 "#include <slib/ui.h>%n%n"
								 "#include \"layouts.h\"%n%n"
								 "#include \"strings.h\"%n"
								 "#include \"colors.h\"%n"
								 "#include \"drawables.h\"%n"
								), m_conf.generate_cpp.ns));

		{
			ListElements<String> includes(m_conf.generate_cpp.layout.include_headers_in_cpp);
			for (sl_size i = 0; i < includes.count; i++) {
				if (includes[i].isNotEmpty()) {
					sbCpp.add(String::format(StringView::literal("#include \"%s\"%n"), includes[i]));
				}
			}
		}

		sbHeaderBase.add(String::format(StringView::literal("%n" "namespace %s%n" "{%n\tnamespace ui%n\t{%n"), m_conf.generate_cpp.ns));
		{
			for (auto&& pair : m_layouts) {
				if (pair.value.isNotNull()) {
					Ref<SAppLayoutResource> layout = pair.value;
					sbHeaderBase.add(String::format(StringView::literal("\t\tclass %s;%n"), pair.key));
				}
			}
		}
		sbHeaderBase.addStatic("\t}\r\n}\r\n");

		{
			for (auto&& pair : m_layouts) {
				if (pair.value.isNotNull()) {
					sbHeader.add(String::format(StringView::literal("#include \"ui/%s.h\"%n"), pair.key));
					sbCpp.add(String::format(StringView::literal("#include \"ui/%s.cpp.inc\"%n"), pair.key));
					if (!(_generateLayoutsCpp_Layout(targetPath, pair.value.get()))) {
						return sl_false;
					}
				}
			}
		}

		String pathHeaderBase = targetPath + StringView::literal("/layouts_base.h");
		String contentHeaderBase = sbHeaderBase.merge();
		if (File::readAllTextUTF8(pathHeaderBase) != contentHeaderBase) {
			if (!(File::writeAllTextUTF8(pathHeaderBase, contentHeaderBase))) {
				logError(g_str_error_file_write_failed, pathHeaderBase);
				return sl_false;
			}
		}

		String pathHeader = targetPath + StringView::literal("/layouts.h");
		String contentHeader = sbHeader.merge();
		if (File::readAllTextUTF8(pathHeader) != contentHeader) {
			if (!(File::writeAllTextUTF8(pathHeader, contentHeader))) {
				logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}

		String pathCpp = targetPath + StringView::literal("/layouts.cpp");
		String contentCpp = sbCpp.merge();
		if (File::readAllTextUTF8(pathCpp) != contentCpp) {
			if (!(File::writeAllTextUTF8(pathCpp, contentCpp))) {
				logError(g_str_error_file_write_failed, pathCpp);
				return sl_false;
			}
		}

		return sl_true;
	}

	sl_bool SAppDocument::_generateLayoutsCpp_Layout(const String& targetPath, SAppLayoutResource* layout)
	{

		String name = layout->name;

		StringBuffer sbHeader, sbCpp;

		sbHeader.addStatic("#pragma once\r\n\r\n#include \"../layouts_base.h\"\r\n");
		{
			String prefix = name + "_";
			for (auto&& item : m_layouts) {
				if (item.key.startsWith(prefix)) {
					sbHeader.add(String::format(StringView::literal("#include \"%s.h\"%n"), item.key));
				}
			}
		}
		sbHeader.addStatic("\r\n");

		String namespacePrefix = String::format(StringView::literal("namespace %s%n" "{%n\tnamespace ui%n\t{%n") , m_conf.generate_cpp.ns);
		sbHeader.add(namespacePrefix);
		sbCpp.add(namespacePrefix);

		if (layout->baseClassName.isNotEmpty()) {
			sbHeader.add(String::format(StringView::literal("\t\tSLIB_DECLARE_UILAYOUT_BEGIN(%s, %s)%n"), name, layout->baseClassName));
			sbCpp.add(String::format(StringView::literal("\t\tSLIB_DEFINE_UILAYOUT(%s, %s)%n%n"), name, layout->baseClassName));
		} else {
			if (layout->layoutType == SAppLayoutType::Window) {
				sbHeader.add(String::format(StringView::literal("\t\tSLIB_DECLARE_WINDOW_LAYOUT_BEGIN(%s)%n"), name));
				sbCpp.add(String::format(StringView::literal("\t\tSLIB_DEFINE_WINDOW_LAYOUT(%s)%n%n"), name));
			} else if (layout->layoutType == SAppLayoutType::Page) {
				sbHeader.add(String::format(StringView::literal("\t\tSLIB_DECLARE_PAGE_LAYOUT_BEGIN(%s)%n"), name));
				sbCpp.add(String::format(StringView::literal("\t\tSLIB_DEFINE_PAGE_LAYOUT(%s)%n%n"), name));
			} else if (layout->layoutType == SAppLayoutType::View) {
				sbHeader.add(String::format(StringView::literal("\t\tSLIB_DECLARE_VIEW_LAYOUT_BEGIN(%s)%n"), name));
				sbCpp.add(String::format(StringView::literal("\t\tSLIB_DEFINE_VIEW_LAYOUT(%s)%n%n"), name));
			} else {
				return sl_false;
			}
		}

		{
			String prefix = name + StringView::literal("_");
			for (auto&& name : m_layouts) {
				if (name.key.startsWith(prefix)) {
					sbHeader.add(String::format(StringView::literal("\t\t\ttypedef %s %s;%n"), name.key, name.key.substring(prefix.getLength())));
				}
			}
		}

		sbCpp.add(String::format(StringView::literal("\t\tvoid %s::initialize()%n\t\t{%n"), name));

		{
			ListElements<String> radioGroups(layout->radioGroups.getAllKeys());
			for (sl_size i = 0; i < radioGroups.count; i++) {
				sbHeader.add(String::format(StringView::literal("\t\t\tslib::Ref<slib::RadioGroup> %s;%n"), radioGroups[i]));
				sbCpp.add(String::format(StringView::literal("\t\t\t%s = new slib::RadioGroup;%n"), radioGroups[i]));
			}
			if (radioGroups.count > 0) {
				sbHeader.addStatic("\r\n");
				sbCpp.addStatic("\r\n");
			}
		}
		{
			ObjectLocker lock(&(layout->itemArrays));
			for (auto&& item : layout->itemArrays) {
				sbHeader.add(String::format(StringView::literal("\t\t\tslib::Ref<%s> %s[%d];%n"), item.value.className, item.key, item.value.itemCount));
			}
			if (layout->itemArrays.isNotEmpty()) {
				sbHeader.addStatic("\r\n");
			}
		}
		{
			MutexLocker lock(layout->customEvents.getLocker());
			for (auto&& ev : layout->customEvents) {
				sbHeader.add(String::format(StringView::literal("\t\t\tSLIB_UILAYOUT_EVENT(%s) {%n"), StringView(ev.key).substring(2)));
				for (auto&& item : ev.value) {
					if (item.flagIterate) {
						sbHeader.add(String::format(StringView::literal("\t\t\t\tSLIB_UILAYOUT_FORWARD_ITERATE_EVENT(%s, %s)%n"), item.name, item.event));
					} else {
						sbHeader.add(String::format(StringView::literal("\t\t\t\tSLIB_UILAYOUT_FORWARD_EVENT(%s, %s)%n"), item.name, item.event));
					}
				}
				sbHeader.addStatic("\t\t\t}\r\n");
			}
			if (layout->customEvents.isNotEmpty()) {
				sbHeader.addStatic("\r\n");
			}
		}

		StringBuffer sbLayout;

		if (layout->sp.flagDefined) {
			if (layout->sp.isNeededOnLayoutFunction()) {
				sbLayout.add(String::format(StringView::literal("%n\t\t\tsetScaledPixel(%s);%n"), layout->sp.getAccessString()));
			} else {
				sbCpp.add(String::format(StringView::literal("%n\t\t\tsetScaledPixel(%s);%n%n"), layout->sp.getAccessString()));
			}
		}

		StringBuffer sbDelayed;
		StringBuffer sbSetData;

		LayoutControlGenerateParams params;
		params.sbDeclare = &sbHeader;
		params.sbDefineInit = &sbCpp;
		params.sbDefineInitDelayed = &sbDelayed;
		params.sbDefineLayout = &sbLayout;
		params.sbDefineSetData = &sbSetData;
		if (!(_generateLayoutsCpp_Item(layout, layout, sl_null, &params, sl_null))) {
			return sl_false;
		}

		if (sbDelayed.getLength()) {
			sbCpp.link(sbDelayed);
		}
		sbCpp.add(String::format(StringView::literal("\t\t}%n%n\t\tvoid %s::layoutViews(sl_ui_len CONTENT_WIDTH, sl_ui_len CONTENT_HEIGHT)%n\t\t{%n"), name));
		sbCpp.link(sbLayout);
		sbCpp.add(String::format(StringView::literal("\t\t}%n%n\t\tvoid %s::setData(const slib::Variant& data, slib::UIUpdateMode mode)%n\t\t{%n"), name));
		sbCpp.link(sbSetData);
		sbCpp.addStatic("\t\t}\r\n\r\n");

		if (layout->baseClassName.isNotEmpty()) {
			sbHeader.addStatic("\t\tSLIB_DECLARE_UILAYOUT_END\r\n\r\n");
		} else {
			if (layout->layoutType == SAppLayoutType::Window) {
				sbHeader.addStatic("\t\tSLIB_DECLARE_WINDOW_LAYOUT_END\r\n\r\n");
			} else if (layout->layoutType == SAppLayoutType::Page) {
				sbHeader.addStatic("\t\tSLIB_DECLARE_PAGE_LAYOUT_END\r\n\r\n");
			} else if (layout->layoutType == SAppLayoutType::View) {
				sbHeader.addStatic("\t\tSLIB_DECLARE_VIEW_LAYOUT_END\r\n\r\n");
			}
		}

		sbHeader.addStatic("\t}\r\n}\r\n");
		sbCpp.addStatic("\t}\r\n}\r\n");

		String pathHeader = String::concat(targetPath, "/ui/", name, ".h");
		String contentHeader = sbHeader.merge();
		if (File::readAllTextUTF8(pathHeader) != contentHeader) {
			if (!(File::writeAllTextUTF8(pathHeader, contentHeader))) {
				logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}

		String pathCpp = String::concat(targetPath, "/ui/", name, ".cpp.inc");
		String contentCpp = sbCpp.merge();
		if (File::readAllTextUTF8(pathCpp) != contentCpp) {
			if (!(File::writeAllTextUTF8(pathCpp, contentCpp))) {
				logError(g_str_error_file_write_failed, pathCpp);
				return sl_false;
			}
		}

		return sl_true;
	}

	sl_bool SAppDocument::_generateLayoutsCpp_Item(SAppLayoutResource* layout, SAppLayoutResourceItem* item, SAppLayoutResourceItem* parent, LayoutControlGenerateParams* params, const String& addStatement)
	{
		String name;
		if (parent) {
			name = item->name;
			String className;
			if (item->classNameGetter.isNotNull()) {
				className = item->classNameGetter(this, item);
			} else {
				className = item->className;
				getItemFromMap(m_layouts, layout->fileNamespace, className, &className, (Ref<SAppLayoutResource>*)sl_null);
			}
			if (item->arrayIndex < 0) {
				if (className.endsWith('>')) {
					params->sbDeclare->add(String::format(StringView::literal("\t\t\tslib::Ref< %s > %s;%n"), className, name));
				} else {
					params->sbDeclare->add(String::format(StringView::literal("\t\t\tslib::Ref<%s> %s;%n"), className, name));
				}
			}
			params->sbDefineInit->add(String::format(StringView::literal("\t\t\t%2$s = new %1$s;%n"), className, name));
		} else {
			name = sThis;
		}

		LayoutControlProcessParams pp;
		pp.op = SAppLayoutOperation::Generate;
		pp.resource = layout;
		pp.resourceItem = item;
		pp.parentResourceItem = parent;
		pp.name = name;
		pp.addStatement = addStatement;
		(LayoutControlGenerateParams&)pp = *params;

		if (!(_processLayoutResourceControl(&pp))) {
			return sl_false;
		}

		return sl_true;
	}

	sl_bool SAppDocument::_simulateLayoutInWindow(SAppLayoutResource* layout, SAppSimulateLayoutParam& param)
	{
		Ref<SAppLayoutSimulationWindow> window = new SAppLayoutSimulationWindow;
		if (window.isNotNull()) {
			UISize size = UI::getScreenSize();
			m_layoutSimulationParams.screenWidth = size.x;
			m_layoutSimulationParams.screenHeight = size.y;
			SAppDimensionValue simulatorWidth;
			if (simulatorWidth.parse(layout->getXmlAttribute(sSimulatorWidth), sl_null)) {
				if (simulatorWidth.flagDefined && simulatorWidth.checkForWindowSize()) {
					param.pageSize.x = _getDimensionValue(simulatorWidth);
					window->setSavingPageSize(sl_false);
				}
			}
			SAppDimensionValue simulatorHeight;
			if (simulatorHeight.parse(layout->getXmlAttribute(sSimulatorHeight), sl_null)) {
				if (simulatorHeight.flagDefined && simulatorHeight.checkForWindowSize()) {
					param.pageSize.y = _getDimensionValue(simulatorHeight);
					window->setSavingPageSize(sl_false);
				}
			}
			if (layout->layoutType == SAppLayoutType::Window) {
				auto callback = param.onCloseWindow;
				window->setOnDestroy([callback](Window* window) {
					window->onDestroy();
					callback((SAppLayoutSimulationWindow*)window);
				});
			} else {
				window->setClientSize(param.pageSize);
				auto callback = param.onClosePage;
				window->setOnDestroy([callback](Window* window) {
					window->onDestroy();
					callback((SAppLayoutSimulationWindow*)window);
				});
			}
			return window->open(this, layout);
		}
		return sl_false;
	}

	void SAppDocument::_registerLayoutSimulationWindow(const Ref<SAppLayoutSimulationWindow>& window)
	{
		m_layoutSimulationWindows.add(window);
	}

	void SAppDocument::_removeLayoutSimulationWindow(const Ref<SAppLayoutSimulationWindow>& window)
	{
		m_layoutSimulationWindows.remove(window);
	}

	Ref<CRef> SAppDocument::_simulateLayoutCreateOrLayoutItem(SAppLayoutSimulator* simulator, SAppLayoutResourceItem* item, SAppLayoutResourceItem* parent, CRef* parentItem, SAppLayoutOperation op)
	{
		Ref<SAppLayoutSimulationWindow> window = simulator->getSimulationWindow();
		if (window.isNull()) {
			return sl_null;
		}
		Ref<SAppLayoutResource> layout = simulator->getLayoutResource();
		if (layout.isNull()) {
			return sl_null;
		}

		Ref<CRef> viewItem;
		if (parent) {
			if (op == SAppLayoutOperation::SimulateLayout) {
				viewItem = simulator->getViewItemByName(item->name);
				if (viewItem.isNull()) {
					return sl_null;
				}
			}
		} else {
			viewItem = simulator->getSimulationContentView();
			View* view = (View*)(viewItem.get());
			if (viewItem.isNull()) {
				return sl_null;
			}
			UISize size = window->getClientSize();
			if (!(simulator->isImportView()) && layout->layoutType == SAppLayoutType::Page) {
				m_layoutSimulationParams.screenWidth = size.x;
				m_layoutSimulationParams.screenHeight = size.y;
				m_layoutSimulationParams.flagResizeScreen = sl_true;
				m_layoutSimulationParams.viewportWidth = view->getWidth();
				m_layoutSimulationParams.viewportHeight = view->getHeight();
			} else {
				m_layoutSimulationParams.viewportWidth = size.x;
				m_layoutSimulationParams.viewportHeight = size.y;
			}
			if (layout->sp.flagDefined) {
				m_layoutSimulationParams.sp = _getDimensionValue(layout->sp);
			} else {
				m_layoutSimulationParams.sp = 1;
			}
		}

		LayoutControlProcessParams pp;
		pp.op = op;
		pp.resource = layout.get();
		pp.resourceItem = item;
		pp.parentResourceItem = parent;
		pp.simulator = simulator;
		pp.window = window.get();
		pp.viewItem = viewItem;
		pp.parentItem = parentItem;
		pp.name = item->name;
		if (!(_processLayoutResourceControl(&pp))) {
			return sl_null;
		}

		if (parent) {
			if (op == SAppLayoutOperation::SimulateInit) {
				viewItem = pp.viewItem;
				if (viewItem.isNull()) {
					return sl_null;
				}
				simulator->registerViewItemByName(item->name, viewItem);
			}
		}

		return viewItem;

	}

	sl_ui_pos SAppDocument::_getDimensionValue(const SAppDimensionValue& value)
	{
		if (!(value.flagDefined)) {
			return 0;
		}
		switch (value.unit) {
			case SAppDimensionValue::PX:
				return UIResource::toUiPos(value.amount);
			case SAppDimensionValue::SW:
			case SAppDimensionValue::SAFE_W:
				return UIResource::toUiPos(value.amount * m_layoutSimulationParams.screenWidth);
			case SAppDimensionValue::SH:
			case SAppDimensionValue::SAFE_H:
				return UIResource::toUiPos(value.amount * m_layoutSimulationParams.screenHeight);
			case SAppDimensionValue::SMIN:
				return UIResource::toUiPos(value.amount * SLIB_MIN(m_layoutSimulationParams.screenWidth, m_layoutSimulationParams.screenHeight));
			case SAppDimensionValue::SMAX:
				return UIResource::toUiPos(value.amount * SLIB_MAX(m_layoutSimulationParams.screenWidth, m_layoutSimulationParams.screenHeight));
			case SAppDimensionValue::VW:
				return UIResource::toUiPos(value.amount * m_layoutSimulationParams.viewportWidth);
			case SAppDimensionValue::VH:
				return UIResource::toUiPos(value.amount * m_layoutSimulationParams.viewportHeight);
			case SAppDimensionValue::VMIN:
				return UIResource::toUiPos(value.amount * SLIB_MIN(m_layoutSimulationParams.viewportWidth, m_layoutSimulationParams.viewportHeight));
			case SAppDimensionValue::VMAX:
				return UIResource::toUiPos(value.amount * SLIB_MAX(m_layoutSimulationParams.viewportWidth, m_layoutSimulationParams.viewportHeight));
			case SAppDimensionValue::SP:
				return UIResource::toUiPos(value.amount * m_layoutSimulationParams.sp);
			case SAppDimensionValue::DP:
				return UIResource::toUiPos(slib::UIResource::dpToPixel(value.amount));
			case SAppDimensionValue::PT:
				return UIResource::toUiPos(slib::UIResource::pointToPixel(value.amount));
			case SAppDimensionValue::M:
				return UIResource::toUiPos(slib::UIResource::meterToPixel(value.amount));
			case SAppDimensionValue::CM:
				return UIResource::toUiPos(slib::UIResource::centimeterToPixel(value.amount));
			case SAppDimensionValue::MM:
				return UIResource::toUiPos(slib::UIResource::millimeterToPixel(value.amount));
			case SAppDimensionValue::INCH:
				return UIResource::toUiPos(slib::UIResource::inchToPixel(value.amount));
		}
		return 0;
	}

	sl_real SAppDocument::_getDimensionValue(const SAppDimensionFloatValue& value)
	{
		if (!(value.flagDefined)) {
			return 0;
		}
		switch (value.unit) {
			case SAppDimensionValue::PX:
				return value.amount;
			case SAppDimensionValue::SW:
				return value.amount * (sl_real)(m_layoutSimulationParams.screenWidth);
			case SAppDimensionValue::SH:
				return value.amount * (sl_real)(m_layoutSimulationParams.screenHeight);
			case SAppDimensionValue::SMIN:
				return value.amount * (sl_real)(SLIB_MIN(m_layoutSimulationParams.screenWidth, m_layoutSimulationParams.screenHeight));
			case SAppDimensionValue::SMAX:
				return value.amount * (sl_real)(SLIB_MAX(m_layoutSimulationParams.screenWidth, m_layoutSimulationParams.screenHeight));
			case SAppDimensionValue::VW:
				return value.amount * (sl_real)(m_layoutSimulationParams.viewportWidth);
			case SAppDimensionValue::VH:
				return value.amount * (sl_real)(m_layoutSimulationParams.viewportHeight);
			case SAppDimensionValue::VMIN:
				return value.amount * (sl_real)(SLIB_MIN(m_layoutSimulationParams.viewportWidth, m_layoutSimulationParams.viewportHeight));
			case SAppDimensionValue::VMAX:
				return value.amount * (sl_real)(SLIB_MAX(m_layoutSimulationParams.viewportWidth, m_layoutSimulationParams.viewportHeight));
			case SAppDimensionValue::SP:
				return value.amount * m_layoutSimulationParams.sp;
			case SAppDimensionValue::DP:
				return slib::UIResource::dpToPixel(value.amount);
			case SAppDimensionValue::PT:
				return slib::UIResource::pointToPixel(value.amount);
			case SAppDimensionValue::M:
				return slib::UIResource::meterToPixel(value.amount);
			case SAppDimensionValue::CM:
				return slib::UIResource::centimeterToPixel(value.amount);
			case SAppDimensionValue::MM:
				return slib::UIResource::millimeterToPixel(value.amount);
			case SAppDimensionValue::INCH:
				return slib::UIResource::inchToPixel(value.amount);
		}
		return 0;
	}

	sl_bool SAppDocument::_getFontAccessString(const String& fileNamespace, const SAppFontValue& value, String& result)
	{
		String strSize;
		if (value.size.flagDefined) {
			strSize = value.size.getAccessString();
		} else {
			SLIB_STATIC_STRING(v, "slib::UI::getDefaultFontSize()")
			strSize = v;
		}
		String strFamily;
		if (value.family.flagDefined) {
			if (!(_getStringAccessString(fileNamespace, value.family, strFamily))) {
				return sl_false;
			}
		} else {
			SLIB_STATIC_STRING(v, "slib::UI::getDefaultFontFamily()")
			strFamily = v;
		}
		result = String::format(StringView::literal("slib::Font::create(%s, %s, %s, %s, %s)"), strFamily, strSize, value.bold.value ? StringView::literal("sl_true") : StringView::literal("sl_false"), value.italic.value ? StringView::literal("sl_true") : StringView::literal("sl_false"), value.underline.value ? StringView::literal("sl_true") : StringView::literal("sl_false"));
		return sl_true;
	}

	sl_bool SAppDocument::_getFontValue(const String& fileNamespace, const SAppFontValue& value, Ref<Font>& result)
	{
		sl_real size;
		if (value.size.flagDefined) {
			size = _getDimensionValue(value.size);
		} else {
			size = UI::getDefaultFontSize();
		}
		String family;
		if (value.family.flagDefined) {
			if (!(_getStringValue(fileNamespace, value.family, family))) {
				return sl_false;
			}
		} else {
			family = UI::getDefaultFontFamily();
		}
		result = Font::create(family, size, value.bold.value, value.italic.value, value.underline.value);
		return sl_true;
	}

	sl_bool SAppDocument::_getBorderAccessString(const String& fileNamespace, const SAppBorderValue& value, String& result)
	{
		if (value.flagNull) {
			SLIB_STATIC_STRING(v, "sl_null")
			result = v;
			return sl_true;
		}
		String strStyle;
		if (value.style.flagDefined) {
			strStyle = value.style.getAccessString();
		} else {
			SLIB_STATIC_STRING(v, "slib::PenStyle::Default")
			strStyle = v;
		}
		String strWidth;
		if (value.width.flagDefined) {
			strWidth = value.width.getAccessString();
		} else {
			SLIB_STATIC_STRING(v, "-1.0f")
			strWidth = v;
		}
		String strColor;
		if (value.color.flagDefined) {
			if (!(_getColorAccessString(fileNamespace, value.color, strColor))) {
				return sl_false;
			}
		} else {
			SLIB_STATIC_STRING(v, "slib::Color::zero()")
			strColor = v;
		}
		result = String::format(StringView::literal("slib::PenDesc(%s, %s, %s)"), strStyle, strWidth, strColor);
		return sl_true;
	}

	sl_bool SAppDocument::_getBorderValue(const String& fileNamespace, const SAppBorderValue& value, PenDesc& result)
	{
		if (value.style.flagDefined) {
			result.style = value.style.value;
		} else {
			result.style = PenStyle::Default;
		}
		if (value.width.flagDefined) {
			result.width = _getDimensionValue(value.width);
		} else {
			result.width = -1.0f;
		}
		if (value.color.flagDefined) {
			if (!(_getColorValue(fileNamespace, value.color, result.color))) {
				return sl_false;
			}
		} else {
			result.color.setZero();
		}
		return sl_true;
	}

	Ref<SAppLayoutStyle> SAppDocument::_lookupLayoutStyle(const String& fileNamespace, const String& theme, const String& styleName)
	{
		Ref<SAppLayoutStyle> style;
		if (fileNamespace.isNotEmpty()) {
			if (theme.isNotEmpty()) {
				if (m_layoutStyles.get(getGlobalStyleName(fileNamespace, theme, styleName), &style)) {
					return style;
				}
			}
			if (m_layoutStyles.get(getGlobalStyleName(fileNamespace, sl_null, styleName), &style)) {
				return style;
			}
		}
		if (theme.isNotEmpty()) {
			if (m_layoutStyles.get(getGlobalStyleName(sl_null, theme, styleName), &style)) {
				return style;
			}
		}
		if (m_layoutStyles.get(styleName, &style)) {
			return style;
		}
		return sl_null;
	}

	sl_bool SAppDocument::_parseStyleAttribute(const String& fileNamespace, const String& parentTheme, SAppLayoutXmlItem* item)
	{
		if (item->element.isNull()) {
			return sl_true;
		}
		String theme = item->element->getAttribute(sTheme);
		if (theme.isNull()) {
			theme = parentTheme;
		}
		String propStyles = theme + StringView::literal("::styles");
		Ref< CList< Ref<SAppLayoutStyle> > > _styles = Ref< CList< Ref<SAppLayoutStyle> > >::cast(item->element->getProperty(propStyles).getRef());
		List< Ref<SAppLayoutStyle> >& styles = *(reinterpret_cast<List< Ref<SAppLayoutStyle> >*>(&_styles));
		if (styles.isNotNull()) {
			item->element->setProperty(sStyles, styles.ref);
			item->styles = Move(styles);
			return sl_true;
		}
		styles = List< Ref<SAppLayoutStyle> >::create();
		{
			Ref<SAppLayoutStyle> style = _lookupLayoutStyle(fileNamespace, theme, StringView::literal("$") + item->element->getName());
			if (style.isNotNull()) {
				if (!(styles.add_NoLock(Move(style)))) {
					logError(item->element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		}
		String strStyles = item->getXmlAttributeWithoutStyle(sStyle).trim();
		if (strStyles.isNotEmpty()) {
			ListElements<String> arr(strStyles.split(','));
			for (sl_size i = 0; i < arr.count; i++) {
				String s = arr[i].trim();
				Ref<SAppLayoutStyle> style = _lookupLayoutStyle(fileNamespace, theme, s);
				if (style.isNotNull()) {
					if (!(styles.add_NoLock(Move(style)))) {
						logError(item->element, g_str_error_out_of_memory);
						return sl_false;
					}
				} else {
					logError(item->element, g_str_error_layout_style_not_found, s);
					return sl_false;
				}
			}
		}
		item->element->setProperty(propStyles, styles.ref);
		item->element->setProperty(sStyles, styles.ref);
		item->styles = Move(styles);
		return sl_true;
	}

	sl_bool SAppDocument::_getXmlChildElements(List< Ref<XmlElement> >& ret, const String& fileNamespace, const String& theme, SAppLayoutXmlItem* item, const String& tagName)
	{
		if (!_addXmlChildElements(ret, fileNamespace, theme, item->element, sl_null, tagName)) {
			return sl_false;
		}
		{
			ListElements< Ref<SAppLayoutStyle> > _styles(item->styles);
			for (sl_size i = 0; i < _styles.count; i++) {
				Ref<SAppLayoutStyle> style = _styles[i];
				if (style.isNotNull()) {
					if (!(_addXmlChildElements(ret, fileNamespace, theme, style.get(), tagName))) {
						return sl_false;
					}
				}
			}
		}
		return sl_true;
	}

	sl_bool SAppDocument::_addXmlChildElements(List< Ref<XmlElement> >& list, const String& fileNamespace, const String& theme, SAppLayoutStyle* style, const String& tagName)
	{
		{
			ListElements< Ref<SAppLayoutStyle> > _styles(style->styles);
			for (sl_size i = 0; i < _styles.count; i++) {
				Ref<SAppLayoutStyle> other = _styles[i];
				if (other.isNotNull()) {
					if (!_addXmlChildElements(list, fileNamespace, theme, other.get(), tagName)) {
						return sl_false;
					}
				}
			}
		}
		return _addXmlChildElements(list, fileNamespace, theme, style->element, sl_null, tagName);
	}

	sl_bool SAppDocument::_addXmlChildElements(List< Ref<XmlElement> >& list, const String& fileNamespace, const String& theme, const Ref<XmlElement>& parent, const RefT<SAppLayoutXmlItem>& caller, const String& tagName)
	{
		{
			ListElements< Ref<XmlElement> > children(parent->getChildElements());
			for (sl_size i = 0; i < children.count; i++) {
				Ref<XmlElement> element = children[i];
				if (element.isNotNull()) {
					if (caller.isNotNull()) {
						element = element->duplicate();
						if (element.isNull()) {
							logError(element, g_str_error_out_of_memory);
							return sl_false;
						}
						element->setProperty(sCaller, caller);
						if (children.count == 1) {
							element->setProperty(sInherit, sl_true);
						}
					}
					Ref<SAppLayoutInclude> include;
					String name = element->getName();
					if (name == StringView::literal("include")) {
						String src = element->getAttribute(sSrc);
						if (src.isEmpty()) {
							logError(element, g_str_error_resource_layout_attribute_invalid, sSrc, name);
							return sl_false;
						}
						getItemFromMap(m_layoutIncludes, fileNamespace, src, sl_null, &include);
						if (include.isNull()) {
							logError(element, g_str_error_layout_include_not_found, name);
							return sl_false;
						}
					} else {
						getItemFromMap(m_layoutIncludes, fileNamespace, name, sl_null, &include);
					}
					if (include.isNotNull()) {
						RefT<SAppLayoutXmlItem> xml = new CRefT<SAppLayoutXmlItem>(element);
						if (xml.isNull()) {
							logError(element, g_str_error_out_of_memory);
							return sl_false;
						}
						if (!(_parseStyleAttribute(fileNamespace, theme, xml.get()))) {
							return sl_false;
						}
						String _theme = xml->getXmlAttribute(sTheme);
						if (_theme.isNull()) {
							_theme = theme;
						}
						if (!_addXmlChildElements(list, fileNamespace, _theme, include->element, xml, tagName)) {
							return sl_false;
						}
					} else {
						sl_bool flagIfdef = name == StringView::literal("ifdef");
						sl_bool flagIfndef = name == StringView::literal("ifndef");
						sl_bool flagIfeq = name == StringView::literal("ifeq");
						sl_bool flagIfneq = name == StringView::literal("ifneq");
						if (flagIfdef || flagIfndef || flagIfeq || flagIfneq) {
							String var = element->getAttribute(sName);
							if (var.startsWith(':')) {
								SAppLayoutXmlItem item(element);
								String value = item.getVariableValue(var.substring(1));
								sl_bool flagPass;
								if (flagIfdef) {
									flagPass = value.isNotEmpty();
								} else if (flagIfndef) {
									flagPass = value.isEmpty();
								} else if (flagIfeq) {
									flagPass = value == item.getXmlAttribute(sValue);
								} else {
									flagPass = value != item.getXmlAttribute(sValue);
								}
								if (flagPass) {
									if (!_addXmlChildElements(list, fileNamespace, theme, element, sl_null, tagName)) {
										return sl_false;
									}
								}
							}
						} else if (name == StringView::literal("children")) {
							RefT<SAppLayoutXmlItem> caller;
							Ref<XmlElement> e = parent;
							while (e.isNotNull()) {
								caller = RefT<SAppLayoutXmlItem>::cast(e->getProperty(sCaller).getRef());
								if (caller.isNotNull()) {
									break;
								}
								e = e->getParentElement();
							}
							if (caller.isNotNull()) {
								ListElements< Ref<XmlElement> > callerChildren(caller->element->getChildElements());
								for (sl_size i = 0; i < callerChildren.count; i++) {
									if (!(list.add_NoLock(Move(callerChildren[i])))) {
										logError(element, g_str_error_out_of_memory);
										return sl_false;
									}
								}
							}
						} else if (tagName.isEmpty() || name == tagName) {
							if (!(list.add_NoLock(Move(element)))) {
								logError(element, g_str_error_out_of_memory);
								return sl_false;
							}
						}
					}
				}
			}
		}
		return sl_true;
	}

	namespace
	{
		static sl_bool IsNoView(SAppLayoutItemType type)
		{
			return type >= SAppLayoutItemType::NoView;
		}

		static sl_bool IsSimulateOp(int op)
		{
			return op >= SAppLayoutOperation::SimulateInit && op <= SAppLayoutOperation::SimulateLayout;
		}
	}

#define PROCESS_CONTROL_SWITCH(NAME) \
	case SAppLayoutItemType::NAME: \
		if (!(_processLayoutResourceControl_##NAME(params))) { \
			return sl_false; \
		} \
		break;

	sl_bool SAppDocument::_processLayoutResourceControl(LayoutControlProcessParams* params)
	{
		m_currentFileNamespace = params->resource->name;
		SAppLayoutResourceItem* resourceItem = params->resourceItem;
		SAppLayoutOperation op = params->op;
		SAppLayoutItemType resourceType = resourceItem->itemType;
		switch (resourceType) {
			case SAppLayoutItemType::ViewGroup:
				{
					if (params->parentResourceItem) {
						if (!(_processLayoutResourceControl_ViewGroup(params))) {
							return sl_false;
						}
					} else {
						if (params->resource->layoutType == SAppLayoutType::Window) {
							if (!(_processLayoutResourceControl_Window(params))) {
								return sl_false;
							}
						} else if (params->resource->layoutType == SAppLayoutType::Page) {
							if (!(_processLayoutResourceControl_Page(params))) {
								return sl_false;
							}
						} else {
							if (!(_processLayoutResourceControl_ViewGroup(params))) {
								return sl_false;
							}
						}
					}
				}
				break;
			PROCESS_CONTROL_SWITCH(View)
			PROCESS_CONTROL_SWITCH(Import)
			PROCESS_CONTROL_SWITCH(Button)
			PROCESS_CONTROL_SWITCH(Label)
			PROCESS_CONTROL_SWITCH(Line)
			PROCESS_CONTROL_SWITCH(Check)
			PROCESS_CONTROL_SWITCH(Radio)
			PROCESS_CONTROL_SWITCH(Edit)
			PROCESS_CONTROL_SWITCH(Password)
			PROCESS_CONTROL_SWITCH(TextArea)
			PROCESS_CONTROL_SWITCH(Image)
			PROCESS_CONTROL_SWITCH(Select)
			PROCESS_CONTROL_SWITCH(SelectSwitch)
			PROCESS_CONTROL_SWITCH(ComboBox)
			PROCESS_CONTROL_SWITCH(Scroll)
			PROCESS_CONTROL_SWITCH(Linear)
			PROCESS_CONTROL_SWITCH(LinearIterate)
			PROCESS_CONTROL_SWITCH(TileLayout)
			PROCESS_CONTROL_SWITCH(TileIterate)
			PROCESS_CONTROL_SWITCH(List)
			PROCESS_CONTROL_SWITCH(Collection)
			PROCESS_CONTROL_SWITCH(Table)
			PROCESS_CONTROL_SWITCH(ListControl)
			PROCESS_CONTROL_SWITCH(Render)
			PROCESS_CONTROL_SWITCH(Tab)
			PROCESS_CONTROL_SWITCH(Tree)
			PROCESS_CONTROL_SWITCH(TreeItem)
			PROCESS_CONTROL_SWITCH(Split)
			PROCESS_CONTROL_SWITCH(Web)
			PROCESS_CONTROL_SWITCH(Progress)
			PROCESS_CONTROL_SWITCH(Slider)
			PROCESS_CONTROL_SWITCH(Switch)
			PROCESS_CONTROL_SWITCH(Picker)
			PROCESS_CONTROL_SWITCH(DatePicker)
			PROCESS_CONTROL_SWITCH(Pager)
			PROCESS_CONTROL_SWITCH(Navigation)
			PROCESS_CONTROL_SWITCH(Audio)
			PROCESS_CONTROL_SWITCH(Video)
			PROCESS_CONTROL_SWITCH(Camera)
			PROCESS_CONTROL_SWITCH(Drawer)
			PROCESS_CONTROL_SWITCH(Refresh)
			PROCESS_CONTROL_SWITCH(ListBox)
			PROCESS_CONTROL_SWITCH(LabelList)
			PROCESS_CONTROL_SWITCH(Pdf)
			PROCESS_CONTROL_SWITCH(GroupBox)
			PROCESS_CONTROL_SWITCH(Map)
			PROCESS_CONTROL_SWITCH(Grid)
			default:
				return sl_false;
		}

		if (op == SAppLayoutOperation::Parse) {
			if (resourceItem->flagSkipParseChildren) {
				return sl_true;
			}
			List< Ref<XmlElement> > children;
			if (!(_getXmlChildElements(children, params->resource->fileNamespace, resourceItem->theme, resourceItem, sl_null))) {
				return sl_false;
			}
			for (auto&& child : children) {
				String tagName = child->getName();
				if (tagName == StringView::literal("layout")) {
					if (resourceType != SAppLayoutItemType::LinearIterate && resourceType != SAppLayoutItemType::TileIterate && resourceType != SAppLayoutItemType::List && resourceType != SAppLayoutItemType::Collection) {
						Ref<SAppLayoutResource> childLayout = _parseLayoutResource(params->resource->filePath, params->resource->fileNamespace, child, params->source, params->resource, sl_null);
						if (childLayout.isNull()) {
							return sl_false;
						}
					}
					continue;
				}
				sl_bool flagIgnoreChild = sl_false;
				switch (resourceType) {
					case SAppLayoutItemType::Table:
						flagIgnoreChild = tagName == StringView::literal("column") || tagName == StringView::literal("row");
						break;
					case SAppLayoutItemType::ListControl:
						flagIgnoreChild = tagName == StringView::literal("column");
						break;
					case SAppLayoutItemType::Grid:
						flagIgnoreChild = tagName == StringView::literal("column") || tagName == StringView::literal("row") || tagName == StringView::literal("header") || tagName == StringView::literal("footer") || tagName == StringView::literal("body");
						break;
					case SAppLayoutItemType::Select:
					case SAppLayoutItemType::SelectSwitch:
					case SAppLayoutItemType::ComboBox:
					case SAppLayoutItemType::Picker:
					case SAppLayoutItemType::LabelList:
					case SAppLayoutItemType::Tab:
					case SAppLayoutItemType::Split:
					case SAppLayoutItemType::Pager:
						flagIgnoreChild = tagName == StringView::literal("item");
						break;
					case SAppLayoutItemType::TreeItem:
						if (tagName != StringView::literal("item")) {
							logError(child, g_str_error_resource_layout_type_invalid, tagName);
							return sl_false;
						}
						break;
					default:
						break;
				}
				if (flagIgnoreChild) {
					continue;
				}
				Ref<SAppLayoutResourceItem> childItem = _parseLayoutResourceItemChild(params->resource, resourceItem, child, params->source);
				if (childItem.isNull()) {
					return sl_false;
				}
				if (resourceType == SAppLayoutItemType::Linear) {
					if (IsNoView(childItem->itemType)) {
						return sl_false;
					}
					SAppLayoutLinearAttributes* attrs = (SAppLayoutLinearAttributes*)(resourceItem->attrs.get());
					SAppLayoutViewAttributes* childAttrs = (SAppLayoutViewAttributes*)(childItem->attrs.get());
					if (!(attrs->orientation.flagDefined) || attrs->orientation.value == LayoutOrientation::Vertical) {
						childAttrs->topMode = PositionMode::Free;
						childAttrs->bottomMode = PositionMode::Free;
					} else {
						childAttrs->leftMode = PositionMode::Free;
						childAttrs->rightMode = PositionMode::Free;
					}
				} else if (resourceType == SAppLayoutItemType::Refresh) {
					if (IsNoView(childItem->itemType)) {
						return sl_false;
					}
					SAppLayoutViewAttributes* childAttrs = (SAppLayoutViewAttributes*)(childItem->attrs.get());
					childAttrs->width.flagDefined = sl_true;
					childAttrs->width.amount = 1;
					childAttrs->width.unit = SAppDimensionValue::MATCH_PARENT;
					childAttrs->height.flagDefined = sl_true;
					childAttrs->height.amount = 1;
					childAttrs->height.unit = SAppDimensionValue::MATCH_PARENT;
				}
				if (!(resourceItem->children.add_NoLock(Move(childItem)))) {
					logError(resourceItem->element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == SAppLayoutOperation::Generate) {
			if (resourceItem->flagSkipGenerateChildren) {
				return sl_true;
			}
			String name;
			if (params->parentResourceItem) {
				name = params->name;
			} else {
				params->sbDefineInit->addStatic("\r\n");
				SLIB_STATIC_STRING(v, "m_contentView")
				name = v;
			}
			ListElements< Ref<SAppLayoutResourceItem> > children(resourceItem->children);
			for (sl_size i = 0; i < children.count; i++) {
				Ref<SAppLayoutResourceItem>& child = children[i];
				String addStatement = String::format(StringView::literal("\t\t\t%s->addChild(%s, slib::UIUpdateMode::Init);%n%n"), name, child->name);
				if (!(_generateLayoutsCpp_Item(params->resource, child.get(), resourceItem, params, addStatement) )) {
					return sl_false;
				}
			}
		} else if (IsSimulateOp(op)) {
			if (resourceItem->flagSkipSimulateChildren) {
				return sl_true;
			}
			if (!(params->parentResourceItem) && params->resource->layoutType != SAppLayoutType::Window) {
				View* view = CastInstance<View>(params->viewItem.get());
				if (view) {
					m_layoutSimulationParams.viewportWidth = view->getWidth();
					m_layoutSimulationParams.viewportHeight = view->getHeight();
				}
			}
			ListElements< Ref<SAppLayoutResourceItem> > children(resourceItem->children);
			for (sl_size i = 0; i < children.count; i++) {
				Ref<SAppLayoutResourceItem>& child = children[i];
				Ref<CRef> childViewItem = _simulateLayoutCreateOrLayoutItem(params->simulator, child.get(), resourceItem, params->viewItem.get(), op);
				if (childViewItem.isNotNull()) {
					if (op == SAppLayoutOperation::SimulateInit) {
						View* view = CastInstance<View>(params->viewItem.get());
						if (view) {
							View* childView = CastInstance<View>(childViewItem.get());
							if (childView) {
								view->addChild(ToRef(&childView), UIUpdateMode::Init);
							}
						}
					}
				} else {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

#define BEGIN_PROCESS_LAYOUT_CONTROL(NAME, VIEWTYPE) \
	sl_bool SAppDocument::_processLayoutResourceControl_##NAME(LayoutControlProcessParams* params) \
	{ \
		StringView strTab = StringView::literal("\t\t\t"); \
		SLIB_UNUSED(strTab) \
		SAppLayoutResource* resource = params->resource; \
		SLIB_UNUSED(resource) \
		SAppLayoutResourceItem* resourceItem = params->resourceItem; \
		const Ref<XmlElement>& element = resourceItem->element; \
		SAppLayoutOperation op = params->op; \
		const String& name = params->name; \
		SLIB_UNUSED(name) \
		Ref<SAppLayout##NAME##Attributes>& attr = Ref<SAppLayout##NAME##Attributes>::cast(resourceItem->attrs); \
		if (op == SAppLayoutOperation::Parse) { \
			if (attr.isNull()) { \
				attr = new SAppLayout##NAME##Attributes; \
				if (attr.isNull()) { \
					logError(element, g_str_error_out_of_memory); \
					return sl_false; \
				} \
			} \
			if (resourceItem->className.isEmpty()) { \
				SLIB_STATIC_STRING(v, "slib::" #VIEWTYPE) \
				resourceItem->className = v; \
			} \
		} else if (op == SAppLayoutOperation::SimulateInit) { \
			if (params->viewItem.isNull()) { \
				params->viewItem = new VIEWTYPE; \
			} \
		} \
		VIEWTYPE* view = (VIEWTYPE*)(params->viewItem.get()); \
		SLIB_UNUSED(view)

#define END_PROCESS_LAYOUT_CONTROL \
		return sl_true; \
	}

#define LAYOUT_CONTROL_DEFINE_XML(NAME, ...) \
	SAppLayoutXmlItem NAME(__VA_ARGS__); \
	if (!(_parseStyleAttribute(resource->fileNamespace, params->parentResourceItem ? params->parentResourceItem->theme : String::null(), &NAME))) { \
		return sl_false; \
	}

#define LAYOUT_CONTROL_DEFINE_XML_CHILDREN(NAME, XML, TAG_NAME) \
	List< Ref<XmlElement> > _##NAME; \
	if (!(_getXmlChildElements(_##NAME, resource->fileNamespace, resourceItem->theme, &XML, TAG_NAME))) { \
		return sl_false; \
	} \
	ListElements< Ref<XmlElement> > NAME(_##NAME);

#define LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(NAME, TAG_NAME) LAYOUT_CONTROL_DEFINE_XML_CHILDREN(NAME, *resourceItem, TAG_NAME)

	namespace
	{
		static const Ref<XmlElement>& GetXmlElement(SAppLayoutXmlItem& item)
		{
			return item.element;
		}

		static String GetXmlAttribute(SAppLayoutXmlItem& item, const String& name)
		{
			return item.getXmlAttribute(name);
		}

		static const Ref<XmlElement>& GetXmlElement(const Ref<XmlElement>& element)
		{
			return element;
		}

		static String GetXmlAttribute(const Ref<XmlElement>& element, const String& name)
		{
			return element->getAttribute(name);
		}
	}

#define LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME) \
	logError(GetXmlElement(XML), g_str_error_resource_layout_attribute_invalid, NAME, GetXmlAttribute(XML, NAME));
#define LOG_ERROR_LAYOUT_CONTROL_ATTR(NAME) LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(*resourceItem, NAME)

#define TEXT_Init "Init"
#define TEXT_None "None"
#define TEXT_UpdateLayout "UpdateLayout"

#define GEN_UPDATE_MODE1(MODE) "slib::UIUpdateMode::" TEXT_##MODE
#define GEN_UPDATE_MODE2(MODE) ", " GEN_UPDATE_MODE1(MODE)
#define USE_UPDATE_MODE1(MODE) UIUpdateMode::MODE
#define USE_UPDATE_MODE2(MODE) , USE_UPDATE_MODE1(MODE)

#define CATEGORY_GEN_UPDATE1_UI_BASIC(MODE)
#define CATEGORY_USE_UPDATE1_UI_BASIC(MODE)
#define CATEGORY_GEN_UPDATE1_UI_CONTROL(MODE) GEN_UPDATE_MODE1(MODE)
#define CATEGORY_USE_UPDATE1_UI_CONTROL(MODE) USE_UPDATE_MODE1(MODE)
#define CATEGORY_GEN_UPDATE1_UI_ITEM(MODE) GEN_UPDATE_MODE1(MODE)
#define CATEGORY_USE_UPDATE1_UI_ITEM(MODE) USE_UPDATE_MODE1(MODE)
#define CATEGORY_GEN_UPDATE2_UI_BASIC(MODE)
#define CATEGORY_USE_UPDATE2_UI_BASIC(MODE)
#define CATEGORY_GEN_UPDATE2_UI_CONTROL(MODE) GEN_UPDATE_MODE2(MODE)
#define CATEGORY_USE_UPDATE2_UI_CONTROL(MODE) USE_UPDATE_MODE2(MODE)
#define CATEGORY_GEN_UPDATE2_UI_ITEM(MODE) GEN_UPDATE_MODE2(MODE)
#define CATEGORY_USE_UPDATE2_UI_ITEM(MODE) USE_UPDATE_MODE2(MODE)

#define CATEGORY_GEN_UPDATE1_CONTROL_BASIC(MODE)
#define CATEGORY_USE_UPDATE1_CONTROL_BASIC(MODE)
#define CATEGORY_GEN_UPDATE1_CONTROL_CONTROL(MODE) GEN_UPDATE_MODE1(MODE)
#define CATEGORY_USE_UPDATE1_CONTROL_CONTROL(MODE) USE_UPDATE_MODE1(MODE)
#define CATEGORY_GEN_UPDATE1_CONTROL_ITEM(MODE)
#define CATEGORY_USE_UPDATE1_CONTROL_ITEM(MODE)
#define CATEGORY_GEN_UPDATE2_CONTROL_BASIC(MODE)
#define CATEGORY_USE_UPDATE2_CONTROL_BASIC(MODE)
#define CATEGORY_GEN_UPDATE2_CONTROL_CONTROL(MODE) GEN_UPDATE_MODE2(MODE)
#define CATEGORY_USE_UPDATE2_CONTROL_CONTROL(MODE) USE_UPDATE_MODE2(MODE)
#define CATEGORY_GEN_UPDATE2_CONTROL_ITEM(MODE)
#define CATEGORY_USE_UPDATE2_CONTROL_ITEM(MODE)

#define CATEGORY_GEN_UPDATE1_ITEM_BASIC(MODE)
#define CATEGORY_USE_UPDATE1_ITEM_BASIC(MODE)
#define CATEGORY_GEN_UPDATE1_ITEM_CONTROL(MODE)
#define CATEGORY_USE_UPDATE1_ITEM_CONTROL(MODE)
#define CATEGORY_GEN_UPDATE1_ITEM_ITEM(MODE) GEN_UPDATE_MODE1(MODE)
#define CATEGORY_USE_UPDATE1_ITEM_ITEM(MODE) USE_UPDATE_MODE1(MODE)
#define CATEGORY_GEN_UPDATE2_ITEM_BASIC(MODE)
#define CATEGORY_USE_UPDATE2_ITEM_BASIC(MODE)
#define CATEGORY_GEN_UPDATE2_ITEM_CONTROL(MODE)
#define CATEGORY_USE_UPDATE2_ITEM_CONTROL(MODE)
#define CATEGORY_GEN_UPDATE2_ITEM_ITEM(MODE) GEN_UPDATE_MODE2(MODE)
#define CATEGORY_USE_UPDATE2_ITEM_ITEM(MODE) USE_UPDATE_MODE2(MODE)

#define GEN_UPDATE1(CATEGORY, REQUEST, MODE) CATEGORY_GEN_UPDATE1_##REQUEST##_##CATEGORY(MODE)
#define GEN_UPDATE2(CATEGORY, REQUEST, MODE) CATEGORY_GEN_UPDATE2_##REQUEST##_##CATEGORY(MODE)
#define USE_UPDATE1(CATEGORY, REQUEST, MODE) CATEGORY_USE_UPDATE1_##REQUEST##_##CATEGORY(MODE)
#define USE_UPDATE2(CATEGORY, REQUEST, MODE) CATEGORY_USE_UPDATE2_##REQUEST##_##CATEGORY(MODE)

#define PRIV_LAYOUT_CONTROL_PARSE(XML, NAME, VAR, ...) \
	String _strValue = GetXmlAttribute(XML, NAME); \
	if (!(VAR.parse(_strValue, ##__VA_ARGS__))) { \
		LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME) \
		return sl_false; \
	} \
	if (VAR.isUsingData()) { \
		String f = String::concat(NAME, ":"); \
		String s = GetXmlAttribute(XML, f); \
		if (!(VAR.parseDataAccess(s))) { \
			LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, f) \
			return sl_false; \
		} \
	}
#define LAYOUT_CONTROL_PARSE(XML, NAME, VAR, ...) { PRIV_LAYOUT_CONTROL_PARSE(XML, NAME, VAR, ##__VA_ARGS__) }

#define PRIV_LAYOUT_CONTROL_GENERATE(BUF_TYPE, SETFUNC, ARG_FORMAT, ...) \
	params->sbDefine##BUF_TYPE->add(String::format(StringView::literal("%s%s->" #SETFUNC "(" ARG_FORMAT ");%n"), strTab, name, ##__VA_ARGS__));
#define LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT, ...) PRIV_LAYOUT_CONTROL_GENERATE(Init, SETFUNC, ARG_FORMAT, ##__VA_ARGS__)
#define LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, ARG_FORMAT, ...) PRIV_LAYOUT_CONTROL_GENERATE(Layout, SETFUNC, ARG_FORMAT, ##__VA_ARGS__)
#define LAYOUT_CONTROL_GENERATE_DELAYED(SETFUNC, ARG_FORMAT, ...) PRIV_LAYOUT_CONTROL_GENERATE(InitDelayed, SETFUNC, ARG_FORMAT, ##__VA_ARGS__)

#define SET_DATA_MODE_BASIC(MODE)
#define SET_DATA_MODE_CONTROL(MODE) MODE
#define SET_DATA_MODE_ITEM(MODE) MODE

#define LAYOUT_CONTROL_PARSE_GENERIC(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE(XML, NAME SUFFIX, VAR)
#define LAYOUT_CONTROL_GENERATE_GENERIC(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.isDefinedDataAccess()) { \
		params->sbDefineSetData->add(String::format(StringView::literal("%s%s->" #SETFUNC "(%s" SET_DATA_MODE_##CATEGORY(", mode") ");%n"), strTab, name, VAR.getDataAccessString())); \
	} \
	if (VAR.flagDefined) { \
		String value = VAR.getAccessString(); \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_GENERIC(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && op == SAppLayoutOperation::SimulateInit) { \
		auto& value = VAR.value; \
		view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
	}

#define LAYOUT_CONTROL_PARSE_VOID(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE(XML, NAME SUFFIX, VAR)
#define LAYOUT_CONTROL_GENERATE_VOID(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined && VAR.value) { \
		LAYOUT_CONTROL_GENERATE(SETFUNC, GEN_UPDATE1(CATEGORY, UI, Init)) \
	}
#define LAYOUT_CONTROL_SIMULATE_VOID(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && VAR.value && op == SAppLayoutOperation::SimulateInit) { \
		view->SETFUNC(USE_UPDATE1(CATEGORY, UI, Init)); \
	}

	namespace
	{
		SLIB_INLINE static sl_bool Xor(sl_bool l, sl_bool r)
		{
			if (l) {
				return !r;
			} else {
				return r;
			}
		}

		SLIB_INLINE static sl_bool IsAbsolute(sl_bool flagResizeScreen, SAppDimensionBaseValue& v)
		{
			if (flagResizeScreen) {
				return SAppDimensionValue::isAbsoluteUnit(v.unit);
			} else {
				return SAppDimensionValue::isGlobalUnit(v.unit);
			}
		}

		SLIB_INLINE static sl_bool IsAbsolute(sl_bool flagResizeScreen, SAppDrawableValue& v)
		{
			if (flagResizeScreen) {
				return v.isAbsoluteUnit();
			} else {
				return v.isGlobalUnit();
			}
		}
	}

#define LAYOUT_CONTROL_PARSE_DIMENSION(XML, NAME, SUFFIX, VAR, CHECKFUNC) \
	{ \
		PRIV_LAYOUT_CONTROL_PARSE(XML, NAME SUFFIX, VAR, this) \
		if (!(VAR.CHECKFUNC(!(params->parentResourceItem)))) { \
			LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME SUFFIX) \
			return sl_false; \
		} \
	}
#define PRIV_LAYOUT_CONTROL_GENERATE_DIMENSION(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	String value = VAR.getAccessString(); \
	if (VAR.isNeededOnLayoutFunction()) { \
		LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, ITEM, None), ##__VA_ARGS__) \
	} else { \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_GENERATE_DIMENSION(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined && !(SAppDimensionValue::isSpecialUnit(VAR.unit))) { \
		PRIV_LAYOUT_CONTROL_GENERATE_DIMENSION(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_CAN_SIMULATE_DIMENSION(VAR) Xor(IsAbsolute(m_layoutSimulationParams.flagResizeScreen, VAR), op == SAppLayoutOperation::SimulateLayout)
#define PRIV_LAYOUT_CONTROL_SIMULATE_DIMENSION(VAR, SETFUNC, CATEGORY, ...) \
	if (LAYOUT_CONTROL_CAN_SIMULATE_DIMENSION(VAR)) { \
		auto value = _getDimensionValue(VAR); \
		if (op == SAppLayoutOperation::SimulateLayout) { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, ITEM, None)); \
		} else { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
		} \
	}
#define LAYOUT_CONTROL_SIMULATE_DIMENSION(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && !(SAppDimensionValue::isSpecialUnit(VAR.unit))) { \
		PRIV_LAYOUT_CONTROL_SIMULATE_DIMENSION(VAR, SETFUNC, CATEGORY, ##__VA_ARGS__) \
	}

#define LAYOUT_CONTROL_PARSE_REFERING(XML, NAME, SUFFIX, VAR) LAYOUT_CONTROL_PARSE(XML, NAME SUFFIX, VAR, (XML).element)

#define LAYOUT_CONTROL_PARSE_STRING(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE_REFERING(XML, NAME, SUFFIX, VAR)
#define LAYOUT_CONTROL_GENERATE_STRING(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.isDefinedDataAccess()) { \
		String value; \
		if (!(_getStringDataAccessString(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		params->sbDefineSetData->add(String::format(StringView::literal("%s%s->" #SETFUNC "(%s" SET_DATA_MODE_##CATEGORY(", mode") ");%n"), strTab, name, value)); \
	} \
	if (VAR.flagDefined) { \
		String value; \
		if (!(_getStringAccessString(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_STRING(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && op == SAppLayoutOperation::SimulateInit) { \
		String value; \
		if (!(_getStringValue(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
	}

#define LAYOUT_CONTROL_PARSE_DRAWABLE(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE(XML, NAME SUFFIX, VAR, this, (XML).element)
#define LAYOUT_CONTROL_GENERATE_DRAWABLE(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.isDefinedDataAccess()) { \
		String value; \
		if (!(_getDrawableDataAccessString(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		params->sbDefineSetData->add(String::format(StringView::literal("%s%s->" #SETFUNC "(%s, mode);%n"), strTab, name, value)); \
	} \
	if (VAR.flagDefined) { \
		String value; \
		if (!(_getDrawableAccessString(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_DRAWABLE(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && LAYOUT_CONTROL_CAN_SIMULATE_DIMENSION(VAR)) { \
		Ref<Drawable> value; \
		if (!(_getDrawableValue(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		if (op == SAppLayoutOperation::SimulateLayout) { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, ITEM, None)); \
		} else { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
		} \
	}

#define LAYOUT_CONTROL_PARSE_COLOR(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE_REFERING(XML, NAME, SUFFIX, VAR)
#define LAYOUT_CONTROL_GENERATE_COLOR(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.isDefinedDataAccess()) { \
		String value; \
		if (!(_getColorDataAccessString(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		params->sbDefineSetData->add(String::format(StringView::literal("%s%s->" #SETFUNC "(%s, mode);%n"), strTab, name, value)); \
	} \
	if (VAR.flagDefined) { \
		String value; \
		if (!(_getColorAccessString(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_COLOR(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && op == SAppLayoutOperation::SimulateInit) { \
		Color value; \
		if (!(_getColorValue(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
	}

#define LAYOUT_CONTROL_PARSE_FONT(XML, NAME, SUFFIX, VAR, ...) \
	if (!(VAR.parse(&(XML), NAME, "" SUFFIX, this, !(params->parentResourceItem)))) { \
		return sl_false; \
	}
#define LAYOUT_CONTROL_GENERATE_FONT(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		String value; \
		if (!(_getFontAccessString(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		if (VAR.size.isNeededOnLayoutFunction()) { \
			LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, CONTROL, UpdateLayout) GEN_UPDATE2(CATEGORY, ITEM, None), ##__VA_ARGS__) \
		} else { \
			LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
		} \
	}
#define LAYOUT_CONTROL_SIMULATE_FONT(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && LAYOUT_CONTROL_CAN_SIMULATE_DIMENSION(VAR.size)) { \
		Ref<Font> value; \
		if (!(_getFontValue(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		if (op == SAppLayoutOperation::SimulateLayout) { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, CONTROL, UpdateLayout) USE_UPDATE2(CATEGORY, ITEM, None)); \
		} else { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
		} \
	}

#define LAYOUT_CONTROL_PARSE_BORDER(XML, NAME, SUFFIX, VAR, ...) \
	if (!(VAR.parse(&(XML), NAME, "" SUFFIX, this, !(params->parentResourceItem)))) { \
		return sl_false; \
	}
#define LAYOUT_CONTROL_GENERATE_BORDER(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		String value; \
		if (!(_getBorderAccessString(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		if (VAR.width.isNeededOnLayoutFunction()) { \
			LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, ITEM, None), ##__VA_ARGS__) \
		} else { \
			LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
		} \
	}
#define LAYOUT_CONTROL_SIMULATE_BORDER(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && LAYOUT_CONTROL_CAN_SIMULATE_DIMENSION(VAR.width)) { \
		if (VAR.flagNull) { \
			Ref<Pen> value; \
			if (op == SAppLayoutOperation::SimulateLayout) { \
				view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, ITEM, None)); \
			} else { \
				view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
			} \
		} else { \
			PenDesc value; \
			if (!(_getBorderValue(resource->fileNamespace, VAR, value))) { \
				return sl_false; \
			} \
			if (op == SAppLayoutOperation::SimulateLayout) { \
				view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, ITEM, None)); \
			} else { \
				view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
			} \
		} \
	}

#define LAYOUT_CONTROL_PARSE_MENU(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE_REFERING(XML, NAME, SUFFIX, VAR)
#define LAYOUT_CONTROL_GENERATE_MENU(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		String name, value; \
		if (!(_getMenuAccessString(resource->fileNamespace, VAR, sl_false, name, value))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_MENU(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && op == SAppLayoutOperation::SimulateInit) { \
		Ref<Menu> value; \
		if (!(_getMenuValue(resource->fileNamespace, VAR, value))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
	}
	
#define LAYOUT_CONTROL_PARSE_SIZE(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE_DIMENSION(XML, NAME, SUFFIX, VAR, checkSize)
#define LAYOUT_CONTROL_GENERATE_SIZE(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		if (VAR.unit == SAppDimensionValue::FILL || VAR.unit == SAppDimensionValue::MATCH_PARENT) { \
			String value = String::format(StringView::literal("%ff"), VAR.unit == SAppDimensionValue::FILL ? VAR.amount : -(VAR.amount)); \
			LAYOUT_CONTROL_GENERATE(SETFUNC##Filling, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
		} else if (VAR.unit == SAppDimensionValue::WRAP) { \
			StringView value = StringView::literal("sl_true"); \
			LAYOUT_CONTROL_GENERATE(SETFUNC##Wrapping, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
		} else if (VAR.unit == SAppDimensionValue::WEIGHT) { \
			String value = String::format(StringView::literal("%ff"), VAR.amount); \
			LAYOUT_CONTROL_GENERATE(SETFUNC##Weight, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
		} else { \
			PRIV_LAYOUT_CONTROL_GENERATE_DIMENSION(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ##__VA_ARGS__) \
		} \
	}
#define LAYOUT_CONTROL_SIMULATE_SIZE(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined) { \
		if (VAR.unit == SAppDimensionValue::FILL || VAR.unit == SAppDimensionValue::MATCH_PARENT) { \
			if (op == SAppLayoutOperation::SimulateInit) { \
				auto value = VAR.unit == SAppDimensionValue::FILL ? VAR.amount : -(VAR.amount); \
				view->SETFUNC##Filling(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
			} \
		} else if (VAR.unit == SAppDimensionValue::WRAP) { \
			if (op == SAppLayoutOperation::SimulateInit) { \
				sl_bool value = sl_true; \
				view->SETFUNC##Wrapping(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
			} \
		} else if (VAR.unit == SAppDimensionValue::WEIGHT) { \
			if (op == SAppLayoutOperation::SimulateInit) { \
				auto value = VAR.amount; \
				view->SETFUNC##Weight(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
			} \
		} else { \
			PRIV_LAYOUT_CONTROL_SIMULATE_DIMENSION(VAR, SETFUNC, CATEGORY, ##__VA_ARGS__) \
		} \
	}

#define LAYOUT_CONTROL_PARSE_MARGIN(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE_DIMENSION(XML, NAME, SUFFIX, VAR, checkMargin)
#define LAYOUT_CONTROL_GENERATE_MARGIN(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		if (VAR.unit == SAppDimensionValue::WEIGHT) { \
			String value = String::format(StringView::literal("%ff"), VAR.amount); \
			LAYOUT_CONTROL_GENERATE(SETFUNC##Weight, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
		} else { \
 			if (!(SAppDimensionValue::isSpecialUnit(VAR.unit))) { \
				PRIV_LAYOUT_CONTROL_GENERATE_DIMENSION(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ##__VA_ARGS__) \
			} \
		} \
	}
#define LAYOUT_CONTROL_SIMULATE_MARGIN(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined) { \
		if (VAR.unit == SAppDimensionValue::WEIGHT) { \
			if (op == SAppLayoutOperation::SimulateInit) { \
				auto& value = VAR.amount; \
				view->SETFUNC##Weight(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
			} \
		} else { \
 			if (!(SAppDimensionValue::isSpecialUnit(VAR.unit))) { \
				PRIV_LAYOUT_CONTROL_SIMULATE_DIMENSION(VAR, SETFUNC, CATEGORY, ##__VA_ARGS__) \
			} \
		} \
	}

#define LAYOUT_CONTROL_PARSE_XML(TYPE, XML, ATTR, NAME, ...) \
	{ \
		SLIB_STATIC_STRING(_name, #NAME) \
		LAYOUT_CONTROL_PARSE_##TYPE(XML, _name, , ATTR NAME, ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_PARSE_ATTR(TYPE, ATTR, NAME, ...) LAYOUT_CONTROL_PARSE_XML(TYPE, *resourceItem, ATTR, NAME, ##__VA_ARGS__)

#define PRIV_LAYOUT_CONTROL_GENERATE_ATTR(TYPE, VAR, SETFUNC, CATEGORY) LAYOUT_CONTROL_GENERATE_##TYPE(VAR, SETFUNC, CATEGORY, "%s", value)
#define LAYOUT_CONTROL_GENERATE_ATTR(TYPE, VAR, SETFUNC) PRIV_LAYOUT_CONTROL_GENERATE_ATTR(TYPE, VAR, SETFUNC, BASIC)
#define LAYOUT_CONTROL_GENERATE_UI_ATTR(TYPE, VAR, SETFUNC) PRIV_LAYOUT_CONTROL_GENERATE_ATTR(TYPE, VAR, SETFUNC, CONTROL)

#define PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, VAR, SETFUNC, CATEGORY) LAYOUT_CONTROL_SIMULATE_##TYPE(VAR, SETFUNC, CATEGORY, value)
#define LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, VAR, SETFUNC) PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, VAR, SETFUNC, BASIC)
#define LAYOUT_CONTROL_SIMULATE_UI_ATTR(TYPE, VAR, SETFUNC) PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, VAR, SETFUNC, CONTROL)

#define PRIV_LAYOUT_CONTROL_ATTR(TYPE, NAME, SETFUNC, CATEGORY, ...) \
	if (op == SAppLayoutOperation::Parse) { \
		LAYOUT_CONTROL_PARSE_ATTR(TYPE, attr->, NAME, ##__VA_ARGS__) \
	} else if (op == SAppLayoutOperation::Generate) { \
		PRIV_LAYOUT_CONTROL_GENERATE_ATTR(TYPE, attr->NAME, SETFUNC, CATEGORY) \
	} else if (IsSimulateOp(op)) { \
		PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, attr->NAME, SETFUNC, CATEGORY) \
	}
#define LAYOUT_CONTROL_ATTR(TYPE, NAME, SETFUNC, ...) PRIV_LAYOUT_CONTROL_ATTR(TYPE, NAME, SETFUNC, BASIC, ##__VA_ARGS__)
#define LAYOUT_CONTROL_UI_ATTR(TYPE, NAME, SETFUNC, ...) PRIV_LAYOUT_CONTROL_ATTR(TYPE, NAME, SETFUNC, CONTROL, ##__VA_ARGS__)

#define PRIV_LAYOUT_CONTROL_SIMULATABLE_ATTR(TYPE, NAME, SIMULATION_NAME, SETFUNC, CATEGORY, ...) \
	if (op == SAppLayoutOperation::Parse) { \
		LAYOUT_CONTROL_PARSE_ATTR(TYPE, attr->, NAME, ##__VA_ARGS__) \
		LAYOUT_CONTROL_PARSE_ATTR(TYPE, attr->, SIMULATION_NAME, ##__VA_ARGS__) \
	} else if (op == SAppLayoutOperation::Generate) { \
		PRIV_LAYOUT_CONTROL_GENERATE_ATTR(TYPE, attr->NAME, SETFUNC, CATEGORY) \
	} else if (IsSimulateOp(op)) { \
		if (attr->SIMULATION_NAME.flagDefined) { \
			PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, attr->SIMULATION_NAME, SETFUNC, CATEGORY) \
		} else { \
			PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, attr->NAME, SETFUNC, CATEGORY) \
		} \
	}
#define LAYOUT_CONTROL_SIMULATABLE_ATTR(TYPE, NAME, SIMULATION_NAME, SETFUNC, ...) PRIV_LAYOUT_CONTROL_SIMULATABLE_ATTR(TYPE, NAME, SIMULATION_NAME, SETFUNC, BASIC, ##__VA_ARGS__)
#define LAYOUT_CONTROL_UI_SIMULATABLE_ATTR(TYPE, NAME, SIMULATION_NAME, SETFUNC, ...) PRIV_LAYOUT_CONTROL_SIMULATABLE_ATTR(TYPE, NAME, SIMULATION_NAME, SETFUNC, CONTROL, ##__VA_ARGS__)

#define LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, XML, NAME, SUFFIX, VAR, ...) \
	{ \
		for (sl_size i = 0; i < CountOfArray(g_stateDefines); i++) { \
			typename RemoveConstReference<decltype(VAR)>::Type::VALUE value; \
			LAYOUT_CONTROL_PARSE_##TYPE(XML, NAME, + StringView(g_stateDefines[i].suffix) SUFFIX, value, ##__VA_ARGS__) \
			if (value.flagDefined) { \
				VAR.values.put_NoLock(g_stateDefines[i].state, value); \
			} \
		} \
	}
#define LAYOUT_CONTROL_PARSE_STATE_MAP_XML(TYPE, XML, ATTR, NAME, ...) \
	{ \
		SLIB_STATIC_STRING(_name, #NAME) \
		LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, XML, _name, , ATTR NAME, ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_PARSE_STATE_MAP_ATTR(TYPE, ATTR, NAME, ...) LAYOUT_CONTROL_PARSE_STATE_MAP_XML(TYPE, *resourceItem, ATTR, NAME, ##__VA_ARGS__)
#define LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	{ \
		for (auto&& item : VAR.values) { \
			const char* state = GetViewStateAcessString(item.key); \
			LAYOUT_CONTROL_GENERATE_##TYPE(item.value, SETFUNC, CATEGORY, ARG_FORMAT ", %s", ##__VA_ARGS__, state) \
		} \
	}
#define LAYOUT_CONTROL_SIMULATE_STATE_MAP(TYPE, VAR, SETFUNC, CATEGORY, ...) \
	{ \
		for (auto&& item : VAR.values) { \
			LAYOUT_CONTROL_SIMULATE_##TYPE(item.value, SETFUNC, CATEGORY, ##__VA_ARGS__, item.key) \
		} \
	}

#define LAYOUT_CONTROL_STATE_MAP(TYPE, NAME, SETFUNC, ...) \
	if (op == SAppLayoutOperation::Parse) { \
		LAYOUT_CONTROL_PARSE_STATE_MAP_ATTR(TYPE, attr->, NAME, ##__VA_ARGS__) \
	} else if (op == SAppLayoutOperation::Generate) { \
		LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, attr->NAME, SETFUNC, CONTROL, "%s", value) \
	} else if (IsSimulateOp(op)) { \
		LAYOUT_CONTROL_SIMULATE_STATE_MAP(TYPE, attr->NAME, SETFUNC, CONTROL, value) \
	}

#define LAYOUT_CONTROL_PROCESS_SUPER(BASE) \
	{ \
		static_cast<SAppLayout##BASE##Attributes*>(attr.get()); \
		String tempAddStatement = params->addStatement; \
		params->addStatement = sl_null; \
		if (!(_processLayoutResourceControl_##BASE(params))) { \
			return sl_false; \
		} \
		params->addStatement = tempAddStatement; \
	}

#define LAYOUT_CONTROL_ADD_STATEMENT \
	if (op == SAppLayoutOperation::Generate) { \
		params->sbDefineInit->add(params->addStatement); \
	}

#define LAYOUT_CONTROL_SET_NATIVE_WIDGET \
	if (op == SAppLayoutOperation::Generate) { \
		if (attr->isNotRequiredNative(sl_false)) { \
			if (!(attr->nativeWidget.flagDefined)) { \
				LAYOUT_CONTROL_GENERATE(setCreatingNativeWidget, "sl_false") \
			} \
		} \
	} else if (op == SAppLayoutOperation::SimulateInit) { \
		if (attr->isNotRequiredNative(sl_false)) { \
			if (!(attr->nativeWidget.flagDefined)) { \
				view->setCreatingNativeWidget(sl_false); \
			} \
		} \
	}

#define LAYOUT_CONTROL_SET_NATIVE_WIDGET_CHECK_BACKGROUND_COLOR \
	if (op == SAppLayoutOperation::Generate) { \
		if (attr->isNotRequiredNative(sl_true)) { \
			if (!(attr->nativeWidget.flagDefined)) { \
				LAYOUT_CONTROL_GENERATE(setCreatingNativeWidget, "sl_false") \
			} \
		} \
	} else if (op == SAppLayoutOperation::SimulateInit) { \
		if (attr->isNotRequiredNative(sl_true)) { \
			if (!(attr->nativeWidget.flagDefined)) { \
				view->setCreatingNativeWidget(sl_false); \
			} \
		} \
	}

#define LAYOUT_CONTROL_CHECK_VIEW_NAME(NAME) \
	if (NAME.isNotEmpty()) { \
		Ref<SAppLayoutResourceItem> item = resource->itemsByName.getValue(NAME); \
		if (item.isNull()) { \
			logError(element, g_str_error_layout_include_not_found, NAME); \
			return sl_false; \
		} \
		if (IsNoView(item->itemType)) { \
			logError(element, g_str_error_layout_include_not_found, NAME); \
			return sl_false; \
		} \
	}

	namespace
	{
		struct SAppStateDefine
		{
			ViewState state;
			const char* suffix;
		};
		SAppStateDefine g_stateDefines[] = {
			{ ViewState::All, sl_null },
			{ ViewState::Default, "Default" },
			{ ViewState::Normal, "Normal" },
			{ ViewState::Hover, "Hover" },
			{ ViewState::Pressed, "Pressed" },
			{ ViewState::Disabled, "Disabled" },
			{ ViewState::Focused, "Focused" },
			{ ViewState::FocusedNormal, "FocusedNormal" },
			{ ViewState::FocusedHover, "FocusedHover" },
			{ ViewState::FocusedPressed, "FocusedPressed" },
			{ ViewState::Selected, "Selected" },
			{ ViewState::SelectedNormal, "SelectedNormal" },
			{ ViewState::SelectedHover, "SelectedHover" },
			{ ViewState::SelectedPressed, "SelectedPressed" }
		};

		static const char* GetViewStateAcessString(ViewState state)
		{
			switch (state) {
				case ViewState::Default:
					return "slib::ViewState::Default";
				case ViewState::Normal:
					return "slib::ViewState::Normal";
				case ViewState::Hover:
					return "slib::ViewState::Hover";
				case ViewState::Pressed:
					return "slib::ViewState::Pressed";
				case ViewState::Disabled:
					return "slib::ViewState::Disabled";
				case ViewState::Focused:
					return "slib::ViewState::Focused";
				case ViewState::FocusedNormal:
					return "slib::ViewState::FocusedNormal";
				case ViewState::FocusedHover:
					return "slib::ViewState::FocusedHover";
				case ViewState::FocusedPressed:
					return "slib::ViewState::FocusedPressed";
				case ViewState::Selected:
					return "slib::ViewState::Selected";
				case ViewState::SelectedNormal:
					return "slib::ViewState::SelectedNormal";
				case ViewState::SelectedHover:
					return "slib::ViewState::SelectedHover";
				case ViewState::SelectedPressed:
					return "slib::ViewState::SelectedPressed";
				default:
					return "slib::ViewState::All";
			}
		}
	}

	BEGIN_PROCESS_LAYOUT_CONTROL(View, View)
	{
		sl_bool flagRoot = params->parentResourceItem == sl_null;
		sl_bool flagView = !flagRoot || resource->layoutType != SAppLayoutType::Window;

		if (flagView) {
			LAYOUT_CONTROL_ATTR(STRING, id, setId)
			LAYOUT_CONTROL_UI_ATTR(SIZE, width, setWidth)
			LAYOUT_CONTROL_UI_ATTR(SIZE, height, setHeight)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, left, setLeft, checkPosition)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, top, setTop, checkPosition)

			if (op == SAppLayoutOperation::Parse) {

				attr->leftMode = PositionMode::Free;
				attr->topMode = PositionMode::Free;
				attr->rightMode = PositionMode::Free;
				attr->bottomMode = PositionMode::Free;

				SAppAlignmentValue align;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , align)
				if (align.flagDefined) {
					Alignment h = align.value & Alignment::HorizontalMask;
					if (h == Alignment::Right) {
						attr->rightMode = PositionMode::ParentEdge;
					} else if (h == Alignment::Center) {
						attr->leftMode = PositionMode::CenterInParent;
					} else if (h == Alignment::Left) {
						attr->leftMode = PositionMode::ParentEdge;
					}
					Alignment v = align.value & Alignment::VerticalMask;
					if (v == Alignment::Bottom) {
						attr->bottomMode = PositionMode::ParentEdge;
					} else if (v == Alignment::Middle) {
						attr->topMode = PositionMode::CenterInParent;
					} else if (v == Alignment::Top) {
						attr->topMode = PositionMode::ParentEdge;
					}
				}

#define LAYOUT_CONTROL_VIEW_PARSE_ALIGN_LAYOUT(NAME, PREFIX, PARENT_POS, OTHER_POS) \
				SAppAlignLayoutValue NAME; \
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , NAME) \
				if (NAME.flagDefined) { \
					if (NAME.flagAlignParent) { \
						attr->PREFIX##Mode = PositionMode::PARENT_POS; \
					} else { \
						if (flagRoot) { \
							LOG_ERROR_LAYOUT_CONTROL_ATTR(#NAME) \
							return sl_false; \
						} \
						attr->PREFIX##Mode = PositionMode::OTHER_POS; \
						attr->PREFIX##ReferingView = NAME.referingView; \
					} \
				}

#define LAYOUT_CONTROL_VIEW_PARSE_NAME(NAME, PREFIX, POS) \
				SAppNameValue NAME; \
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , NAME) \
				if (NAME.flagDefined) { \
					if (flagRoot) { \
						LOG_ERROR_LAYOUT_CONTROL_ATTR(#NAME) \
						return sl_false; \
					} \
					attr->PREFIX##Mode = PositionMode::POS; \
					attr->PREFIX##ReferingView = NAME.value; \
				}

				LAYOUT_CONTROL_VIEW_PARSE_ALIGN_LAYOUT(alignLeft, left, ParentEdge, OtherStart)
				LAYOUT_CONTROL_VIEW_PARSE_NAME(toRightOf, left, OtherEnd)
				LAYOUT_CONTROL_VIEW_PARSE_ALIGN_LAYOUT(alignTop, top, ParentEdge, OtherStart)
				LAYOUT_CONTROL_VIEW_PARSE_NAME(below, top, OtherEnd)
				LAYOUT_CONTROL_VIEW_PARSE_ALIGN_LAYOUT(alignRight, right, ParentEdge, OtherEnd)
				LAYOUT_CONTROL_VIEW_PARSE_NAME(toLeftOf, right, OtherStart)
				LAYOUT_CONTROL_VIEW_PARSE_ALIGN_LAYOUT(alignBottom, bottom, ParentEdge, OtherEnd)
				LAYOUT_CONTROL_VIEW_PARSE_NAME(above, bottom, OtherStart)
				LAYOUT_CONTROL_VIEW_PARSE_ALIGN_LAYOUT(centerHorizontal, left, CenterInParent, CenterInOther)
				LAYOUT_CONTROL_VIEW_PARSE_ALIGN_LAYOUT(centerVertical, top, CenterInParent, CenterInOther)
				LAYOUT_CONTROL_VIEW_PARSE_ALIGN_LAYOUT(alignCenter, left, CenterInParent, CenterInOther)
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , alignCenter)
				if (alignCenter.flagDefined) {
					attr->topMode = attr->leftMode;
					attr->topReferingView = attr->leftReferingView;
				}

				if (params->parentResourceItem && resourceItem->itemType != SAppLayoutItemType::Import && params->parentResourceItem->itemType != SAppLayoutItemType::Table) {
					if (!(attr->left.flagDefined)) {
						if (attr->leftMode == PositionMode::Free && attr->rightMode == PositionMode::Free) {
							attr->leftMode = PositionMode::ParentEdge;
						}
					}
					if (!(attr->top.flagDefined)) {
						if (attr->topMode == PositionMode::Free && attr->bottomMode == PositionMode::Free) {
							attr->topMode = PositionMode::ParentEdge;
						}
					}
				}

			} else {
				LAYOUT_CONTROL_CHECK_VIEW_NAME(attr->leftReferingView)
				LAYOUT_CONTROL_CHECK_VIEW_NAME(attr->topReferingView)
				LAYOUT_CONTROL_CHECK_VIEW_NAME(attr->rightReferingView)
				LAYOUT_CONTROL_CHECK_VIEW_NAME(attr->bottomReferingView)
			}
			
			if (op == SAppLayoutOperation::Generate) {
				if (attr->leftMode == PositionMode::CenterInParent) {
					LAYOUT_CONTROL_GENERATE(setCenterHorizontal, "slib::UIUpdateMode::Init")
				} else if (attr->leftMode == PositionMode::CenterInOther) {
					LAYOUT_CONTROL_GENERATE_DELAYED(setAlignCenterHorizontal, "%s, slib::UIUpdateMode::Init", attr->leftReferingView)
				} else {
					if (attr->leftMode == PositionMode::ParentEdge) {
						LAYOUT_CONTROL_GENERATE(setAlignParentLeft, "slib::UIUpdateMode::Init")
					} else if (attr->leftMode == PositionMode::OtherStart) {
						LAYOUT_CONTROL_GENERATE_DELAYED(setAlignLeft, "%s, slib::UIUpdateMode::Init", attr->leftReferingView)
					} else if (attr->leftMode == PositionMode::OtherEnd) {
						LAYOUT_CONTROL_GENERATE_DELAYED(setRightOf, "%s, slib::UIUpdateMode::Init", attr->leftReferingView)
					}
					if (attr->rightMode == PositionMode::ParentEdge) {
						LAYOUT_CONTROL_GENERATE(setAlignParentRight, "slib::UIUpdateMode::Init")
					} else if (attr->rightMode == PositionMode::OtherStart) {
						LAYOUT_CONTROL_GENERATE_DELAYED(setLeftOf, "%s, slib::UIUpdateMode::Init", attr->rightReferingView)
					} else if (attr->rightMode == PositionMode::OtherEnd) {
						LAYOUT_CONTROL_GENERATE_DELAYED(setAlignRight, "%s, slib::UIUpdateMode::Init", attr->rightReferingView)
					}
				}
				if (attr->topMode == PositionMode::CenterInParent) {
					LAYOUT_CONTROL_GENERATE(setCenterVertical, "slib::UIUpdateMode::Init")
				} else if (attr->topMode == PositionMode::CenterInOther) {
					LAYOUT_CONTROL_GENERATE_DELAYED(setAlignCenterVertical, "%s, slib::UIUpdateMode::Init", attr->topReferingView)
				} else {
					if (attr->topMode == PositionMode::ParentEdge) {
						LAYOUT_CONTROL_GENERATE(setAlignParentTop, "slib::UIUpdateMode::Init")
					} else if (attr->topMode == PositionMode::OtherStart) {
						LAYOUT_CONTROL_GENERATE_DELAYED(setAlignTop, "%s, slib::UIUpdateMode::Init", attr->topReferingView)
					} else if (attr->topMode == PositionMode::OtherEnd) {
						LAYOUT_CONTROL_GENERATE_DELAYED(setBelow, "%s, slib::UIUpdateMode::Init", attr->topReferingView)
					}
					if (attr->bottomMode == PositionMode::ParentEdge) {
						LAYOUT_CONTROL_GENERATE(setAlignParentBottom, "slib::UIUpdateMode::Init")
					} else if (attr->bottomMode == PositionMode::OtherStart) {
						LAYOUT_CONTROL_GENERATE_DELAYED(setAbove, "%s, slib::UIUpdateMode::Init", attr->bottomReferingView)
					} else if (attr->bottomMode == PositionMode::OtherEnd) {
						LAYOUT_CONTROL_GENERATE_DELAYED(setAlignBottom, "%s, slib::UIUpdateMode::Init", attr->bottomReferingView)
					}
				}
			} else if (IsSimulateOp(op)) {
				if (attr->leftMode == PositionMode::CenterInParent) {
					if (op == SAppLayoutOperation::SimulateInit) {
						view->setCenterHorizontal(UIUpdateMode::Init);
					}
				} else if (attr->leftMode == PositionMode::CenterInOther) {
					if (op == SAppLayoutOperation::SimulateLayout) {
						view->setAlignCenterHorizontal(CastRef<View>(params->simulator->getViewItemByName(attr->leftReferingView)), UIUpdateMode::Init);
					}
				} else {
					if (attr->leftMode == PositionMode::ParentEdge) {
						if (op == SAppLayoutOperation::SimulateInit) {
							view->setAlignParentLeft(UIUpdateMode::Init);
						}
					} else if (attr->leftMode == PositionMode::OtherStart) {
						if (op == SAppLayoutOperation::SimulateLayout) {
							view->setAlignLeft(CastRef<View>(params->simulator->getViewItemByName(attr->leftReferingView)), UIUpdateMode::Init);
						}
					} else if (attr->leftMode == PositionMode::OtherEnd) {
						if (op == SAppLayoutOperation::SimulateLayout) {
							view->setRightOf(CastRef<View>(params->simulator->getViewItemByName(attr->leftReferingView)), UIUpdateMode::Init);
						}
					}
					if (attr->rightMode == PositionMode::ParentEdge) {
						if (op == SAppLayoutOperation::SimulateInit) {
							view->setAlignParentRight(UIUpdateMode::Init);
						}
					} else if (attr->rightMode == PositionMode::OtherStart) {
						if (op == SAppLayoutOperation::SimulateLayout) {
							view->setLeftOf(CastRef<View>(params->simulator->getViewItemByName(attr->rightReferingView)), UIUpdateMode::Init);
						}
					} else if (attr->rightMode == PositionMode::OtherEnd) {
						if (op == SAppLayoutOperation::SimulateLayout) {
							view->setAlignRight(CastRef<View>(params->simulator->getViewItemByName(attr->rightReferingView)), UIUpdateMode::Init);
						}
					}
				}
				if (attr->topMode == PositionMode::CenterInParent) {
					if (op == SAppLayoutOperation::SimulateInit) {
						view->setCenterVertical(UIUpdateMode::Init);
					}
				} else if (attr->topMode == PositionMode::CenterInOther) {
					view->setAlignCenterVertical(CastRef<View>(params->simulator->getViewItemByName(attr->topReferingView)), UIUpdateMode::Init);
				} else {
					if (attr->topMode == PositionMode::ParentEdge) {
						if (op == SAppLayoutOperation::SimulateInit) {
							view->setAlignParentTop(UIUpdateMode::Init);
						}
					} else if (attr->topMode == PositionMode::OtherStart) {
						view->setAlignTop(CastRef<View>(params->simulator->getViewItemByName(attr->topReferingView)), UIUpdateMode::Init);
					} else if (attr->topMode == PositionMode::OtherEnd) {
						view->setBelow(CastRef<View>(params->simulator->getViewItemByName(attr->topReferingView)), UIUpdateMode::Init);
					}
					if (attr->bottomMode == PositionMode::ParentEdge) {
						if (op == SAppLayoutOperation::SimulateInit) {
							view->setAlignParentBottom(UIUpdateMode::Init);
						}
					} else if (attr->bottomMode == PositionMode::OtherStart) {
						if (op == SAppLayoutOperation::SimulateLayout) {
							view->setAbove(CastRef<View>(params->simulator->getViewItemByName(attr->bottomReferingView)), UIUpdateMode::Init);
						}
					} else if (attr->bottomMode == PositionMode::OtherEnd) {
						if (op == SAppLayoutOperation::SimulateLayout) {
							view->setAlignBottom(CastRef<View>(params->simulator->getViewItemByName(attr->bottomReferingView)), UIUpdateMode::Init);
						}
					}
				}
			}

			LAYOUT_CONTROL_UI_ATTR(DIMENSION, minWidth, setMinimumWidth, checkScalarSize)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, maxWidth, setMaximumWidth, checkScalarSize)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, minHeight, setMinimumHeight, checkScalarSize)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, maxHeight, setMaximumHeight, checkScalarSize)

			LAYOUT_CONTROL_UI_ATTR(GENERIC, aspectRatio, setAspectRatio)

			if (op == SAppLayoutOperation::Parse) {
				if (!flagRoot || resource->layoutType == SAppLayoutType::View) {
					if (!(attr->width.flagDefined) && attr->leftMode != PositionMode::Free && attr->rightMode != PositionMode::Free) {
						attr->width.flagDefined = sl_true;
						attr->width.amount = 1;
						attr->width.unit = SAppDimensionValue::MATCH_PARENT;
					}
					if (!(attr->height.flagDefined) && attr->topMode != PositionMode::Free && attr->bottomMode != PositionMode::Free) {
						attr->height.flagDefined = sl_true;
						attr->height.amount = 1;
						attr->height.unit = SAppDimensionValue::MATCH_PARENT;
					}
					if (resourceItem->itemType != SAppLayoutItemType::Import && resourceItem->itemType != SAppLayoutItemType::Drawer && resourceItem->itemType != SAppLayoutItemType::Image) {
						if (attr->aspectRatio.flagDefined) {
							if (!(attr->width.flagDefined) && !(attr->height.flagDefined)) {
								attr->width.flagDefined = sl_true;
								attr->width.amount = 1;
								attr->width.unit = SAppDimensionValue::WRAP;
							}
						} else {
							if (!(attr->width.flagDefined)) {
								attr->width.flagDefined = sl_true;
								attr->width.amount = 1;
								attr->width.unit = SAppDimensionValue::WRAP;
							}
							if (!(attr->height.flagDefined)) {
								attr->height.flagDefined = sl_true;
								attr->height.amount = 1;
								attr->height.unit = SAppDimensionValue::WRAP;
							}
						}
					}
				}
			}
			if (op == SAppLayoutOperation::Generate) {
				if (attr->aspectRatio.flagDefined) {
					if (attr->width.flagDefined) {
						if (!(attr->height.flagDefined)) {
							LAYOUT_CONTROL_GENERATE(setAspectRatioMode, "slib::AspectRatioMode::AdjustHeight, slib::UIUpdateMode::Init")
						}
					} else {
						if (attr->height.flagDefined) {
							LAYOUT_CONTROL_GENERATE(setAspectRatioMode, "slib::AspectRatioMode::AdjustWidth, slib::UIUpdateMode::Init")
						}
					}
				}
			} else if (op == SAppLayoutOperation::SimulateInit) {
				if (attr->aspectRatio.flagDefined) {
					if (attr->width.flagDefined) {
						if (!(attr->height.flagDefined)) {
							view->setAspectRatioMode(AspectRatioMode::AdjustHeight, slib::UIUpdateMode::Init);
						}
					} else {
						if (attr->height.flagDefined) {
							view->setAspectRatioMode(AspectRatioMode::AdjustWidth, slib::UIUpdateMode::Init);
						}
					}
				}
			}

			LAYOUT_CONTROL_UI_ATTR(MARGIN, marginLeft, setMarginLeft)
			LAYOUT_CONTROL_UI_ATTR(MARGIN, marginTop, setMarginTop)
			LAYOUT_CONTROL_UI_ATTR(MARGIN, marginRight, setMarginRight)
			LAYOUT_CONTROL_UI_ATTR(MARGIN, marginBottom, setMarginBottom)
			if (op == SAppLayoutOperation::Parse) {
				SAppDimensionValue margin;
				LAYOUT_CONTROL_PARSE_ATTR(MARGIN, , margin)
				if (margin.flagDefined) {
					if (!(attr->marginLeft.flagDefined)) {
						attr->marginLeft = margin;
					}
					if (!(attr->marginTop.flagDefined)) {
						attr->marginTop = margin;
					}
					if (!(attr->marginRight.flagDefined)) {
						attr->marginRight = margin;
					}
					if (!(attr->marginBottom.flagDefined)) {
						attr->marginBottom = margin;
					}
				}
			}
		}

		LAYOUT_CONTROL_UI_ATTR(MARGIN, paddingLeft, setPaddingLeft)
		LAYOUT_CONTROL_UI_ATTR(MARGIN, paddingTop, setPaddingTop)
		LAYOUT_CONTROL_UI_ATTR(MARGIN, paddingRight, setPaddingRight)
		LAYOUT_CONTROL_UI_ATTR(MARGIN, paddingBottom, setPaddingBottom)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue padding;
			LAYOUT_CONTROL_PARSE_ATTR(MARGIN, , padding)
			if (padding.flagDefined) {
				if (!(attr->paddingLeft.flagDefined)) {
					attr->paddingLeft = padding;
				}
				if (!(attr->paddingTop.flagDefined)) {
					attr->paddingTop = padding;
				}
				if (!(attr->paddingRight.flagDefined)) {
					attr->paddingRight = padding;
				}
				if (!(attr->paddingBottom.flagDefined)) {
					attr->paddingBottom = padding;
				}
			}
		}

		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, background, setBackground)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, backgroundScale, setBackgroundScaleMode)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, backgroundAlign, setBackgroundAlignment)

		if (flagView) {
			SAppBooleanValue& border = attr->nativeBorder;
			if (op == SAppLayoutOperation::Parse) {
				String value = GetXmlAttribute(*resourceItem, "border");
				if (border.parse(value)) {
					resourceItem->element->removeAttribute("border");
				}
			} else if (op == SAppLayoutOperation::Generate) {
				LAYOUT_CONTROL_GENERATE_UI_ATTR(GENERIC, border, setBorder)
			} else if (IsSimulateOp(op)) {
				LAYOUT_CONTROL_SIMULATE_UI_ATTR(GENERIC, border, setBorder)
			}
			LAYOUT_CONTROL_STATE_MAP(BORDER, border, setBorder)
			if (op == SAppLayoutOperation::Parse) {
				SAppBorderValue::normalizeStateMap(attr->border);
			}
		}

		LAYOUT_CONTROL_UI_ATTR(GENERIC, drawing, setDrawing)

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, boundRadius, setBoundRadius, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, boundRadiusX, setBoundRadiusX, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, boundRadiusY, setBoundRadiusY, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, boundShape, setBoundShape)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, contentRadius, setContentRadius, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, contentRadiusX, setContentRadiusX, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, contentRadiusY, setContentRadiusY, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, contentShape, setContentShape)
		LAYOUT_CONTROL_STATE_MAP(COLOR, paddingColor, setPaddingColor)

		LAYOUT_CONTROL_UI_ATTR(FONT, font, setFont)
		if (op == SAppLayoutOperation::Parse) {
			if (params->parentResourceItem) {
				if (!(IsNoView(params->parentResourceItem->itemType))) {
					attr->font.inheritFrom(((SAppLayoutViewAttributes*)(params->parentResourceItem->attrs.get()))->font);
				}
			}
		}
		LAYOUT_CONTROL_UI_ATTR(GENERIC, fontSizeWeight, setFontSizeWeight)

		if (flagView) {
			LAYOUT_CONTROL_UI_ATTR(GENERIC, alpha, setAlpha)
			LAYOUT_CONTROL_UI_ATTR(COLOR, colorKey, setColorKey)
			LAYOUT_CONTROL_UI_ATTR(GENERIC, antiAlias, setAntiAlias)
			LAYOUT_CONTROL_UI_ATTR(GENERIC, backgroundAntiAlias, setBackgroundAntiAlias)
			LAYOUT_CONTROL_UI_ATTR(GENERIC, contentAntiAlias, setContentAntiAlias)
		}

		LAYOUT_CONTROL_UI_ATTR(GENERIC, opaque, setOpaque)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, layer, setLayer)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, shadowOpacity, setShadowOpacity)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, shadowRadius, setShadowRadius, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, shadowOffsetX, setShadowOffsetX, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, shadowOffsetY, setShadowOffsetY, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(COLOR, shadowColor, setShadowColor)

		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_PARSE_ATTR(GENERIC, attr->, scrolling)
			if (!(attr->scrolling.flagDefined)) {
				if (resourceItem->itemTypeName == "hscroll") {
					attr->scrolling.flagDefined = sl_true;
					attr->scrolling.horizontal = sl_true;
					attr->scrolling.vertical = sl_false;
				} else if (resourceItem->itemTypeName == "vscroll") {
					attr->scrolling.flagDefined = sl_true;
					attr->scrolling.horizontal = sl_false;
					attr->scrolling.vertical = sl_true;
				}
			}
		} else if (op == SAppLayoutOperation::Generate) {
			if (attr->scrolling.flagDefined) {
				LAYOUT_CONTROL_GENERATE(setHorizontalScrolling, "%s, slib::UIUpdateMode::Init", attr->scrolling.horizontal?"sl_true":"sl_false")
				LAYOUT_CONTROL_GENERATE(setVerticalScrolling, "%s, slib::UIUpdateMode::Init", attr->scrolling.vertical?"sl_true":"sl_false")
			}
		} else if (op == SAppLayoutOperation::SimulateInit) {
			if (attr->scrolling.flagDefined) {
				view->setHorizontalScrolling(attr->scrolling.horizontal?sl_true:sl_false, UIUpdateMode::Init);
				view->setVerticalScrolling(attr->scrolling.vertical?sl_true:sl_false, UIUpdateMode::Init);
			}
		}

		LAYOUT_CONTROL_ATTR(DIMENSION, contentWidth, setContentWidth, checkScalarSize)
		LAYOUT_CONTROL_ATTR(DIMENSION, contentHeight, setContentHeight, checkScalarSize)

		LAYOUT_CONTROL_ATTR(GENERIC, paging, setPaging)
		LAYOUT_CONTROL_ATTR(DIMENSION, pageWidth, setPageWidth, checkScalarSize)
		LAYOUT_CONTROL_ATTR(DIMENSION, pageHeight, setPageHeight, checkScalarSize)

		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_PARSE_ATTR(GENERIC, attr->, scrollBars)
		} else if (op == SAppLayoutOperation::Generate) {
			if (attr->scrollBars.flagDefined) {
				LAYOUT_CONTROL_GENERATE(setScrollBarsVisible, "%s, %s, slib::UIUpdateMode::Init", attr->scrollBars.horizontalScrollBar ? "sl_true" : "sl_false", attr->scrollBars.verticalScrollBar ? "sl_true" : "sl_false")
			}
		} else if (op == SAppLayoutOperation::SimulateInit) {
			if (attr->scrollBars.flagDefined) {
				view->setScrollBarsVisible(attr->scrollBars.horizontalScrollBar, attr->scrollBars.verticalScrollBar, UIUpdateMode::Init);
			}
		}
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, hscrollThumb, setHorizontalScrollThumb)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, hscrollTrack, setHorizontalScrollTrack)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, vscrollThumb, setVerticalScrollThumb)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, vscrollTrack, setVerticalScrollTrack)
		if (op == SAppLayoutOperation::Parse) {
			SAppStateMap<SAppDrawableValue> scrollThumb;
			LAYOUT_CONTROL_PARSE_STATE_MAP_ATTR(DRAWABLE, , scrollThumb)
			attr->hscrollThumb.mergeDefault(scrollThumb);
			attr->vscrollThumb.mergeDefault(scrollThumb);
			SAppStateMap<SAppDrawableValue> scrollTrack;
			LAYOUT_CONTROL_PARSE_STATE_MAP_ATTR(DRAWABLE, , scrollTrack)
			attr->hscrollTrack.mergeDefault(scrollTrack);
			attr->vscrollTrack.mergeDefault(scrollTrack);
		}
		LAYOUT_CONTROL_ATTR(GENERIC, scrollingByMouse, setContentScrollingByMouse)
		LAYOUT_CONTROL_ATTR(GENERIC, scrollingByTouch, setContentScrollingByTouch)
		LAYOUT_CONTROL_ATTR(GENERIC, scrollingByMouseWheel, setContentScrollingByMouseWheel)
		LAYOUT_CONTROL_ATTR(GENERIC, scrollingByKeyboard, setContentScrollingByKeyboard)
		LAYOUT_CONTROL_ATTR(GENERIC, autoHideScrollBar, setAutoHideScrollBar)
		LAYOUT_CONTROL_ATTR(GENERIC, smoothScrolling, setSmoothContentScrolling)

		LAYOUT_CONTROL_ATTR(GENERIC, childFocusedState, setUsingChildFocusedState)
		LAYOUT_CONTROL_ATTR(GENERIC, focusable, setFocusable)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, focus, setFocus)
		LAYOUT_CONTROL_ATTR(GENERIC, hitTest, setHitTestable)
		LAYOUT_CONTROL_ATTR(GENERIC, touchMultipleChildren, setTouchMultipleChildren)
		LAYOUT_CONTROL_ATTR(GENERIC, cursor, setCursor)
		LAYOUT_CONTROL_ATTR(STRING, toolTip, setToolTip)
		LAYOUT_CONTROL_ATTR(GENERIC, ime, setUsingIME)
		if (flagView) {
			LAYOUT_CONTROL_ATTR(GENERIC, tabStop, setTabStopEnabled)
		}

#define LAYOUT_CONTROL_VIEW_TAB_STOP(NAME, SETTER) \
		if (op == SAppLayoutOperation::Parse) { \
			attr->NAME = resourceItem->getXmlAttribute(#NAME); \
		} else if (op == SAppLayoutOperation::Generate) { \
			if (attr->NAME.isNotEmpty()) { \
				LAYOUT_CONTROL_CHECK_VIEW_NAME(attr->NAME) \
				LAYOUT_CONTROL_GENERATE_DELAYED(SETTER, "%s", attr->NAME) \
			} \
		} else if (op == SAppLayoutOperation::SimulateLayout) { \
			if (attr->NAME.isNotEmpty()) { \
				Ref<View> refer = CastRef<View>(params->simulator->getViewItemByName(attr->NAME)); \
				if (refer.isNull()) { \
					logError(element, g_str_error_resource_layout_failed_load_reference_view, attr->NAME); \
					return sl_false; \
				} \
				view->SETTER(refer); \
			} \
		}

		if (!flagRoot) {
			LAYOUT_CONTROL_VIEW_TAB_STOP(nextTabStop, setNextTabStop)
			LAYOUT_CONTROL_VIEW_TAB_STOP(previousTabStop, setPreviousTabStop)
		}

		if (flagView) {
			LAYOUT_CONTROL_UI_SIMULATABLE_ATTR(GENERIC, visibility, simulationVisibility, setVisibility)
			LAYOUT_CONTROL_UI_SIMULATABLE_ATTR(GENERIC, visible, simulationVisible, setVisible)
			LAYOUT_CONTROL_UI_ATTR(GENERIC, enabled, setEnabled)
			LAYOUT_CONTROL_UI_ATTR(GENERIC, clipping, setClipping)
			if (op == SAppLayoutOperation::SimulateLayout) {
				if (attr->clipping.flagDefined) {
					if (attr->boundShape.flagDefined || attr->boundRadius.flagDefined || attr->boundRadiusX.flagDefined || attr->boundRadiusY.flagDefined) {
						view->setClipping(attr->clipping.value, UIUpdateMode::None);
					}
				}
			}
		}

		if (flagView) {
			LAYOUT_CONTROL_ATTR(GENERIC, instance, setCreatingInstance)
			LAYOUT_CONTROL_ATTR(GENERIC, nativeWidget, setCreatingNativeWidget)
			LAYOUT_CONTROL_ATTR(GENERIC, nativeLayer, setCreatingNativeLayer)
			LAYOUT_CONTROL_ATTR(GENERIC, largeContent, setCreatingLargeContent)
			LAYOUT_CONTROL_ATTR(GENERIC, emptyContent, setCreatingEmptyContent)
		}
		LAYOUT_CONTROL_ATTR(GENERIC, childInstances, setCreatingChildInstances)

		LAYOUT_CONTROL_ATTR(GENERIC, okCancelEnabled, setOkCancelEnabled)
		LAYOUT_CONTROL_ATTR(VOID, ok, setOkOnClick)
		LAYOUT_CONTROL_ATTR(VOID, cancel, setCancelOnClick)
		LAYOUT_CONTROL_ATTR(VOID, close, setCloseOnClick)
		LAYOUT_CONTROL_ATTR(VOID, minimize, setMinimizeOnClick)
		LAYOUT_CONTROL_ATTR(VOID, maximize, setMaximizeOnClick)
		LAYOUT_CONTROL_VIEW_TAB_STOP(sendFocus, sendFocusOnClick)
		LAYOUT_CONTROL_ATTR(VOID, acceptMouse, setAcceptOnMouseEvent)
		LAYOUT_CONTROL_ATTR(VOID, movingWindow, setMovingWindowOnMouseEvent)
		LAYOUT_CONTROL_ATTR(GENERIC, mnemonicKey, setMnemonicKey)
		LAYOUT_CONTROL_ATTR(GENERIC, keepKeyboard, setKeepKeyboard)
		LAYOUT_CONTROL_ATTR(GENERIC, playSoundOnClick, setPlaySoundOnClick)
		if (flagView) {
			LAYOUT_CONTROL_ATTR(GENERIC, clientEdge, setClientEdge)
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Window, View)
	{
		Window* view = params->window;

		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_PARSE_ATTR(MENU, attr->, menu)
		} else if (op == SAppLayoutOperation::Generate) {
			if (attr->menu.flagDefined) {
				String menuName, value;
				if (!(_getMenuAccessString(resource->fileNamespace, attr->menu, sl_true, menuName, value))) {
					return sl_false;
				}
				params->sbDeclare->add(String::format("\t\t\tslib::Ref<menu::%s> menu;%n", menuName));
				params->sbDefineInit->add(String::format("%smenu = %s;%n%s%s->setMenu(menu->root);%n", strTab, value, strTab, name));
			}
		} else if (op == SAppLayoutOperation::SimulateInit) {
			if (attr->menu.flagDefined) {
				Ref<Menu> value;
				if (!(_getMenuValue(resource->fileNamespace, attr->menu, value))) {
					return sl_false;
				}
				if (value.isNotNull()) {
					view->setMenu(value);
				}
			}
		}

		LAYOUT_CONTROL_ATTR(DIMENSION, minWidth, setMinimumWidth, checkForWindow)
		LAYOUT_CONTROL_ATTR(DIMENSION, maxWidth, setMaximumWidth, checkForWindow)
		LAYOUT_CONTROL_ATTR(DIMENSION, minHeight, setMinimumHeight, checkForWindow)
		LAYOUT_CONTROL_ATTR(DIMENSION, maxHeight, setMaximumHeight, checkForWindow)
		LAYOUT_CONTROL_ATTR(GENERIC, aspectRatio, setAspectRatio)
		LAYOUT_CONTROL_ATTR(GENERIC, minAspectRatio, setMinimumAspectRatio)
		LAYOUT_CONTROL_ATTR(GENERIC, maxAspectRatio, setMaximumAspectRatio)

		LAYOUT_CONTROL_ATTR(GENERIC, minimized, setMinimized)
		LAYOUT_CONTROL_ATTR(GENERIC, maximized, setMaximized)
		LAYOUT_CONTROL_ATTR(GENERIC, visible, setVisible)
		LAYOUT_CONTROL_ATTR(GENERIC, alwaysOnTop, setAlwaysOnTop)
		LAYOUT_CONTROL_ATTR(GENERIC, closeButton, setCloseButtonEnabled)
		LAYOUT_CONTROL_ATTR(GENERIC, minimizeButton, setMinimizeButtonEnabled)
		LAYOUT_CONTROL_ATTR(GENERIC, maximizeButton, setMaximizeButtonEnabled)
		LAYOUT_CONTROL_ATTR(GENERIC, fullScreenButton, setFullScreenButtonEnabled)
		LAYOUT_CONTROL_ATTR(GENERIC, resizable, setResizable)
		LAYOUT_CONTROL_ATTR(GENERIC, layered, setLayered)
		LAYOUT_CONTROL_ATTR(GENERIC, alpha, setAlpha)
		LAYOUT_CONTROL_ATTR(COLOR, colorKey, setColorKey)
		LAYOUT_CONTROL_ATTR(GENERIC, transparent, setTransparent)
		LAYOUT_CONTROL_ATTR(GENERIC, taskbar, setVisibleInTaskbar)
		LAYOUT_CONTROL_ATTR(GENERIC, excludeFromCapture, setExcludingFromCapture)
		LAYOUT_CONTROL_ATTR(COLOR, backgroundColor, setBackgroundColor)

		LAYOUT_CONTROL_ATTR(GENERIC, modal, setModal)
		LAYOUT_CONTROL_ATTR(GENERIC, dialog, setDialog)
		LAYOUT_CONTROL_ATTR(GENERIC, borderless, setBorderless)
		LAYOUT_CONTROL_ATTR(GENERIC, titleBar, setTitleBarVisible)
		LAYOUT_CONTROL_ATTR(GENERIC, fullScreen, setFullScreen)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, centerScreen, setCenterScreen)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, marginLeft, setMarginLeft, checkForWindow)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, marginTop, setMarginTop, checkForWindow)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, marginRight, setMarginRight, checkForWindow)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, marginBottom, setMarginBottom, checkForWindow)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue margin;
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, , margin, checkForWindow)
			if (margin.flagDefined) {
				if (!(attr->marginLeft.flagDefined)) {
					attr->marginLeft = margin;
				}
				if (!(attr->marginTop.flagDefined)) {
					attr->marginTop = margin;
				}
				if (!(attr->marginRight.flagDefined)) {
					attr->marginRight = margin;
				}
				if (!(attr->marginBottom.flagDefined)) {
					attr->marginBottom = margin;
				}
			}
		}

		LAYOUT_CONTROL_ATTR(STRING, title, setTitle)
		LAYOUT_CONTROL_ATTR(DRAWABLE, icon, setIcon)
		LAYOUT_CONTROL_ATTR(STRING, iconResource, setIconResource)
		LAYOUT_CONTROL_ATTR(DIMENSION, left, setLeft, checkForWindow)
		LAYOUT_CONTROL_ATTR(DIMENSION, top, setTop, checkForWindow)
		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, attr->, width, checkForWindowSize)
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, attr->, height, checkForWindowSize)
		} else if (op == SAppLayoutOperation::Generate) {
			if (attr->width.flagDefined) {
				if (attr->width.unit == SAppDimensionValue::WRAP) {
					LAYOUT_CONTROL_GENERATE(setWidthWrapping, "sl_true, slib::UIUpdateMode::Init")
				} else if (attr->width.unit == SAppDimensionValue::FILL || attr->width.unit == SAppDimensionValue::MATCH_PARENT) {
					LAYOUT_CONTROL_GENERATE(setWidthFilling, "sl_true, slib::UIUpdateMode::Init")
				} else {
					LAYOUT_CONTROL_GENERATE_ATTR(DIMENSION, attr->width, setClientWidth)
				}
			}
			if (attr->height.flagDefined) {
				if (attr->height.unit == SAppDimensionValue::WRAP) {
					LAYOUT_CONTROL_GENERATE(setHeightWrapping, "sl_true, slib::UIUpdateMode::Init")
				} else if (attr->height.unit == SAppDimensionValue::FILL || attr->height.unit == SAppDimensionValue::MATCH_PARENT) {
					LAYOUT_CONTROL_GENERATE(setHeightFilling, "sl_true, slib::UIUpdateMode::Init")
				} else {
					LAYOUT_CONTROL_GENERATE_ATTR(DIMENSION, attr->height, setClientHeight)
				}
			}
		} else if (op == SAppLayoutOperation::SimulateInit) {
			if (attr->width.flagDefined) {
				if (attr->width.unit == SAppDimensionValue::WRAP) {
					view->setWidthWrapping(sl_true, UIUpdateMode::Init);
				} else if (attr->width.unit == SAppDimensionValue::FILL || attr->width.unit == SAppDimensionValue::MATCH_PARENT) {
					view->setWidthFilling(sl_true, UIUpdateMode::Init);
				} else {
					LAYOUT_CONTROL_SIMULATE_ATTR(DIMENSION, attr->width, setClientWidth)
				}
			}
			if (attr->height.flagDefined) {
				if (attr->height.unit == SAppDimensionValue::WRAP) {
					view->setHeightWrapping(sl_true, UIUpdateMode::Init);
				} else if (attr->height.unit == SAppDimensionValue::FILL || attr->height.unit == SAppDimensionValue::MATCH_PARENT) {
					view->setHeightFilling(sl_true, UIUpdateMode::Init);
				} else {
					LAYOUT_CONTROL_SIMULATE_ATTR(DIMENSION, attr->height, setClientHeight)
				}
			}
		}
		LAYOUT_CONTROL_ATTR(GENERIC, closeOnOK, setCloseOnOK)
		LAYOUT_CONTROL_ATTR(GENERIC, closeOnCancel, setCloseOnCancel)

		params->name = "m_contentView";
		if (!(_processLayoutResourceControl_View(params))) {
			attr->resetLayout();
			return sl_false;
		}

		if (op == SAppLayoutOperation::Parse) {
			if (!(attr->backgroundColor.flagDefined)) {
				SAppDrawableValue background;
				if (attr->background.values.getCount() == 1 && attr->background.values.get_NoLock(ViewState::All, &background)) {
					if (background.flagDefined && background.flagColor) {
						attr->backgroundColor.flagDefined = sl_true;
						attr->backgroundColor.color = background.color;
						attr->backgroundColor.resourceName = background.resourceName;
						attr->background.values.setNull();
					}
				}
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Page, ViewPage)
	{
		if (!(_processLayoutResourceControl_View(params))) {
			return sl_false;
		}

		if (op == SAppLayoutOperation::Parse) {
			if (!(attr->width.flagDefined)) {
				attr->width.flagDefined = sl_true;
				attr->width.amount = 1;
				attr->width.unit = SAppDimensionValue::FILL;
			}
			if (!(attr->height.flagDefined)) {
				attr->height.flagDefined = sl_true;
				attr->height.amount = 1;
				attr->height.unit = SAppDimensionValue::FILL;
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(ViewGroup, ViewGroup)
	{
		if (!(_processLayoutResourceControl_View(params))) {
			return sl_false;
		}
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Import, SAppLayoutImportView)
	{
		Ref<SAppLayoutResource> layoutImport;
		if (op == SAppLayoutOperation::Parse) {
			attr->layout = resourceItem->getXmlAttribute("layout");
			if (attr->layout.isEmpty()) {
				LOG_ERROR_LAYOUT_CONTROL_ATTR("layout")
				return sl_false;
			}
			resourceItem->className = attr->layout;
		} else {
			layoutImport = _openLayoutResource(resource, attr->layout);
			if (layoutImport.isNull()) {
				logError(element, g_str_error_layout_not_found, attr->layout);
				return sl_false;
			}
			if (layoutImport->layoutType != SAppLayoutType::View && layoutImport->layoutType != SAppLayoutType::Page) {
				logError(element, g_str_error_layout_is_not_view, attr->layout);
				return sl_false;
			}
		}
		if (op == SAppLayoutOperation::Generate) {
		} else if (IsSimulateOp(op)) {
			if (op == SAppLayoutOperation::SimulateInit) {
				if (layoutImport->layoutType == SAppLayoutType::Page) {
					Ref<SAppLayoutImportPage> _view = new SAppLayoutImportPage;
					if (_view.isNotNull()) {
						_view->initialize(params->simulator, layoutImport.get());
					} else {
						return sl_false;
					}
					params->viewItem = _view.get();
				} else {
					Ref<SAppLayoutImportView> _view = new SAppLayoutImportView;
					if (_view.isNotNull()) {
						_view->initialize(params->simulator, layoutImport.get());
					} else {
						return sl_false;
					}
					params->viewItem = _view.get();
				}
			} else {
				if (!view) {
					return sl_false;
				}
			}
		}

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, data, setData)

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == SAppLayoutOperation::SimulateLayout) {
			if (layoutImport->layoutType == SAppLayoutType::Page) {
				((SAppLayoutImportPage*)view)->layoutViews(view->getWidth(), view->getHeight());
			} else {
				view->layoutViews(view->getWidth(), view->getHeight());
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Button, Button)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(STRING, text, setText)
		LAYOUT_CONTROL_UI_ATTR(STRING, hyperText, setHyperText)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, multiLine, setMultiLine)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, ellipsize, setEllipsize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, lines, setLineCount)
		LAYOUT_CONTROL_ATTR(GENERIC, mnemonic, setMnemonic)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, defaultButton, setDefaultButton)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconWidth, setIconWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconHeight, setIconHeight, checkScalarSize)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue iconSize;
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, , iconSize, checkScalarSize)
			if (iconSize.flagDefined) {
				if (!(attr->iconWidth.flagDefined)) {
					attr->iconWidth = iconSize;
				}
				if (!(attr->iconHeight.flagDefined)) {
					attr->iconHeight = iconSize;
				}
			}
		}
		LAYOUT_CONTROL_UI_ATTR(GENERIC, iconAlign, setIconAlignment)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, textAlign, setTextAlignment)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, textBeforeIcon, setTextBeforeIcon)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, extendTextFrame, setExtendTextFrame)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, orientation, setLayoutOrientation)

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginLeft, setIconMarginLeft, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginTop, setIconMarginTop, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginRight, setIconMarginRight, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginBottom, setIconMarginBottom, checkPosition)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue iconMargin;
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, , iconMargin, checkPosition)
			if (iconMargin.flagDefined) {
				if (!(attr->iconMarginLeft.flagDefined)) {
					attr->iconMarginLeft = iconMargin;
				}
				if (!(attr->iconMarginTop.flagDefined)) {
					attr->iconMarginTop = iconMargin;
				}
				if (!(attr->iconMarginRight.flagDefined)) {
					attr->iconMarginRight = iconMargin;
				}
				if (!(attr->iconMarginBottom.flagDefined)) {
					attr->iconMarginBottom = iconMargin;
				}
			}
		}

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, textMarginLeft, setTextMarginLeft, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, textMarginTop, setTextMarginTop, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, textMarginRight, setTextMarginRight, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, textMarginBottom, setTextMarginBottom, checkPosition)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue textMargin;
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, , textMargin, checkPosition)
			if (textMargin.flagDefined) {
				if (!(attr->textMarginLeft.flagDefined)) {
					attr->textMarginLeft = textMargin;
				}
				if (!(attr->textMarginTop.flagDefined)) {
					attr->textMarginTop = textMargin;
				}
				if (!(attr->textMarginRight.flagDefined)) {
					attr->textMarginRight = textMargin;
				}
				if (!(attr->textMarginBottom.flagDefined)) {
					attr->textMarginBottom = textMargin;
				}
			}
		}

		LAYOUT_CONTROL_UI_ATTR(GENERIC, defaultColorFilter, setUsingDefaultColorFilter)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, focusedColorFilter, setUsingFocusedColorFilter)

		LAYOUT_CONTROL_STATE_MAP(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, icon, setIcon)
		LAYOUT_CONTROL_STATE_MAP(COLOR, colorOverlay, setColorOverlay)

#define LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(TYPE, NAME, SETFUNC, ...) \
		if (op == SAppLayoutOperation::Parse) { \
			LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, *(resourceItem), #NAME, +suffix, category.NAME, ##__VA_ARGS__) \
		} else if (op == SAppLayoutOperation::Generate) { \
			LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, category.NAME, SETFUNC, CONTROL, "%d, %s", i, value) \
		} else if (IsSimulateOp(op)) { \
			LAYOUT_CONTROL_SIMULATE_STATE_MAP(TYPE, category.NAME, SETFUNC, CONTROL, (sl_uint32)i, value) \
		}

		for (sl_size i = 0; i < CountOfArray(attr->categories); i++) {
			SAppLayoutButtonCategory& category = attr->categories[i];
			String suffix = String::fromSize(i);
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(COLOR, textColor, setTextColor)
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(DRAWABLE, icon, setIcon)
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(DRAWABLE, background, setBackground)
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(BORDER, border, setBorder)
			if (op == SAppLayoutOperation::Parse) {
				SAppBorderValue::normalizeStateMap(category.border);
			}
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(COLOR, colorOverlay, setColorOverlay)
		}

		LAYOUT_CONTROL_SET_NATIVE_WIDGET_CHECK_BACKGROUND_COLOR

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Label, LabelView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_SIMULATABLE_ATTR(STRING, text, simulationText, setText)
		LAYOUT_CONTROL_UI_ATTR(STRING, hyperText, setHyperText)
		LAYOUT_CONTROL_STATE_MAP(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, multiLine, setMultiLine)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, ellipsize, setEllipsize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, lines, setLineCount)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, alignInVisibleRect, setAligningInVisibleRect)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, linksInText, setDetectingHyperlinksInPlainText);
		LAYOUT_CONTROL_UI_ATTR(COLOR, linkColor, setLinkColor)
		LAYOUT_CONTROL_UI_ATTR(COLOR, lineColor, setLineColor)
		LAYOUT_CONTROL_ATTR(GENERIC, mnemonic, setMnemonic)
		LAYOUT_CONTROL_ATTR(GENERIC, contextMenu, setUsingContextMenu)

		if (op == SAppLayoutOperation::Parse) {
			if (!(attr->text.flagDefined) && !(attr->hyperText.flagDefined)) {
				resourceItem->flagSkipParseChildren = sl_true;
				String value = String::create(params->source.substring(element->getStartContentPositionInSource(), element->getEndContentPositionInSource()));
				if (value.isNotEmpty()) {
					attr->hyperText.flagDefined = sl_true;
					attr->hyperText.flagReferResource = sl_false;
					attr->hyperText.valueOrName = value;
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Line, LineView)
	{
		LAYOUT_CONTROL_UI_ATTR(GENERIC, orientation, setOrientation)
		if (!(attr->orientation.flagDefined)) {
			if (op == SAppLayoutOperation::Parse) {
				if (resourceItem->itemTypeName == "hline") {
					attr->orientation.flagDefined = sl_true;
					attr->orientation.value = LayoutOrientation::Horizontal;
				} else if (resourceItem->itemTypeName == "vline") {
					attr->orientation.flagDefined = sl_true;
					attr->orientation.value = LayoutOrientation::Vertical;
				}
			}
		}

		LAYOUT_CONTROL_UI_ATTR(COLOR, lineColor, setLineColor)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, thickness, setLineThickness, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, lineStyle, setLineStyle)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Check, CheckBox)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(Button)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, checked, setChecked)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Radio, RadioButton)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(Check)

		LAYOUT_CONTROL_ATTR(STRING, value, setValue)

		if (op == SAppLayoutOperation::Parse) {
			attr->group = resourceItem->getXmlAttribute("group");
			if (attr->group.isNotEmpty()) {
				if (!(SAppUtil::checkName(attr->group.getData(), attr->group.getLength()))) {
					logError(element, g_str_error_resource_layout_name_invalid, attr->group);
					return sl_false;
				}
				if (!(_checkLayoutResourceItemName(resource, attr->group, element, sl_true))) {
					return sl_false;
				}
				resource->radioGroups.put(attr->group, sl_true);
			}
		} else if (op == SAppLayoutOperation::Generate) {
			if (attr->group.isNotEmpty()) {
				params->sbDefineInit->add(String::format("%s%s->add(%s);%n", strTab, attr->group, name));
			}
		} else if (op == SAppLayoutOperation::SimulateInit) {
			if (attr->group.isNotEmpty()) {
				Ref<RadioGroup> group = params->simulator->getRadioGroup(attr->group);
				if (group.isNotNull()) {
					group->add(view);
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Edit, EditView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(STRING, text, setText)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)
		LAYOUT_CONTROL_UI_ATTR(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_UI_ATTR(STRING, hintText, setHintText)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, hintGravity, setHintGravity)
		LAYOUT_CONTROL_UI_ATTR(COLOR, hintTextColor, setHintTextColor)
		LAYOUT_CONTROL_UI_ATTR(FONT, hintFont, setHintFont)
		if (op == SAppLayoutOperation::Parse) {
			if (attr->hintFont.flagDefined) {
				attr->hintFont.inheritFrom(attr->font);
			}
		}
		LAYOUT_CONTROL_UI_ATTR(GENERIC, readOnly, setReadOnly)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, password, setPassword)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, number, setNumber)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, lowercase, setLowercase)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, uppercase, setUppercase)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, multiLine, setMultiLine)
		LAYOUT_CONTROL_ATTR(GENERIC, returnKey, setReturnKeyType)
		LAYOUT_CONTROL_ATTR(GENERIC, keyboard, setKeyboardType)
		LAYOUT_CONTROL_ATTR(GENERIC, autoCap, setAutoCapitalizationType)
		LAYOUT_CONTROL_ATTR(VOID, focusNextOnReturnKey, setFocusNextOnReturnKey)
		LAYOUT_CONTROL_ATTR(GENERIC, popup, setUsingPopup)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Password, PasswordView)
	{
		if (!(_processLayoutResourceControl_Edit(params))) {
			return sl_false;
		}
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(TextArea, TextArea)
	{
		if (!(_processLayoutResourceControl_Edit(params))) {
			return sl_false;
		}
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Image, ImageView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, scale, setScaleMode)
		LAYOUT_CONTROL_ATTR(GENERIC, minAspectRatio, setMinimumAutoAspectRatio)
		LAYOUT_CONTROL_ATTR(GENERIC, maxAspectRatio, setMaximumAutoAspectRatio)

		if (op == SAppLayoutOperation::Parse) {
			if (!(attr->width.flagDefined) && !(attr->height.flagDefined)) {
				if (attr->aspectRatio.flagDefined) {
					attr->width.flagDefined = sl_true;
					attr->width.amount = 1;
					attr->width.unit = SAppDimensionValue::WRAP;
				} else {
					attr->width.flagDefined = sl_true;
					attr->width.amount = 1;
					attr->width.unit = SAppDimensionValue::WRAP;
					attr->height.flagDefined = sl_true;
					attr->height.amount = 1;
					attr->height.unit = SAppDimensionValue::WRAP;
				}
			}
		}
		if (!(attr->aspectRatio.flagDefined)) {
			if (op == SAppLayoutOperation::Generate) {
				if (attr->width.flagDefined) {
					if (!(attr->height.flagDefined)) {
						LAYOUT_CONTROL_GENERATE(setAutoAspectRatio, "sl_true")
						LAYOUT_CONTROL_GENERATE(setAspectRatioMode, "slib::AspectRatioMode::AdjustHeight, slib::UIUpdateMode::Init")
					}
				} else {
					if (attr->height.flagDefined) {
						LAYOUT_CONTROL_GENERATE(setAutoAspectRatio, "sl_true")
						LAYOUT_CONTROL_GENERATE(setAspectRatioMode, "slib::AspectRatioMode::AdjustWidth, slib::UIUpdateMode::Init")
					}
				}
			} else if (op == SAppLayoutOperation::SimulateInit) {
				if (attr->width.flagDefined) {
					if (!(attr->height.flagDefined)) {
						view->setAutoAspectRatio(sl_true);
						view->setAspectRatioMode(AspectRatioMode::AdjustHeight, slib::UIUpdateMode::Init);
					}
				} else {
					if (attr->height.flagDefined) {
						view->setAutoAspectRatio(sl_true);
						view->setAspectRatioMode(AspectRatioMode::AdjustWidth, slib::UIUpdateMode::Init);
					}
				}
			}
		}

		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, src, setSource)
		LAYOUT_CONTROL_ATTR(STRING, url, loadUrl)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

#define LAYOUT_CONTROL_PROCESS_SELECT_ITEMS \
		if (op == SAppLayoutOperation::Parse) { \
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(itemXmls, "item") \
			for (sl_size i = 0; i < itemXmls.count; i++) { \
				LAYOUT_CONTROL_DEFINE_XML(itemXml, itemXmls[i]) \
				SAppLayoutSelectItem subItem; \
				LAYOUT_CONTROL_PARSE_XML(STRING, itemXml, subItem., title) \
				if (!(subItem.title.flagDefined)) { \
					String text = itemXml.getXmlText(); \
					if (text.isNotEmpty()) { \
						if (!(subItem.title.parse(text, itemXml.element))) { \
							logError(itemXml.element, g_str_error_resource_layout_value_invalid, text); \
							return sl_false; \
						} \
					} \
				} \
				LAYOUT_CONTROL_PARSE_XML(STRING, itemXml, subItem., value) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, itemXml, subItem., selected) \
				if (!(attr->items.add_NoLock(Move(subItem)))) { \
					logError(itemXml.element, g_str_error_out_of_memory); \
					return sl_false; \
				} \
			} \
		} else if (op == SAppLayoutOperation::Generate) { \
			ListElements<SAppLayoutSelectItem> selectItems(attr->items); \
			if (selectItems.count > 0) { \
				sl_size i; \
				for (i = 0; i < selectItems.count; i++) { \
					SAppLayoutSelectItem& selectItem = selectItems[i]; \
					String strSelectedItemTitle; \
					if (!(_getStringAccessString(resource->fileNamespace, selectItem.title, strSelectedItemTitle))) { \
						return sl_false; \
					} \
					String strSelectedItemValue; \
					if (!(_getStringAccessString(resource->fileNamespace, selectItem.value, strSelectedItemValue))) { \
						return sl_false; \
					} \
					LAYOUT_CONTROL_GENERATE(addItem, "%s, %s, slib::UIUpdateMode::Init", strSelectedItemValue, strSelectedItemTitle) \
				} \
				for (i = 0; i < selectItems.count; i++) { \
					SAppLayoutSelectItem& selectItem = selectItems[i]; \
					if (selectItem.selected.flagDefined && selectItem.selected.value) { \
						LAYOUT_CONTROL_GENERATE(selectItem, "%d, slib::UIUpdateMode::Init", i) \
					} \
				} \
			} \
		} else if (op == SAppLayoutOperation::SimulateInit) { \
			ListElements<SAppLayoutSelectItem> selectItems(attr->items); \
			if (selectItems.count > 0) { \
				sl_uint32 n = (sl_uint32)(selectItems.count); \
				sl_uint32 i; \
				for (i = 0; i < n; i++) { \
					SAppLayoutSelectItem& selectItem = selectItems[i]; \
					String selectedItemTitle; \
					if (!(_getStringValue(resource->fileNamespace, selectItem.title, selectedItemTitle))) { \
						return sl_false; \
					} \
					String selectedItemValue; \
					if (!(_getStringValue(resource->fileNamespace, selectItem.value, selectedItemValue))) { \
						return sl_false; \
					} \
					view->addItem(selectedItemValue, selectedItemTitle, UIUpdateMode::Init); \
				} \
				for (i = 0; i < n; i++) { \
					SAppLayoutSelectItem& selectItem = selectItems[i]; \
					if (selectItem.selected.flagDefined && selectItem.selected.value) { \
						view->selectItem(i, UIUpdateMode::Init); \
					} \
				} \
			} \
		}

	BEGIN_PROCESS_LAYOUT_CONTROL(Select, SelectView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)

		LAYOUT_CONTROL_SET_NATIVE_WIDGET

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS
		LAYOUT_CONTROL_UI_ATTR(STRING, value, selectValue)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(SelectSwitch, SelectSwitch)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)

		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, leftIcon, setLeftIcon)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, rightIcon, setRightIcon)

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconWidth, setIconWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconHeight, setIconHeight, checkScalarSize)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue iconSize;
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, , iconSize, checkScalarSize)
			if (iconSize.flagDefined) {
				if (!(attr->iconWidth.flagDefined)) {
					attr->iconWidth = iconSize;
				}
				if (!(attr->iconHeight.flagDefined)) {
					attr->iconHeight = iconSize;
				}
			}
		}

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS
		LAYOUT_CONTROL_UI_ATTR(STRING, value, selectValue)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(ComboBox, ComboBox)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(STRING, text, setText)

		LAYOUT_CONTROL_SET_NATIVE_WIDGET

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Scroll, ScrollView)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(childXmls, sl_null)
			if (childXmls.count > 0) {
				if (childXmls.count != 1) {
					logError(element, g_str_error_resource_layout_must_contain_one_child);
					return sl_false;
				}
				Ref<SAppLayoutResourceItem> contentItem = _parseLayoutResourceItemChild(resource, resourceItem, childXmls[0], params->source);
				if (contentItem.isNull()) {
					return sl_false;
				}
				if (IsNoView(contentItem->itemType)) {
					return sl_false;
				}
				SAppLayoutViewAttributes* contentAttrs = (SAppLayoutViewAttributes*)(contentItem->attrs.get());
				if (!(attr->scrolling.flagDefined) || attr->scrolling.vertical) {
					contentAttrs->topMode = PositionMode::Free;
					contentAttrs->bottomMode = PositionMode::Free;
				}
				if (!(attr->scrolling.flagDefined) || attr->scrolling.horizontal) {
					contentAttrs->leftMode = PositionMode::Free;
					contentAttrs->rightMode = PositionMode::Free;
				}
				attr->content = contentItem;
			}
		} else if (op == SAppLayoutOperation::Generate) {
			if (attr->content.isNotNull()) {
				String addChildStatement = String::format("%s%s->setContentView(%s, slib::UIUpdateMode::Init);%n%n", strTab, name, attr->content->name);
				if (!(_generateLayoutsCpp_Item(resource, attr->content.get(), resourceItem, params, addChildStatement))) {
					return sl_false;
				}
			}
		} else if (IsSimulateOp(op)) {
			if (attr->content.isNotNull()) {
				Ref<View> contentView = CastRef<View>(_simulateLayoutCreateOrLayoutItem(params->simulator, attr->content.get(), resourceItem, view, op));
				if (contentView.isNotNull()) {
					if (op == SAppLayoutOperation::SimulateInit) {
						view->setContentView(contentView, UIUpdateMode::Init);
					}
				} else {
					return sl_false;
				}
			}
		}

		resourceItem->flagSkipParseChildren = sl_true;
		resourceItem->flagSkipGenerateChildren = sl_true;
		resourceItem->flagSkipSimulateChildren = sl_true;

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Linear, LinearLayout)
	{
		LAYOUT_CONTROL_UI_ATTR(GENERIC, orientation, setOrientation)
		if (!(attr->orientation.flagDefined)) {
			if (op == SAppLayoutOperation::Parse) {
				if (resourceItem->itemTypeName == "hlinear" || resourceItem->itemTypeName == "hlinear-iterate") {
					attr->orientation.flagDefined = sl_true;
					attr->orientation.value = LayoutOrientation::Horizontal;
				} else if (resourceItem->itemTypeName == "vlinear" || resourceItem->itemTypeName == "vlinear-iterate") {
					attr->orientation.flagDefined = sl_true;
					attr->orientation.value = LayoutOrientation::Vertical;
				}
			}
		}

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

#define LAYOUT_CONTROL_PARSE_CHILD_LAYOUT(XML_ATTR_NAME, FLAG_ITERATE) \
		attr->layout.name = resourceItem->getXmlAttribute(XML_ATTR_NAME); \
		LAYOUT_CONTROL_PARSE_ATTR(GENERIC, attr->layout., data) \
		LAYOUT_CONTROL_PARSE_ATTR(GENERIC, attr->layout., simulationCount) \
		if (attr->layout.simulationCount.flagDefined) { \
			if (attr->layout.simulationCount.value > 100) { \
				attr->layout.simulationCount.value = 100; \
			} \
		} \
		if (FLAG_ITERATE && attr->layout.name.isNotEmpty()) { \
			resourceItem->getEventMapping(XML_ATTR_NAME ":on", attr->layout.eventMappings); \
			for (auto&& mapping : attr->layout.eventMappings) { \
				if (!(mapping.value.startsWith(StringView::literal("on")))) { \
					logError(resourceItem->element, g_str_error_event_invalid_prefix, mapping.value); \
					return sl_false; \
				} \
				_registerLayoutCustomEvent(resource, mapping.value, name, mapping.key, sl_true); \
			} \
		} \
		LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(childXmls, sl_null) \
		if (childXmls.count > 0) { \
			if (attr->layout.name.isNotEmpty()) { \
				logError(element, g_str_error_resource_layout_child_layout_already_specified); \
				return sl_false; \
			} \
			if (childXmls.count != 1) { \
				logError(element, g_str_error_resource_layout_must_contain_one_child); \
				return sl_false; \
			} \
			Ref<XmlElement>& childXml = childXmls[0]; \
			if (childXml->getName() == "layout") { \
				sl_bool flagGeneratedName; \
				Ref<SAppLayoutResource> childLayout = _parseLayoutResource(resource->filePath, resource->fileNamespace, childXml, params->source, resource, &(attr->layout.name), &flagGeneratedName); \
				if (childLayout.isNull()) { \
					return sl_false; \
				} \
				if (FLAG_ITERATE && flagGeneratedName) { \
					for (auto&& item : childLayout->customEvents) { \
						_registerLayoutCustomEvent(resource, item.key, name, item.key.substring(2), sl_true); \
					} \
				} \
			} else { \
				logError(childXml, g_str_error_resource_layout_type_invalid, childXml->getName()); \
				return sl_false; \
			} \
		}

#define LAYOUT_CONTROL_DEFINE_ITERATE_BODY(SUPER_VIEW) \
		Ref<SAppLayoutResource> childLayout; \
		if (op == SAppLayoutOperation::Parse) { \
			LAYOUT_CONTROL_PARSE_CHILD_LAYOUT("item", sl_true) \
			if (attr->layout.name.isEmpty()) { \
				LOG_ERROR_LAYOUT_CONTROL_ATTR("item"); \
				return sl_false; \
			} \
			if (resourceItem->className == "slib::" #SUPER_VIEW) { \
				String _layoutName = attr->layout.name; \
				resourceItem->classNameGetter = [_layoutName](SAppDocument* doc, SAppLayoutResourceItem* item) { \
					String layoutName = _layoutName; \
					doc->getItemFromMap(doc->m_layouts, item->fileNamespace, layoutName, &layoutName, (Ref<SAppLayoutResource>*)sl_null); \
					return String::concat("slib::IterateLayout<slib::" #SUPER_VIEW ", ", layoutName, ">"); \
				}; \
			} \
			if (!(attr->layout.simulationCount.flagDefined)) { \
				attr->layout.simulationCount.value = 3; \
			} \
		} else { \
			childLayout = _openLayoutResource(resource, attr->layout.name); \
			if (childLayout.isNull()) { \
				logError(element, g_str_error_layout_not_found, attr->layout.name); \
				return sl_false; \
			} \
		} \
		if (op == SAppLayoutOperation::Parse) { \
			if (resourceItem->getXmlAttribute("item").isEmpty()) { \
				childLayout = _openLayoutResource(resource, attr->layout.name); \
				if (childLayout.isNull()) { \
					logError(element, g_str_error_layout_not_found, attr->layout.name); \
					return sl_false; \
				} \
				if (childLayout->layoutType == SAppLayoutType::View) { \
					SAppLayoutViewAttributes* childAttrs = (SAppLayoutViewAttributes*)(childLayout->attrs.get()); \
					if (!(childAttrs->left.flagDefined)) { \
						if (childAttrs->leftMode == PositionMode::Free && childAttrs->rightMode == PositionMode::Free) { \
							childAttrs->leftMode = PositionMode::ParentEdge; \
						} \
					} \
					if (!(childAttrs->top.flagDefined)) { \
						if (childAttrs->topMode == PositionMode::Free && childAttrs->bottomMode == PositionMode::Free) { \
							childAttrs->topMode = PositionMode::ParentEdge; \
						} \
					} \
				} \
			} \
		} else if (op == SAppLayoutOperation::Generate) { \
			if (attr->layout.data.isDefinedDataAccess()) { \
				params->sbDefineSetData->add(String::format("%sSLIB_UILAYOUT_ITERATE_VIEWS(%s, %s, %s, mode)%n", strTab, name, attr->layout.name, attr->layout.data.getDataAccessString())); \
			} \
		} else if (IsSimulateOp(op)) { \
			if (!(view->getProperty("setChildren").getBoolean())) { \
				for (sl_uint32 i = 0; i < attr->layout.simulationCount.value; i++) { \
					Ref<SAppLayoutImportView> child = new SAppLayoutImportView; \
					if (child.isNotNull()) { \
						child->initialize(params->simulator, childLayout.get()); \
						view->addChild(child); \
					} \
				} \
				view->setProperty("setChildren", sl_true); \
			} \
		}


	BEGIN_PROCESS_LAYOUT_CONTROL(LinearIterate, LinearLayout)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(Linear)

		LAYOUT_CONTROL_DEFINE_ITERATE_BODY(LinearLayout)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(TileLayout, TileLayout)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, columns, setColumnCount)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, rows, setRowCount)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, columnWidth, setColumnWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, rowHeight, setRowHeight, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, cellRatio, setCellRatio)
		LAYOUT_CONTROL_UI_ATTR(BORDER, grid, setGrid)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(TileIterate, TileLayout)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(TileLayout)

		LAYOUT_CONTROL_DEFINE_ITERATE_BODY(TileLayout)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	namespace
	{
		class SimuationListViewAdapter : public ViewAdapter
		{
		public:
			WeakRef<CRef> refer;
			SAppLayoutSimulator* simulator;
			Ref<SAppLayoutResource> layout;
			sl_uint64 count = 100;

		public:
			sl_uint64 getItemCount() override
			{
				return count;
			}

			Ref<View> getView(sl_uint64 index, View* original, View* parent) override
			{
				if (original) {
					return original;
				}
				Ref<CRef> _refer = refer;
				if (_refer.isNull()) {
					return sl_null;
				}
				Ref<SAppLayoutImportView> view = new SAppLayoutImportView;
				if (view.isNotNull()) {
					view->initialize(simulator, layout.get());
				}
				return view;
			}

		};
	}

	BEGIN_PROCESS_LAYOUT_CONTROL(List, ListView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		Ref<SAppLayoutResource> childLayout;
		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_PARSE_CHILD_LAYOUT("item", sl_false)
			if (!(attr->layout.simulationCount.flagDefined)) {
				attr->layout.simulationCount.value = 100;
			}
		} else {
			if (attr->layout.name.isNotEmpty()) {
				childLayout = _openLayoutResource(resource, attr->layout.name);
				if (childLayout.isNull()) {
					logError(element, g_str_error_layout_not_found, attr->layout.name);
					return sl_false;
				}
			}
		}
		if (IsSimulateOp(op)) {
			if (childLayout.isNotNull() && !(view->getProperty("setAdapter").getBoolean())) {
				if (attr->layout.simulationCount.value) {
					Ref<SimuationListViewAdapter> adapter = new SimuationListViewAdapter;
					adapter->refer = params->simulator->getRef();
					adapter->simulator = params->simulator;
					adapter->layout = childLayout;
					adapter->count = attr->layout.simulationCount.value;
					view->setAdapter(adapter);
				}
				view->setProperty("setAdapter", sl_true);
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Collection, CollectionView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		Ref<SAppLayoutResource> childLayout;
		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_PARSE_CHILD_LAYOUT("item", sl_false)
			if (!(attr->layout.simulationCount.flagDefined)) {
				attr->layout.simulationCount.value = 100;
			}
		} else {
			if (attr->layout.name.isNotEmpty()) {
				childLayout = _openLayoutResource(resource, attr->layout.name);
				if (childLayout.isNull()) {
					logError(element, g_str_error_layout_not_found, attr->layout.name);
					return sl_false;
				}
			}
		}
		if (IsSimulateOp(op)) {
			if (childLayout.isNotNull() && !(view->getProperty("setAdapter").getBoolean())) {
				if (attr->layout.simulationCount.value) {
					Ref<SimuationListViewAdapter> adapter = new SimuationListViewAdapter;
					adapter->refer = params->simulator->getRef();
					adapter->simulator = params->simulator;
					adapter->layout = childLayout;
					adapter->count = attr->layout.simulationCount.value;
					view->setAdapter(adapter);
				}
				view->setProperty("setAdapter", sl_true);
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Table, TableLayout)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(BORDER, grid, setGrid)
		LAYOUT_CONTROL_UI_ATTR(BORDER, horizontalGrid, setHorizontalGrid)
		LAYOUT_CONTROL_UI_ATTR(BORDER, verticalGrid, setVerticalGrid)

		if (op == SAppLayoutOperation::Parse) {
			{
				LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(columnXmls, "column")
				for (sl_size i = 0; i < columnXmls.count; i++) {
					LAYOUT_CONTROL_DEFINE_XML(columnXml, columnXmls[i])
					SAppLayoutTableColumn column;
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml.element, column., name)
					if (column.name.flagDefined) {
						if (!(_checkLayoutResourceItemName(resource, column.name.value, columnXml.element))) {
							return sl_false;
						}
						resource->otherNames.put(column.name.value, sl_true);
					}
					LAYOUT_CONTROL_PARSE_XML(STRING, columnXml, column., id)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., width, checkSize)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., minWidth, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., maxWidth, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., margin, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., marginLeft, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., marginRight, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., padding, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., paddingLeft, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., paddingRight, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DRAWABLE, columnXml, column., background)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., align)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., visible)
					if (!(attr->columns.add_NoLock(Move(column)))) {
						logError(columnXml.element, g_str_error_out_of_memory);
						return sl_false;
					}
				}
			}
			{
				CHashMap< Pair<sl_uint32, sl_uint32>, sl_bool > cellAllocs;
				LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(rowXmls, "row")
				sl_uint32 nRows = (sl_uint32)(rowXmls.count);
				for (sl_uint32 i = 0; i < nRows; i++) {
					LAYOUT_CONTROL_DEFINE_XML(rowXml, rowXmls[i])
					SAppLayoutTableRow row;
					LAYOUT_CONTROL_PARSE_XML(GENERIC, rowXml.element, row., name)
					if (row.name.flagDefined) {
						if (!(_checkLayoutResourceItemName(resource, row.name.value, rowXml.element))) {
							return sl_false;
						}
						resource->otherNames.put(row.name.value, sl_true);
					}
					LAYOUT_CONTROL_PARSE_XML(STRING, rowXml, row., id)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., height, checkSize)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., minHeight, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., maxHeight, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., margin, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., marginTop, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., marginBottom, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., padding, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., paddingTop, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., paddingBottom, checkPosition)
					LAYOUT_CONTROL_PARSE_XML(DRAWABLE, rowXml, row., background)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, rowXml, row., align)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, rowXml, row., visible)
					sl_uint32 iCell = 0;
					LAYOUT_CONTROL_DEFINE_XML_CHILDREN(childXmls, rowXml, sl_null)
					for (sl_size k = 0; k < childXmls.count; k++) {
						LAYOUT_CONTROL_DEFINE_XML(xmlView, childXmls[k])
						SAppLayoutTableCell cell;
						if (xmlView.getTagName() != "cell") {
							Ref<SAppLayoutResourceItem> subItemView = _parseLayoutResourceItemChild(resource, resourceItem, xmlView.element, params->source);
							if (subItemView.isNull()) {
								return sl_false;
							}
							cell.view = subItemView;
						}
						LAYOUT_CONTROL_PARSE_XML(GENERIC, xmlView, cell., rowspan)
						LAYOUT_CONTROL_PARSE_XML(GENERIC, xmlView, cell., colspan)
						if (!(cell.rowspan.flagDefined) || cell.rowspan.value < 1) {
							cell.rowspan.value = 1;
						}
						if (!(cell.colspan.flagDefined) || cell.colspan.value < 1) {
							cell.colspan.value = 1;
						}
						while (cellAllocs.find_NoLock(Pair<sl_uint32, sl_uint32>(i, iCell))) {
							iCell++;
						}
						if (iCell + cell.colspan.value > attr->columns.getCount()) {
							if (!(attr->columns.setCount_NoLock(iCell+cell.colspan.value))) {
								logError(xmlView.element, g_str_error_out_of_memory);
								return sl_false;
							}
						}
						for (sl_uint32 t1 = 0; t1 < cell.rowspan.value; t1++) {
							for (sl_uint32 t2 = 0; t2 < cell.colspan.value; t2++) {
								cellAllocs.put_NoLock(Pair<sl_uint32, sl_uint32>(i + t1, iCell + t2), sl_true);
							}
						}
						if (!(row.cells.setCount_NoLock(iCell+1))) {
							logError(xmlView.element, g_str_error_out_of_memory);
							return sl_false;
						}
						row.cells.setAt_NoLock(iCell, Move(cell));
					}

					if (!(attr->rows.add_NoLock(Move(row)))) {
						logError(rowXml.element, g_str_error_out_of_memory);
						return sl_false;
					}
				}
			}
		} else if (op == SAppLayoutOperation::Generate) {
			ListElements<SAppLayoutTableColumn> cols(attr->columns);
			ListElements<SAppLayoutTableRow> rows(attr->rows);
			sl_uint32 iRow, iCol;
			sl_uint32 nCols = (sl_uint32)(cols.count);
			sl_uint32 nRows = (sl_uint32)(rows.count);
			LAYOUT_CONTROL_GENERATE(setColumnCount, "%d, slib::UIUpdateMode::Init", nCols)
			LAYOUT_CONTROL_GENERATE(setRowCount, "%d, slib::UIUpdateMode::Init", nRows)
			for (iCol = 0; iCol < nCols; iCol++) {
				SAppLayoutTableColumn& col = cols[iCol];
				if (col.name.flagDefined) {
					params->sbDeclare->add(String::format("\t\t\tslib::Ref<slib::TableLayout::Column> %s;%n", col.name.value));
					params->sbDefineInit->add(String::format("\t\t\t%s = %s->getColumn(%d);%n", col.name.value, resourceItem->name, iCol));
				}
				LAYOUT_CONTROL_GENERATE_STRING(col.id, setColumnId, BASIC, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_SIZE(col.width, setColumnWidth, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(col.minWidth, setColumnMinimumWidth, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(col.maxWidth, setColumnMaximumWidth, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(col.margin, setColumnMargin, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(col.marginLeft, setColumnMarginLeft, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(col.marginRight, setColumnMarginRight, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(col.padding, setColumnPadding, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(col.paddingLeft, setColumnPaddingLeft, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(col.paddingRight, setColumnPaddingRight, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_DRAWABLE(col.background, setColumnBackground, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_GENERIC(col.align, setColumnAlignment, ITEM, "%d, %s", iCol, value)
				LAYOUT_CONTROL_GENERATE_GENERIC(col.visible, setColumnVisible, ITEM, "%d, %s", iCol, value)
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				SAppLayoutTableRow& row = rows[iRow];
				if (row.name.flagDefined) {
					params->sbDeclare->add(String::format("\t\t\tslib::Ref<slib::TableLayout::Row> %s;%n", row.name.value));
					params->sbDefineInit->add(String::format("\t\t\t%s = %s->getRow(%d);%n", row.name.value, resourceItem->name, iRow));
				}
				LAYOUT_CONTROL_GENERATE_STRING(row.id, setRowId, BASIC, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_SIZE(row.height, setRowHeight, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(row.minHeight, setRowMinimumHeight, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(row.maxHeight, setRowMaximumHeight, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(row.margin, setRowMargin, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(row.marginTop, setRowMarginTop, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(row.marginBottom, setRowMarginBottom, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(row.padding, setRowPadding, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(row.paddingTop, setRowPaddingTop, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(row.paddingBottom, setRowPaddingBottom, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DIMENSION(row.paddingBottom, setRowPaddingBottom, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_DRAWABLE(row.background, setRowBackground, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_GENERIC(row.align, setRowAlignment, ITEM, "%d, %s", iRow, value)
				LAYOUT_CONTROL_GENERATE_GENERIC(row.visible, setRowVisible, ITEM, "%d, %s", iRow, value)
			}
		} else if (IsSimulateOp(op)) {
			ListElements<SAppLayoutTableColumn> cols(attr->columns);
			ListElements<SAppLayoutTableRow> rows(attr->rows);
			sl_uint32 iRow, iCol;
			sl_uint32 nCols = (sl_uint32)(cols.count);
			sl_uint32 nRows = (sl_uint32)(rows.count);
			if (op == SAppLayoutOperation::SimulateInit) {
				view->setColumnCount(nCols, UIUpdateMode::Init);
				view->setRowCount(nRows, UIUpdateMode::Init);
			}
			for (iCol = 0; iCol < nCols; iCol++) {
				SAppLayoutTableColumn& col = cols[iCol];
				LAYOUT_CONTROL_SIMULATE_STRING(col.id, setColumnId, BASIC, iCol, value)
				LAYOUT_CONTROL_SIMULATE_SIZE(col.width, setColumnWidth, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(col.minWidth, setColumnMinimumWidth, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(col.maxWidth, setColumnMaximumWidth, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(col.margin, setColumnMargin, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(col.marginLeft, setColumnMarginLeft, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(col.marginRight, setColumnMarginRight, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(col.padding, setColumnPadding, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(col.paddingLeft, setColumnPaddingLeft, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(col.paddingRight, setColumnPaddingRight, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_DRAWABLE(col.background, setColumnBackground, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_GENERIC(col.align, setColumnAlignment, ITEM, iCol, value)
				LAYOUT_CONTROL_SIMULATE_GENERIC(col.visible, setColumnVisible, ITEM, iCol, value)
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				SAppLayoutTableRow& row = rows[iRow];
				LAYOUT_CONTROL_SIMULATE_STRING(row.id, setRowId, BASIC, iRow, value)
				LAYOUT_CONTROL_SIMULATE_SIZE(row.height, setRowHeight, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(row.minHeight, setRowMinimumHeight, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(row.maxHeight, setRowMaximumHeight, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(row.margin, setRowMargin, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(row.marginTop, setRowMarginTop, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(row.marginBottom, setRowMarginBottom, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(row.padding, setRowPadding, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(row.paddingTop, setRowPaddingTop, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(row.paddingBottom, setRowPaddingBottom, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DIMENSION(row.paddingBottom, setRowPaddingBottom, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_DRAWABLE(row.background, setRowBackground, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_GENERIC(row.align, setRowAlignment, ITEM, iRow, value)
				LAYOUT_CONTROL_SIMULATE_GENERIC(row.visible, setRowVisible, ITEM, iRow, value)
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				SAppLayoutTableRow& row = rows[iRow];
				ListElements<SAppLayoutTableCell> cells(row.cells);
				sl_uint32 nCells = Math::min((sl_uint32)(cells.count), nCols);
				for (iCol = 0; iCol < nCells; iCol++) {
					SAppLayoutTableCell& cell = cells[iCol];
					if (cell.view.isNotNull()) {
						Ref<View> contentView = CastRef<View>(_simulateLayoutCreateOrLayoutItem(params->simulator, cell.view.get(), resourceItem, view, op));
						if (contentView.isNotNull()) {
							if (op == SAppLayoutOperation::SimulateInit) {
								view->setCell(iRow, iCol, contentView, cell.rowspan.value, cell.colspan.value, UIUpdateMode::Init);
							}
						} else {
							return sl_false;
						}
					}
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == SAppLayoutOperation::Generate) {
			ListElements<SAppLayoutTableColumn> cols(attr->columns);
			ListElements<SAppLayoutTableRow> rows(attr->rows);
			sl_uint32 iRow, iCol;
			sl_uint32 nCols = (sl_uint32)(cols.count);
			sl_uint32 nRows = (sl_uint32)(rows.count);
			for (iRow = 0; iRow < nRows; iRow++) {
				SAppLayoutTableRow& row = rows[iRow];
				ListElements<SAppLayoutTableCell> cells(row.cells);
				sl_uint32 nCells = Math::min((sl_uint32)(cells.count), nCols);
				for (iCol = 0; iCol < nCells; iCol++) {
					SAppLayoutTableCell& cell = cells[iCol];
					if (cell.view.isNotNull()) {
						String addChildStatement;
						if (cell.colspan.value <= 1 && cell.rowspan.value <= 1) {
							addChildStatement = String::format("%s%s->setCell(%d, %d, %s, slib::UIUpdateMode::Init);%n%n", strTab, name, iRow, iCol, cell.view->name);
						} else {
							addChildStatement = String::format("%s%s->setCell(%d, %d, %s, %d, %d, slib::UIUpdateMode::Init);%n%n", strTab, name, iRow, iCol, cell.view->name, cell.rowspan.value, cell.colspan.value);
						}
						if (!(_generateLayoutsCpp_Item(resource, cell.view.get(), resourceItem, params, addChildStatement))) {
							return sl_false;
						}
					}
				}
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(ListControl, ListControl)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(columnXmls, "column")
			for (sl_size i = 0; i < columnXmls.count; i++) {
				LAYOUT_CONTROL_DEFINE_XML(columnXml, columnXmls[i])
				SAppLayoutListControlColumn column;
				LAYOUT_CONTROL_PARSE_XML(COLOR, columnXml, column., title)
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., width, checkScalarSize)
				LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., align)
				LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., headerAlign)
				if (!(attr->columns.add_NoLock(Move(column)))) {
					logError(columnXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == SAppLayoutOperation::Generate) {
			ListElements<SAppLayoutListControlColumn> columns(attr->columns);
			if (columns.count > 0) {
				LAYOUT_CONTROL_GENERATE(setColumnCount, "%d, slib::UIUpdateMode::Init", columns.count)
				for (sl_size i = 0; i < columns.count; i++) {
					SAppLayoutListControlColumn& column = columns[i];
					LAYOUT_CONTROL_GENERATE_STRING(column.title, setHeaderText, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_DIMENSION(column.width, setColumnWidth, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(column.align, setColumnAlignment, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(column.headerAlign, setHeaderAlignment, ITEM, "%d, %s", i, value)
				}
			}
		} else if (IsSimulateOp(op)) {
			ListElements<SAppLayoutListControlColumn> columns(attr->columns);
			if (columns.count > 0) {
				sl_uint32 n = (sl_uint32)(columns.count);
				view->setColumnCount(n, UIUpdateMode::Init);
				for (sl_uint32 i = 0; i < n; i++) {
					SAppLayoutListControlColumn& column = columns[i];
					LAYOUT_CONTROL_SIMULATE_STRING(column.title, setHeaderText, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_DIMENSION(column.width, setColumnWidth, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(column.align, setColumnAlignment, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(column.headerAlign, setHeaderAlignment, ITEM, i, value)
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Render, RenderView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ATTR(GENERIC, redraw, setRedrawMode)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Tab, TabView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, orientation, setOrientation)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabWidth, setTabWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabHeight, setTabHeight, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, barBackground, setBarBackground)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, contentBackground, setContentBackground)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, tabBackground, setTabBackground)
		LAYOUT_CONTROL_STATE_MAP(COLOR, labelColor, setLabelColor)
		LAYOUT_CONTROL_UI_ATTR(FONT, labelFont, setLabelFont)
		if (op == SAppLayoutOperation::Parse) {
			if (attr->labelFont.flagDefined) {
				attr->labelFont.inheritFrom(attr->font);
			}
		}
		LAYOUT_CONTROL_UI_ATTR(GENERIC, tabAlign, setTabAlignment)

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabPaddingLeft, setTabPaddingLeft, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabPaddingTop, setTabPaddingTop, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabPaddingRight, setTabPaddingRight, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabPaddingBottom, setTabPaddingBottom, checkPosition)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue tabPadding;
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, , tabPadding, checkPosition)
			if (tabPadding.flagDefined) {
				if (!(attr->tabPaddingLeft.flagDefined)) {
					attr->tabPaddingLeft = tabPadding;
				}
				if (!(attr->tabPaddingTop.flagDefined)) {
					attr->tabPaddingTop = tabPadding;
				}
				if (!(attr->tabPaddingRight.flagDefined)) {
					attr->tabPaddingRight = tabPadding;
				}
				if (!(attr->tabPaddingBottom.flagDefined)) {
					attr->tabPaddingBottom = tabPadding;
				}
			}
		}

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabSpaceSize, setTabSpaceSize, checkPosition)

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconWidth, setIconWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconHeight, setIconHeight, checkScalarSize)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue iconSize;
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, , iconSize, checkScalarSize)
			if (iconSize.flagDefined) {
				if (!(attr->iconWidth.flagDefined)) {
					attr->iconWidth = iconSize;
				}
				if (!(attr->iconHeight.flagDefined)) {
					attr->iconHeight = iconSize;
				}
			}
		}

		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(itemXmls, "item")
			for (sl_size i = 0; i < itemXmls.count; i++) {
				LAYOUT_CONTROL_DEFINE_XML(itemXml, itemXmls[i])
				SAppLayoutTabItem subItem;
				LAYOUT_CONTROL_PARSE_XML(STRING, itemXml, subItem., label)
				LAYOUT_CONTROL_PARSE_XML(DRAWABLE, itemXml, subItem., icon)
				LAYOUT_CONTROL_PARSE_XML(GENERIC, itemXml, subItem., selected)
				LAYOUT_CONTROL_DEFINE_XML_CHILDREN(childXmls, itemXml, sl_null)
				if (childXmls.count > 0) {
					if (childXmls.count != 1) {
						logError(itemXml.element, g_str_error_resource_layout_must_contain_one_child);
						return sl_false;
					}
					Ref<SAppLayoutResourceItem> subItemView = _parseLayoutResourceItemChild(resource, resourceItem, childXmls[0], params->source);
					if (subItemView.isNull()) {
						return sl_false;
					}
					if (IsNoView(subItemView->itemType)) {
						return sl_false;
					}
					((SAppLayoutViewAttributes*)(subItemView->attrs.get()))->resetLayout();
					subItem.view = subItemView;
				}
				if (!(attr->items.add_NoLock(Move(subItem)))) {
					logError(itemXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == SAppLayoutOperation::Generate) {
			ListElements<SAppLayoutTabItem> subItems(attr->items);
			if (subItems.count > 0) {
				LAYOUT_CONTROL_GENERATE(setTabCount, "%d, slib::UIUpdateMode::Init", subItems.count)
				sl_size indexSelected = 0;
				sl_bool flagSelected = sl_false;
				for (sl_size i = 0; i < subItems.count; i++) {
					SAppLayoutTabItem& subItem = subItems[i];
					LAYOUT_CONTROL_GENERATE_STRING(subItem.label, setTabLabel, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_DRAWABLE(subItem.icon, setTabIcon, ITEM, "%d, %s", i, value)
					if (subItem.selected.flagDefined && subItem.selected.value) {
						flagSelected = sl_true;
						indexSelected = i;
					}
				}
				if (flagSelected) {
					LAYOUT_CONTROL_GENERATE(selectTab, "%d, slib::UIUpdateMode::Init", indexSelected)
				}
			}
		} else if (IsSimulateOp(op)) {
			ListElements<SAppLayoutTabItem> subItems(attr->items);
			if (subItems.count > 0) {
				sl_uint32 nSubItems = (sl_uint32)(subItems.count);
				if (op == SAppLayoutOperation::SimulateInit) {
					view->setTabCount(nSubItems, UIUpdateMode::Init);
				}
				sl_uint32 indexSelected = 0;
				sl_bool flagSelected = sl_false;
				for (sl_uint32 i = 0; i < nSubItems; i++) {
					SAppLayoutTabItem& subItem = subItems[i];
					LAYOUT_CONTROL_SIMULATE_STRING(subItem.label, setTabLabel, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_DRAWABLE(subItem.icon, setTabIcon, ITEM, i, value)
					if (subItem.selected.flagDefined && subItem.selected.value) {
						flagSelected = sl_true;
						indexSelected = i;
					}
					if (subItem.view.isNotNull()) {
						Ref<View> contentView = CastRef<View>(_simulateLayoutCreateOrLayoutItem(params->simulator, subItem.view.get(), resourceItem, view, op));
						if (contentView.isNotNull()) {
							if (op == SAppLayoutOperation::SimulateInit) {
								view->setTabContentView(i, contentView, UIUpdateMode::Init);
							}
						} else {
							return sl_false;
						}
					}
				}

				if (flagSelected) {
					if (op == SAppLayoutOperation::SimulateInit) {
						view->selectTab(indexSelected, UIUpdateMode::Init);
					}
				}
			}

		}

		LAYOUT_CONTROL_SET_NATIVE_WIDGET

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == SAppLayoutOperation::Generate) {
			ListElements<SAppLayoutTabItem> subItems(attr->items);
			for (sl_size i = 0; i < subItems.count; i++) {
				SAppLayoutTabItem& subItem = subItems[i];
				if (subItem.view.isNotNull()) {
					String addChildStatement = String::format("%s%s->setTabContentView(%d, %s, slib::UIUpdateMode::Init);%n%n", strTab, name, i, subItem.view->name);
					if (!(_generateLayoutsCpp_Item(resource, subItem.view.get(), resourceItem, params, addChildStatement))) {
						return sl_false;
					}
				}
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Tree, TreeView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, itemIcon, setItemIcon)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, openedIcon, setOpenedItemIcon)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, closedIcon, setClosedItemIcon)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, collapsedIcon, setCollapsedIcon)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, expandedIcon, setExpandedIcon)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, itemBackground, setItemBackground)
		LAYOUT_CONTROL_STATE_MAP(COLOR, textColor, setItemTextColor)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemIconSize, setItemIconSize, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemIconWidth, setItemIconWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemIconHeight, setItemIconHeight, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemHeight, setItemHeight, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemPadding, setItemPadding, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemIndent, setItemIndent, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, textIndent, setTextIndent, checkPosition)
		LAYOUT_CONTROL_ATTR(GENERIC, itemCursor, setItemCursor)

		LAYOUT_CONTROL_ADD_STATEMENT

		ListElements< Ref<SAppLayoutResourceItem> > children(resourceItem->children);
		for (sl_size i = 0; i < children.count; i++) {
			Ref<SAppLayoutResourceItem>& child = children[i];
			if (op == SAppLayoutOperation::Generate) {
				String addStatement = String::format("\t\t\t%s->%s(%s, slib::UIUpdateMode::Init);%n%n", params->name, child->itemType == SAppLayoutItemType::TreeItem ? "addItem" : "addChild", child->name);
				if (!(_generateLayoutsCpp_Item(resource, child.get(), resourceItem, params, addStatement))) {
					return sl_false;
				}
			} else  if (IsSimulateOp(op)) {
				Ref<CRef> childViewItem = _simulateLayoutCreateOrLayoutItem(params->simulator, child.get(), resourceItem, view, op);
				if (childViewItem.isNull()) {
					return sl_false;
				}
				if (op == SAppLayoutOperation::SimulateInit) {
					if (child->itemType == SAppLayoutItemType::TreeItem) {
						TreeViewItem* treeItem = CastInstance<TreeViewItem>(childViewItem.get());
						if (!treeItem) {
							return sl_false;
						}
						view->addItem(treeItem, UIUpdateMode::Init);
					} else {
						View* childView = CastInstance<View>(childViewItem.get());
						if (!childView) {
							return sl_false;
						}
						view->addChild(ToRef(&childView), UIUpdateMode::Init);
					}
				}
			}
		}

		resourceItem->flagSkipGenerateChildren = sl_true;
		resourceItem->flagSkipSimulateChildren = sl_true;

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(TreeItem, TreeViewItem)
	{
		LAYOUT_CONTROL_ATTR(STRING, id, setId)
		LAYOUT_CONTROL_UI_ATTR(STRING, text, setText)
		LAYOUT_CONTROL_UI_ATTR(FONT, font, setFont)
		if (op == SAppLayoutOperation::Parse) {
			if (params->parentResourceItem) {
				SAppLayoutItemType parentType = params->parentResourceItem->itemType;
				CRef* parentAttr = params->parentResourceItem->attrs.get();
				if (parentType == SAppLayoutItemType::TreeItem) {
					attr->font.inheritFrom(((SAppLayoutTreeItemAttributes*)parentAttr)->font);
				} else if (parentType == SAppLayoutItemType::Tree) {
					attr->font.inheritFrom(((SAppLayoutTreeAttributes*)parentAttr)->font);
				}
			}
		}
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, background, setBackground)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, icon, setIcon)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, openedIcon, setOpenedIcon)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, closedIcon, setClosedIcon)
		LAYOUT_CONTROL_STATE_MAP(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconSize, setIconSize, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconWidth, setIconWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconHeight, setIconHeight, checkScalarSize)
		LAYOUT_CONTROL_ATTR(STRING, toolTip, setToolTip)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, height, setHeight, checkScalarSize)
		LAYOUT_CONTROL_ATTR(GENERIC, cursor, setCursor)
		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_PARSE_ATTR(GENERIC, attr->, opened)
			LAYOUT_CONTROL_PARSE_ATTR(GENERIC, attr->, selected)
		} else if (op == SAppLayoutOperation::Generate) {
			if (attr->opened.value) {
				LAYOUT_CONTROL_GENERATE_DELAYED(open, "slib::UIUpdateMode::Init")
			}
			if (attr->selected.value) {
				LAYOUT_CONTROL_GENERATE_DELAYED(select, "slib::UIUpdateMode::Init")
			}
		} else if (op == SAppLayoutOperation::SimulateLayout) {
			if (attr->opened.value || attr->selected.value) {
				if (!(view->getProperty("init").getBoolean())) {
					view->setProperty("init", sl_true);
					if (attr->opened.value) {
						view->open();
					}
					if (attr->selected.value) {
						view->select();
					}
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

		ListElements< Ref<SAppLayoutResourceItem> > children(resourceItem->children);
		for (sl_size i = 0; i < children.count; i++) {
			Ref<SAppLayoutResourceItem>& child = children[i];
			if (op == SAppLayoutOperation::Generate) {
				String addStatement = String::format("\t\t\t%s->addChild(%s, slib::UIUpdateMode::Init);%n%n", params->name, child->name);
				if (!(_generateLayoutsCpp_Item(resource, child.get(), resourceItem, params, addStatement))) {
					return sl_false;
				}
			} else  if (IsSimulateOp(op)) {
				Ref<CRef> childViewItem = _simulateLayoutCreateOrLayoutItem(params->simulator, child.get(), resourceItem, view, op);
				if (childViewItem.isNull()) {
					return sl_false;
				}
				if (op == SAppLayoutOperation::SimulateInit) {
					TreeViewItem* treeItem = CastInstance<TreeViewItem>(childViewItem.get());
					if (!treeItem) {
						return sl_false;
					}
					view->addChild(treeItem, UIUpdateMode::Init);
				}
			}
		}

		resourceItem->flagSkipGenerateChildren = sl_true;
		resourceItem->flagSkipSimulateChildren = sl_true;

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Split, SplitLayout)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, orientation, setOrientation)
		if (!(attr->orientation.flagDefined)) {
			if (op == SAppLayoutOperation::Parse) {
				if (resourceItem->itemTypeName == "hsplit") {
					attr->orientation.flagDefined = sl_true;
					attr->orientation.value = LayoutOrientation::Horizontal;
				} else if (resourceItem->itemTypeName == "vsplit") {
					attr->orientation.flagDefined = sl_true;
					attr->orientation.value = LayoutOrientation::Vertical;
				}
			}
		}

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, dividerWidth, setDividerWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, dividerBackground, setDividerBackground)
		LAYOUT_CONTROL_UI_ATTR(COLOR, dividerColor, setDividerColor)
		LAYOUT_CONTROL_ATTR(DIMENSION, cursorMargin, setCursorMargin, checkScalarSize)

		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(itemXmls, "item")
			for (sl_size i = 0; i < itemXmls.count; i++) {
				LAYOUT_CONTROL_DEFINE_XML(itemXml, itemXmls[i])
				SAppLayoutSplitItem subItem;
				LAYOUT_CONTROL_PARSE_XML(GENERIC, itemXml, subItem., weight)
				LAYOUT_CONTROL_PARSE_XML(GENERIC, itemXml, subItem., minWeight)
				LAYOUT_CONTROL_PARSE_XML(GENERIC, itemXml, subItem., maxWeight)
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, itemXml, subItem., minSize, checkScalarSize)
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, itemXml, subItem., maxSize, checkScalarSize)
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, itemXml, subItem., dividerWidth, checkScalarSize)
				LAYOUT_CONTROL_PARSE_XML(DRAWABLE, itemXml, subItem., dividerBackground)
				LAYOUT_CONTROL_PARSE_XML(COLOR, itemXml, subItem., dividerColor)
				LAYOUT_CONTROL_DEFINE_XML_CHILDREN(childXmls, itemXml, sl_null)
				if (childXmls.count > 0) {
					if (childXmls.count != 1) {
						logError(itemXml.element, g_str_error_resource_layout_must_contain_one_child);
						return sl_false;
					}
					Ref<SAppLayoutResourceItem> subItemView = _parseLayoutResourceItemChild(resource, resourceItem, childXmls[0], params->source);
					if (subItemView.isNull()) {
						return sl_false;
					}
					if (IsNoView(subItemView->itemType)) {
						return sl_false;
					}
					((SAppLayoutViewAttributes*)(subItemView->attrs.get()))->resetLayout();
					subItem.view = subItemView;
				}
				if (!(attr->items.add_NoLock(Move(subItem)))) {
					logError(itemXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == SAppLayoutOperation::Generate) {
			ListElements<SAppLayoutSplitItem> subItems(attr->items);
			if (subItems.count > 0) {
				if (subItems.count > 2) {
					LAYOUT_CONTROL_GENERATE(setItemCount, "%d, slib::UIUpdateMode::Init", subItems.count)
				}
				for (sl_size i = 0; i < subItems.count; i++) {
					SAppLayoutSplitItem& subItem = subItems[i];
					LAYOUT_CONTROL_GENERATE_GENERIC(subItem.weight, setItemWeight, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(subItem.minWeight, setItemMinimumWeight, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(subItem.maxWeight, setItemMaximumWeight, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_DIMENSION(subItem.minSize, setItemMinimumSize, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_DIMENSION(subItem.maxSize, setItemMaximumSize, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_DIMENSION(subItem.dividerWidth, setItemDividerWidth, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_DRAWABLE(subItem.dividerBackground, setItemDividerBackground, ITEM, "%d, %s", i, value)
					LAYOUT_CONTROL_GENERATE_COLOR(subItem.dividerColor, setItemDividerColor, ITEM, "%d, %s", i, value)
				}
			}
		} else if (IsSimulateOp(op)) {
			ListElements<SAppLayoutSplitItem> subItems(attr->items);
			if (subItems.count > 0) {
				if (op == SAppLayoutOperation::SimulateInit) {
					if (subItems.count > 2) {
						view->setItemCount(subItems.count, UIUpdateMode::Init);
					}
				}
				for (sl_size i = 0; i < subItems.count; i++) {
					SAppLayoutSplitItem& subItem = subItems[i];
					LAYOUT_CONTROL_SIMULATE_GENERIC(subItem.weight, setItemWeight, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(subItem.minWeight, setItemMinimumWeight, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(subItem.maxWeight, setItemMaximumWeight, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_DIMENSION(subItem.minSize, setItemMinimumSize, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_DIMENSION(subItem.maxSize, setItemMaximumSize, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_DIMENSION(subItem.dividerWidth, setItemDividerWidth, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_DRAWABLE(subItem.dividerBackground, setItemDividerBackground, ITEM, i, value)
					LAYOUT_CONTROL_SIMULATE_COLOR(subItem.dividerColor, setItemDividerColor, ITEM, i, value)
					if (subItem.view.isNotNull()) {
						Ref<View> contentView = CastRef<View>(_simulateLayoutCreateOrLayoutItem(params->simulator, subItem.view.get(), resourceItem, view, op));
						if (contentView.isNotNull()) {
							if (op == SAppLayoutOperation::SimulateInit) {
								view->setItemView(i, contentView);
							}
						} else {
							return sl_false;
						}
					}
				}
				view->relayout();
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == SAppLayoutOperation::Generate) {
			ListElements<SAppLayoutSplitItem> subItems(attr->items);
			for (sl_size i = 0; i < subItems.count; i++) {
				SAppLayoutSplitItem& subItem = subItems[i];
				if (subItem.view.isNotNull()) {
					String addChildStatement = String::format("%s%s->setItemView(%d, %s, slib::UIUpdateMode::Init);%n%n", strTab, name, i, subItem.view->name);
					if (!(_generateLayoutsCpp_Item(resource, subItem.view.get(), resourceItem, params, addChildStatement))) {
						return sl_false;
					}
				}
			}
			LAYOUT_CONTROL_GENERATE(relayout, "slib::UIUpdateMode::None")
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Web, WebView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_PARSE_ATTR(STRING, attr->, url)
			LAYOUT_CONTROL_PARSE_ATTR(STRING, attr->, html)
		} else {
			if (op == SAppLayoutOperation::Generate) {
				String strUrl;
				if (!(_getStringAccessString(resource->fileNamespace, attr->url, strUrl))) {
					return sl_false;
				}
				String strHtml;
				if (!(_getStringAccessString(resource->fileNamespace, attr->html, strHtml))) {
					return sl_false;
				}
				if (attr->html.flagDefined) {
					if (attr->url.flagDefined) {
						LAYOUT_CONTROL_GENERATE(loadHTML, "%s, %s", strHtml, strUrl)
					} else {
						LAYOUT_CONTROL_GENERATE(loadHTML, "%s, sl_null", strHtml)
					}
				} else {
					if (attr->url.flagDefined) {
						LAYOUT_CONTROL_GENERATE(loadURL, "%s", strUrl)
					}
				}
			} else if (op == SAppLayoutOperation::SimulateInit) {
				String _url;
				if (!(_getStringValue(resource->fileNamespace, attr->url, _url))) {
					return sl_false;
				}
				String _html;
				if (!(_getStringValue(resource->fileNamespace, attr->html, _html))) {
					return sl_false;
				}
				if (attr->html.flagDefined) {
					if (attr->url.flagDefined) {
						view->loadHTML(_html, _url);
					} else {
						view->loadHTML(_html, sl_null);
					}
				} else {
					if (attr->url.flagDefined) {
						view->loadURL(_url);
					}
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Progress, ProgressBar)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, orientation, setOrientation)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, min, setMinimumValue)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, max, setMaximumValue)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, dual, setDualValues)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, value, setValue)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, value2, setSecondaryValue)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, discrete, setDiscrete)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, step, setStep)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, reversed, setReversed)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, track, setTrack)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, progress, setProgress)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, progress2, setSecondaryProgress)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Slider, Slider)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(Progress)

		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, thumb, setThumb)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, thumbWidth, setThumbWidth, checkScalarSize);
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, thumbHeight, setThumbHeight, checkScalarSize);
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue thumbSize;
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, , thumbSize, checkScalarSize)
			if (thumbSize.flagDefined) {
				if (!(attr->thumbWidth.flagDefined)) {
					attr->thumbWidth = thumbSize;
				}
				if (!(attr->thumbHeight.flagDefined)) {
					attr->thumbHeight = thumbSize;
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Switch, SwitchView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, value, setValue)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, textInButton, setTextInButton)

#define LAYOUT_CONTROL_SWITCH_ATTR(TYPE, NAME, SETFUNC, ...) \
		if (op == SAppLayoutOperation::Parse) { \
			LAYOUT_CONTROL_PARSE_##TYPE(*(resourceItem), #NAME "Off", , attr->NAME##s[0], ##__VA_ARGS__) \
			LAYOUT_CONTROL_PARSE_##TYPE(*(resourceItem), #NAME "On", , attr->NAME##s[1], ##__VA_ARGS__) \
		} else if (op == SAppLayoutOperation::Generate) { \
			LAYOUT_CONTROL_GENERATE_##TYPE(attr->NAME##s[0], SETFUNC, CONTROL, "sl_false, %s", value) \
			LAYOUT_CONTROL_GENERATE_##TYPE(attr->NAME##s[1], SETFUNC, CONTROL, "sl_true, %s", value) \
		} else if (IsSimulateOp(op)) { \
			LAYOUT_CONTROL_SIMULATE_##TYPE(attr->NAME##s[0], SETFUNC, CONTROL, sl_false, value) \
			LAYOUT_CONTROL_SIMULATE_##TYPE(attr->NAME##s[1], SETFUNC, CONTROL, sl_true, value) \
		}

#define LAYOUT_CONTROL_SWITCH_STATE_MAP(TYPE, NAME, SETFUNC, ...) \
		if (op == SAppLayoutOperation::Parse) { \
			LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, *(resourceItem), #NAME "Off", , attr->NAME##s[0], ##__VA_ARGS__) \
			LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, *(resourceItem), #NAME "On", , attr->NAME##s[1], ##__VA_ARGS__) \
		} else if (op == SAppLayoutOperation::Generate) { \
			LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, attr->NAME##s[0], SETFUNC, CONTROL, "sl_false, %s", value) \
			LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, attr->NAME##s[1], SETFUNC, CONTROL, "sl_true, %s", value) \
		} else if (IsSimulateOp(op)) { \
			LAYOUT_CONTROL_SIMULATE_STATE_MAP(TYPE, attr->NAME##s[0], SETFUNC, CONTROL, sl_false, value) \
			LAYOUT_CONTROL_SIMULATE_STATE_MAP(TYPE, attr->NAME##s[1], SETFUNC, CONTROL, sl_true, value) \
		}

		LAYOUT_CONTROL_UI_ATTR(STRING, text, setText)
		LAYOUT_CONTROL_SWITCH_ATTR(STRING, text, setText)
		LAYOUT_CONTROL_UI_ATTR(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_SWITCH_ATTR(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, thumb, setThumb)
		LAYOUT_CONTROL_SWITCH_STATE_MAP(DRAWABLE, thumb, setThumb)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, track, setTrack)
		LAYOUT_CONTROL_SWITCH_STATE_MAP(DRAWABLE, track, setTrack)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Picker, PickerView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_ATTR(GENERIC, circular, setCircular)
		LAYOUT_CONTROL_ATTR(GENERIC, lineCount, setLineCount)

		LAYOUT_CONTROL_SET_NATIVE_WIDGET

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS
		LAYOUT_CONTROL_UI_ATTR(STRING, value, selectValue)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(DatePicker, DatePicker)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, date, setDate)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Pager, ViewPager)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ATTR(GENERIC, loop, setLoop)

		if (op == SAppLayoutOperation::Parse) {
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(itemXmls, "item")
			for (sl_size i = 0; i < itemXmls.count; i++) {
				LAYOUT_CONTROL_DEFINE_XML(itemXml, itemXmls[i])
				SAppLayoutPagerItem subItem;
				LAYOUT_CONTROL_PARSE_XML(GENERIC, itemXml, subItem., selected)
				LAYOUT_CONTROL_DEFINE_XML_CHILDREN(childXmls, itemXml, sl_null)
				if (childXmls.count > 0) {
					if (childXmls.count != 1) {
						logError(itemXml.element, g_str_error_resource_layout_must_contain_one_child);
						return sl_false;
					}
					Ref<SAppLayoutResourceItem> subItemView = _parseLayoutResourceItemChild(resource, resourceItem, childXmls[0], params->source);
					if (subItemView.isNull()) {
						return sl_false;
					}
					if (IsNoView(subItemView->itemType)) {
						return sl_false;
					}
					((SAppLayoutViewAttributes*)(subItemView->attrs.get()))->resetLayout();
					subItem.view = subItemView;
				}
				if (!(attr->items.add_NoLock(Move(subItem)))) {
					logError(itemXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == SAppLayoutOperation::Generate) {
			/* see below */
		} else if (IsSimulateOp(op)) {
			ListElements<SAppLayoutPagerItem> subItems(attr->items);
			if (subItems.count > 0) {
				sl_uint32 indexSelected = 0;
				sl_uint32 nSubItems = (sl_uint32)(subItems.count);
				for (sl_uint32 i = 0; i < nSubItems; i++) {
					SAppLayoutPagerItem& subItem = subItems[i];
					if (subItem.selected.flagDefined && subItem.selected.value) {
						indexSelected = i;
					}
					if (subItem.view.isNotNull()) {
						Ref<View> contentView = CastRef<View>(_simulateLayoutCreateOrLayoutItem(params->simulator, subItem.view.get(), resourceItem, view, op));
						if (contentView.isNotNull()) {
							if (op == SAppLayoutOperation::SimulateInit) {
								view->addPage(contentView, UIUpdateMode::Init);
							}
						} else {
							return sl_false;
						}
					}
				}
				if (op == SAppLayoutOperation::SimulateInit) {
					view->selectPage(indexSelected);
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == SAppLayoutOperation::Generate) {
			ListElements<SAppLayoutPagerItem> subItems(attr->items);
			if (subItems.count > 0) {
				sl_size indexSelected = 0;
				for (sl_size i = 0; i < subItems.count; i++) {
					SAppLayoutPagerItem& subItem = subItems[i];
					if (subItem.view.isNotNull()) {
						String addChildStatement = String::format("%s%s->addPage(%s, slib::UIUpdateMode::Init);%n%n", strTab, name, subItem.view->name);
						if (!(_generateLayoutsCpp_Item(resource, subItem.view.get(), resourceItem, params, addChildStatement))) {
							return sl_false;
						}
					}
					if (subItem.selected.flagDefined && subItem.selected.value) {
						indexSelected = i;
					}
				}
				LAYOUT_CONTROL_GENERATE(selectPage, "%d", indexSelected)
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Navigation, ViewPageNavigationController)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ATTR(GENERIC, swipe, setSwipeNavigation)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Audio, AudioView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ATTR(GENERIC, framesPerPacket, setFramesPerPacket)
		LAYOUT_CONTROL_ATTR(GENERIC, packetsPerWindow, setPacketsPerWindow)
		LAYOUT_CONTROL_UI_ATTR(COLOR, amplitudeColor, setAmplitudeColor)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, amplitudeScale, setAmplitudeScale)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Video, VideoView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ATTR(GENERIC, repeat, setRepeat)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, rotation, setRotation)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, flip, setFlip)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, scale, setScaleMode)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, controls, setControlsVisible)

		if (op == SAppLayoutOperation::Parse || op == SAppLayoutOperation::Generate) {
			LAYOUT_CONTROL_ATTR(STRING, src, setSource)
		} else if (op == SAppLayoutOperation::SimulateInit) {
			if (attr->src.flagDefined) {
				String value;
				if (!(_getStringValue(resource->fileNamespace, attr->src, value))) {
					return sl_false;
				}
				if (value.startsWith("asset://")) {
					value = String::concat(m_conf.app_path, "/asset/", value.substring(8));
				}
				view->setSource(value);
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Camera, CameraView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(Video)

		LAYOUT_CONTROL_ATTR(STRING, device, setDeviceId)
		LAYOUT_CONTROL_ATTR(GENERIC, autoStart, setAutoStart)
		LAYOUT_CONTROL_ATTR(GENERIC, touchFocus, setTouchFocusEnabled)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Drawer, Drawer)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ATTR(DIMENSION, drawerSize, setDrawerSize, checkScalarSize)
		LAYOUT_CONTROL_ATTR(DIMENSION, dragEdgeSize, setDragEdgeSize, checkScalarSize)
		LAYOUT_CONTROL_ATTR(GENERIC, gravity, setGravity)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Refresh, RefreshView)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == SAppLayoutOperation::Parse) {
			sl_size nChildren = element->getChildElementCount();
			if (nChildren > 0) {
				if (nChildren != 1) {
					logError(element, g_str_error_resource_layout_must_contain_one_child);
					return sl_false;
				}
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(ListBox, ListBox)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, itemCount, setItemCount)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemHeight, setItemHeight, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, multipleSelection, setMultipleSelection)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, itemBackground, setItemBackground)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(LabelList, LabelList)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(ListBox)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, lineHeightWeight, setLineHeightWeight)
		LAYOUT_CONTROL_STATE_MAP(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, ellipsize, setEllipsize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, multiLine, setMultiLine)

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemPaddingLeft, setItemPaddingLeft, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemPaddingTop, setItemPaddingTop, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemPaddingRight, setItemPaddingRight, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemPaddingBottom, setItemPaddingBottom, checkPosition)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue itemPadding;
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, , itemPadding, checkPosition)
			if (itemPadding.flagDefined) {
				if (!(attr->itemPaddingLeft.flagDefined)) {
					attr->itemPaddingLeft = itemPadding;
				}
				if (!(attr->itemPaddingTop.flagDefined)) {
					attr->itemPaddingTop = itemPadding;
				}
				if (!(attr->itemPaddingRight.flagDefined)) {
					attr->itemPaddingRight = itemPadding;
				}
				if (!(attr->itemPaddingBottom.flagDefined)) {
					attr->itemPaddingBottom = itemPadding;
				}
			}
		}

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS
		LAYOUT_CONTROL_UI_ATTR(STRING, value, selectValue)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Pdf, PdfView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Map, MapView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(Render)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, globe, setGlobeMode)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, eyeLocation, setEyeLocation)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, eyeRotation, setEyeRotation)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, eyeTilt, setEyeTilt)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, mapScale, setMapScale)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, minimumAltitude, setMinimumAltitude)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, maximumAltitude, setMaximumAltitude)
		LAYOUT_CONTROL_ATTR(GENERIC, minimumDistanceFromGround, setMinimumDistanceFromGround)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, tileGrid, setTileGridVisible)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, terrainGrid, setTerrainGridVisible)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, compass, setCompass)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, compassSize, setCompassSize, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, compassCenter, setCompassCenter)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, compassAlign, setCompassAlignment)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, compassMarginLeft, setCompassMarginLeft, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, compassMarginTop, setCompassMarginTop, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, compassMarginRight, setCompassMarginRight, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, compassMarginBottom, setCompassMarginBottom, checkPosition)
		if (op == SAppLayoutOperation::Parse) {
			SAppDimensionValue compassMargin;
			LAYOUT_CONTROL_PARSE_ATTR(MARGIN, , compassMargin)
			if (compassMargin.flagDefined) {
				if (!(attr->compassMarginLeft.flagDefined)) {
					attr->compassMarginLeft = compassMargin;
				}
				if (!(attr->compassMarginTop.flagDefined)) {
					attr->compassMarginTop = compassMargin;
				}
				if (!(attr->compassMarginRight.flagDefined)) {
					attr->compassMarginRight = compassMargin;
				}
				if (!(attr->compassMarginBottom.flagDefined)) {
					attr->compassMarginBottom = compassMargin;
				}
			}
		}
		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(GroupBox, GroupBox)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(STRING, label, setLabel)
		LAYOUT_CONTROL_UI_ATTR(COLOR, labelColor, setLabelColor)
		LAYOUT_CONTROL_UI_ATTR(FONT, labelFont, setLabelFont)
		if (op == SAppLayoutOperation::Parse) {
			if (attr->labelFont.flagDefined) {
				attr->labelFont.inheritFrom(attr->font);
			}
		}
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, icon, setIcon)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconSize, setIconSize, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconWidth, setIconWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconHeight, setIconHeight, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMargin, setIconMargin, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginLeft, setIconMarginLeft, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginRight, setIconMarginRight, checkScalarSize)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	sl_bool SAppDocument::_processLayoutResourceControl_Grid_ParseCellCreator(SAppLayoutGridCellAttributes& attr, const String& tagName, SAppLayoutXmlItem& xml)
	{
		#define DEFINE_CHECK_GRID_CELL_CREATOR(NAME, TAG) \
			if (tagName == TAG) { \
				creator = SAppLayoutGridCellAttributes::Creator::NAME; \
				break; \
			}

		SAppLayoutGridCellAttributes::Creator creator = SAppLayoutGridCellAttributes::Creator::None;
		do {
			DEFINE_CHECK_GRID_CELL_CREATOR(None, "cell")
			DEFINE_CHECK_GRID_CELL_CREATOR(Text, "text")
			DEFINE_CHECK_GRID_CELL_CREATOR(HyperText, "hyper")
			DEFINE_CHECK_GRID_CELL_CREATOR(Numero, "no")
			DEFINE_CHECK_GRID_CELL_CREATOR(Sort, "sort")
			DEFINE_CHECK_GRID_CELL_CREATOR(Icon, "icon")
			DEFINE_CHECK_GRID_CELL_CREATOR(Button, "button")
			return sl_false;
		} while(0);
		attr.creator = creator;
		switch (creator) {
			case SAppLayoutGridCellAttributes::Creator::Numero:
				LAYOUT_CONTROL_PARSE_XML(GENERIC, xml, attr.numero., start)
				break;
			default:
				break;
		}
		return sl_true;
	}

	String SAppDocument::_processLayoutResourceControl_Grid_GenerateCellCreator(SAppLayoutGridCellAttributes& attr)
	{
		switch (attr.creator) {
			case SAppLayoutGridCell::Creator::Text:
				return "slib::GridView::TextCell::creator()";
			case SAppLayoutGridCell::Creator::HyperText:
				return "slib::GridView::HyperTextCell::creator()";
			case SAppLayoutGridCell::Creator::Numero:
				if (attr.numero.start.flagDefined) {
					return String::format("slib::GridView::NumeroCell::creator(%s)", attr.numero.start.getAccessString());
				} else {
					return "slib::GridView::NumeroCell::creator()";
				}
			case SAppLayoutGridCell::Creator::Sort:
				return "slib::GridView::SortCell::creator()";
			case SAppLayoutGridCell::Creator::Icon:
				return "slib::GridView::IconCell::creator()";
			case SAppLayoutGridCell::Creator::Button:
				return "slib::GridView::ButtonCell::creator()";
			default:
				return sl_null;
		}
	}

	GridView::CellCreator SAppDocument::_processLayoutResourceControl_Grid_SimulateCellCreator(SAppLayoutGridCellAttributes& attr)
	{
		switch (attr.creator) {
			case SAppLayoutGridCell::Creator::Text:
				return GridView::TextCell::creator();
			case SAppLayoutGridCell::Creator::HyperText:
				return GridView::HyperTextCell::creator();
			case SAppLayoutGridCell::Creator::Numero:
				if (attr.numero.start.flagDefined) {
					return GridView::NumeroCell::creator(attr.numero.start.value);
				} else {
					return GridView::NumeroCell::creator();
				}
			case SAppLayoutGridCell::Creator::Sort:
				return GridView::SortCell::creator();
			case SAppLayoutGridCell::Creator::Icon:
				return GridView::IconCell::creator();
			case SAppLayoutGridCell::Creator::Button:
				return GridView::ButtonCell::creator();
			default:
				return sl_null;
		}
	}

	BEGIN_PROCESS_LAYOUT_CONTROL(Grid, GridView)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, maxColumnWidth, setMaximumColumnWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, minColumnWidth, setMinimumColumnWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, columnWidth, setColumnWidth, checkScalarSize)
		LAYOUT_CONTROL_ATTR(GENERIC, resizableColumn, setColumnResizable)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, rowHeight, setRowHeight, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(BORDER, grid, setGrid)
		LAYOUT_CONTROL_UI_ATTR(BORDER, leftGrid, setLeftGrid)
		LAYOUT_CONTROL_UI_ATTR(BORDER, rightGrid, setRightGrid)
		LAYOUT_CONTROL_ATTR(GENERIC, sort, setSorting)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, ascendingIcon, setAscendingIcon)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, descendingIcon, setDescendingIcon)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, sortIconSize, setSortIconSize, checkScalarSize)
		LAYOUT_CONTROL_ATTR(GENERIC, selection, setSelectionMode)
		LAYOUT_CONTROL_UI_ATTR(BORDER, selectionBorder, setSelectionBorder)
		LAYOUT_CONTROL_ATTR(GENERIC, verticalGrid, setVerticalGrid)
		LAYOUT_CONTROL_ATTR(GENERIC, horizontalGrid, setHorizontalGrid)
		LAYOUT_CONTROL_ATTR(GENERIC, cellCursor, setCellCursor)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, cellPadding, setCellPadding, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, cellPaddingLeft, setCellPaddingLeft, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, cellPaddingTop, setCellPaddingTop, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, cellPaddingRight, setCellPaddingRight, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, cellPaddingBottom, setCellPaddingBottom, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, multiLine, setCellMultiLine)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, ellipsize, setCellEllipsize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, lineCount, setCellLineCount)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, cellAlign, setCellAlignment)
		LAYOUT_CONTROL_ATTR(GENERIC, selectable, setCellSelectable)
		LAYOUT_CONTROL_ATTR(GENERIC, editable, setCellEditable)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, cellAntiAlias, setCellAntiAlias)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, cellBackgroundAntiAlias, setCellBackgroundAntiAlias)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, cellContentAntiAlias, setCellContentAntiAlias)
		LAYOUT_CONTROL_ATTR(GENERIC, defaultColorFilter, setCellUsingDefaultColorFilter)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconWidth, setCellIconWidth, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMargin, setCellIconMargin, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginLeft, setCellIconMarginLeft, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginTop, setCellIconMarginTop, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginRight, setCellIconMarginRight, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, iconMarginBottom, setCellIconMarginBottom, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, iconScale, setCellIconScaleMode)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, iconAlign, setCellIconAlignment)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, cellBackground, setCellBackground)
		LAYOUT_CONTROL_STATE_MAP(COLOR, textColor, setCellTextColor)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, data, setData)

		if (op == SAppLayoutOperation::Parse) {

#define LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(ATTR, XML) \
			{ \
				LAYOUT_CONTROL_PARSE_XML(STRING, XML, ATTR., field) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., align) \
				LAYOUT_CONTROL_PARSE_XML(FONT, XML, ATTR., font) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., cursor) \
				LAYOUT_CONTROL_PARSE_XML(STRING, XML, ATTR., toolTip) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., padding, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., paddingLeft, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., paddingTop, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., paddingRight, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., paddingBottom, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., multiLine) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., ellipsize) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., lineCount) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., selectable) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., editable) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., antiAlias) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., backgroundAntiAlias) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., contentAntiAlias) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., defaultColorFilter) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., iconWidth, checkScalarSize) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., iconMargin, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., iconMarginLeft, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., iconMarginTop, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., iconMarginRight, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, ATTR., iconMarginBottom, checkPosition) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., iconScale) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., iconAlign) \
				LAYOUT_CONTROL_PARSE_STATE_MAP_XML(DRAWABLE, XML, ATTR., background) \
				LAYOUT_CONTROL_PARSE_STATE_MAP_XML(COLOR, XML, ATTR., textColor) \
				LAYOUT_CONTROL_PARSE_STATE_MAP_XML(DRAWABLE, XML, ATTR., icon) \
			}
#define LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES_OF_SECTION(ATTR, XML, SECTION) \
			{ \
				LAYOUT_CONTROL_PARSE_STRING(XML, #SECTION "Field", , ATTR.field) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "Align", , ATTR.align) \
				LAYOUT_CONTROL_PARSE_FONT(XML, #SECTION "Font", , ATTR.font) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "Cursor", , ATTR.cursor) \
				LAYOUT_CONTROL_PARSE_STRING(XML, #SECTION "ToolTip", , ATTR.toolTip) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "Padding", , ATTR.padding, checkPosition) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "PaddingLeft", , ATTR.paddingLeft, checkPosition) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "PaddingTop", , ATTR.paddingTop, checkPosition) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "PaddingRight", , ATTR.paddingRight, checkPosition) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "PaddingBottom", , ATTR.paddingBottom, checkPosition) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "MultiLine", , ATTR.multiLine) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "Ellipsize", , ATTR.ellipsize) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "LineCount", , ATTR.lineCount) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "Selectable", , ATTR.selectable) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "Editable", , ATTR.editable) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "AntiAlias", , ATTR.antiAlias) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "BackgroundAntiAlias", , ATTR.backgroundAntiAlias) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "ContentAntiAlias", , ATTR.contentAntiAlias) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "DefaultColorFilter", , ATTR.defaultColorFilter) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "IconWidth", , ATTR.iconWidth, checkScalarSize) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "IconMargin", , ATTR.iconMargin, checkPosition) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "IconMarginLeft", , ATTR.iconMarginLeft, checkPosition) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "IconMarginTop", , ATTR.iconMarginTop, checkPosition) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "IconMarginRight", , ATTR.iconMarginRight, checkPosition) \
				LAYOUT_CONTROL_PARSE_DIMENSION(XML, #SECTION "IconMarginBottom", , ATTR.iconMarginBottom, checkPosition) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "IconScale", , ATTR.iconScale) \
				LAYOUT_CONTROL_PARSE_GENERIC(XML, #SECTION "IconAlign", , ATTR.iconAlign) \
				LAYOUT_CONTROL_PARSE_STATE_MAP(DRAWABLE, XML, #SECTION "Background", , ATTR.background) \
				LAYOUT_CONTROL_PARSE_STATE_MAP(COLOR, XML, #SECTION "TextColor", , ATTR.textColor) \
				LAYOUT_CONTROL_PARSE_STATE_MAP(DRAWABLE, XML, #SECTION "Icon", , ATTR.icon) \
			}
			{
				LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(columnXmls, "column")
				for (sl_size i = 0; i < columnXmls.count; i++) {
					LAYOUT_CONTROL_DEFINE_XML(columnXml, columnXmls[i])
					SAppLayoutGridColumn column;
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml.element, column., name)
					if (column.name.flagDefined) {
						if (!(_checkLayoutResourceItemName(resource, column.name.value, columnXml.element))) {
							return sl_false;
						}
						resource->otherNames.put(column.name.value, sl_true);
					}
					LAYOUT_CONTROL_PARSE_XML(STRING, columnXml, column., id)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., minWidth, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., maxWidth, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., width, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., fixed)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., visible)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., resizable)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., verticalGrid)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., bodyVerticalGrid)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., headerVerticalGrid)
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, column., footerVerticalGrid)
					LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(column, columnXml)
					LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES_OF_SECTION(column.bodyAttrs, columnXml, body)
					LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES_OF_SECTION(column.headerAttrs, columnXml, header)
					LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES_OF_SECTION(column.footerAttrs, columnXml, footer)
					String text = columnXml.getXmlText();
					if (text.isNotEmpty()) {
						if (!(column.bodyAttrs.text.parse(text, columnXml.element))) {
							logError(columnXml.element, g_str_error_resource_layout_value_invalid, text);
							return sl_false;
						}
					}
					String title = columnXml.getXmlAttribute("title");
					if (title.isNotEmpty()) {
						if (!(column.headerAttrs.text.parse(title, columnXml.element))) {
							logError(columnXml.element, g_str_error_resource_layout_value_invalid, title);
							return sl_false;
						}
					}
					String type = columnXml.getXmlAttribute("type");
					if (type.isNotEmpty()) {
						if (!(_processLayoutResourceControl_Grid_ParseCellCreator(column.bodyAttrs, type, columnXml))) {
							logError(columnXml.element, g_str_error_resource_layout_gridview_unknown_cell_creator, type);
							return sl_false;
						}
					}
					SAppBooleanValue sort;
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, , sort)
					if (sort.flagDefined) {
						if (sort.value) {
							column.headerAttrs.creator = SAppLayoutGridCellAttributes::Creator::Sort;
						} else {
							column.headerAttrs.creator = SAppLayoutGridCellAttributes::Creator::Text;
						}
					}
					if (!(attr->columns.add_NoLock(Move(column)))) {
						logError(columnXml.element, g_str_error_out_of_memory);
						return sl_false;
					}
				}
			}

#define LAYOUT_CONTROL_PARSE_GRID_ROWS(SECTION) \
			if (rowXmls.count) { \
				CHashMap< Pair<sl_uint32, sl_uint32>, sl_bool > cellAllocs; \
				for (sl_size iRow = 0; iRow < rowXmls.count; iRow++) { \
					LAYOUT_CONTROL_DEFINE_XML(rowXml, rowXmls[iRow]) \
					SAppLayoutGridRow row; \
					LAYOUT_CONTROL_PARSE_XML(GENERIC, rowXml.element, row., name) \
					if (row.name.flagDefined) { \
						if (!(_checkLayoutResourceItemName(resource, row.name.value, rowXml.element))) { \
							return sl_false; \
						} \
						resource->otherNames.put(row.name.value, sl_true); \
					} \
					LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(row, rowXml) \
					row.font.inheritFrom(attr->SECTION.font); \
					LAYOUT_CONTROL_PARSE_XML(STRING, rowXml, row., id) \
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., height, checkScalarSize) \
					LAYOUT_CONTROL_PARSE_XML(GENERIC, rowXml, row., visible) \
					LAYOUT_CONTROL_PARSE_XML(GENERIC, rowXml, row., horizontalGrid) \
					sl_uint32 iCell = 0; \
					LAYOUT_CONTROL_DEFINE_XML_CHILDREN(cellXmls, rowXml, sl_null) \
					for (sl_size k = 0; k < cellXmls.count; k++) { \
						LAYOUT_CONTROL_DEFINE_XML(cellXml, cellXmls[k]) \
						SAppLayoutGridCell cell; \
						if (!(_processLayoutResourceControl_Grid_ParseCellCreator(cell, cellXml.getTagName(), cellXml))) { \
							logError(cellXml.element, g_str_error_resource_layout_gridview_unknown_cell_creator, cellXml.getTagName()); \
							return sl_false; \
						} \
						String text = cellXml.getXmlText(); \
						if (text.isNotEmpty()) { \
							if (!(cell.text.parse(text, cellXml.element))) { \
								logError(cellXml.element, g_str_error_resource_layout_value_invalid, text); \
								return sl_false; \
							} \
						} \
						LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(cell, cellXml) \
						cell.font.inheritFrom(row.font); \
						SAppLayoutGridColumn* col = attr->columns.getPointerAt(k); \
						if (col) { \
							if (!(cell.font.flagDefined) && (col->font.flagDefined || col->SECTION##Attrs.font.flagDefined) && row.font.flagDefined) { \
								cell.font.flagDefined = sl_true; \
							} \
							cell.font.inheritFrom(col->SECTION##Attrs.font); \
 						} \
						cell.font.inheritFrom(attr->font); \
						LAYOUT_CONTROL_PARSE_XML(GENERIC, cellXml, cell., rowspan) \
						LAYOUT_CONTROL_PARSE_XML(GENERIC, cellXml, cell., colspan) \
						if (!(cell.rowspan.flagDefined) || cell.rowspan.value < 1) { \
							cell.rowspan.value = 1; \
						} \
						if (!(cell.colspan.flagDefined) || cell.colspan.value < 1) { \
							cell.colspan.value = 1; \
						} \
						while (cellAllocs.find_NoLock(Pair<sl_uint32, sl_uint32>((sl_uint32)iRow, (sl_uint32)iCell))) { \
							iCell++; \
						} \
						if (iCell + cell.colspan.value > attr->columns.getCount()) { \
							if (!(attr->columns.setCount_NoLock(iCell+cell.colspan.value))) { \
								logError(cellXml.element, g_str_error_out_of_memory); \
								return sl_false; \
							} \
						} \
						for (sl_uint32 t1 = 0; t1 < cell.rowspan.value; t1++) { \
							for (sl_uint32 t2 = 0; t2 < cell.colspan.value; t2++) { \
								cellAllocs.put_NoLock(Pair<sl_uint32, sl_uint32>((sl_uint32)iRow + t1, (sl_uint32)iCell + t2), sl_true); \
							} \
						} \
						if (!(row.cells.setCount_NoLock(iCell+1))) { \
							logError(cellXml.element, g_str_error_out_of_memory); \
							return sl_false; \
						} \
						row.cells.setAt_NoLock(iCell, Move(cell)); \
					} \
					row.font.inheritFrom(attr->font); \
					if (!(attr->SECTION.rows.add_NoLock(Move(row)))) { \
						logError(rowXml.element, g_str_error_out_of_memory); \
						return sl_false; \
					} \
				} \
			}

#define LAYOUT_CONTROL_PARSE_GRID_SECTION(SECTION, XML) \
			LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES_OF_SECTION(attr->SECTION, *resourceItem, SECTION) \
			LAYOUT_CONTROL_PARSE_DIMENSION(*resourceItem, #SECTION "RowHeight", , attr->SECTION.rowHeight, checkScalarSize) \
			LAYOUT_CONTROL_PARSE_BORDER(*resourceItem, #SECTION "Grid", , attr->SECTION.grid) \
			LAYOUT_CONTROL_PARSE_GENERIC(*resourceItem, #SECTION "VerticalGrid", , attr->SECTION.verticalGrid) \
			LAYOUT_CONTROL_PARSE_GENERIC(*resourceItem, #SECTION "HorizontalGrid", , attr->SECTION.horizontalGrid) \
			if (XML.element.isNotNull()) { \
				LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(attr->SECTION, XML) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, attr->SECTION., rowHeight, checkScalarSize) \
				LAYOUT_CONTROL_PARSE_XML(BORDER, XML, attr->SECTION., grid) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, attr->SECTION., verticalGrid) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, attr->SECTION., horizontalGrid) \
				LAYOUT_CONTROL_DEFINE_XML_CHILDREN(rowXmls, XML, "row") \
				LAYOUT_CONTROL_PARSE_GRID_ROWS(SECTION) \
				attr->SECTION.font.inheritFrom(attr->font); \
			}

			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(bodyList, "body")
			LAYOUT_CONTROL_DEFINE_XML(body, bodyList.count ? bodyList[0] : Ref<XmlElement>::null())
			LAYOUT_CONTROL_PARSE_GRID_SECTION(body, body)
			if (bodyList.count) {
				if (bodyList.count != 1) {
					logError(bodyList[1], g_str_error_resource_layout_tag_redefined);
					return sl_false;
				}
			} else {
				LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(rowXmls, "row")
				LAYOUT_CONTROL_PARSE_GRID_ROWS(body)
				if (attr->body.rows.isEmpty()) {
					attr->body.rows.setCount_NoLock(1);
				}
			}
			
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(headerList, "header")
			LAYOUT_CONTROL_DEFINE_XML(header, headerList.count ? headerList[0] : Ref<XmlElement>::null())
			LAYOUT_CONTROL_PARSE_GRID_SECTION(header, header)
			if (headerList.count) {
				if (headerList.count != 1) {
					logError(headerList[1], g_str_error_resource_layout_tag_redefined);
					return sl_false;
				}
			} else {
				if (attr->header.rows.isEmpty()) {
					attr->header.rows.setCount_NoLock(1);
				}
			}

			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(footerList, "footer")
			LAYOUT_CONTROL_DEFINE_XML(footer, footerList.count ? footerList[0] : Ref<XmlElement>::null())
			LAYOUT_CONTROL_PARSE_GRID_SECTION(footer, footer)
			if (footerList.count) {
				if (footerList.count != 1) {
					logError(footerList[1], g_str_error_resource_layout_tag_redefined);
					return sl_false;
				}
			}

			{
				ListElements<SAppLayoutGridColumn> columns(attr->columns);
				sl_bool flagLeft = sl_true;
				for (sl_size i = 0; i < columns.count; i++) {
					SAppLayoutGridColumn& column = columns[i];
					column.font.inheritFrom(attr->font);
					column.headerAttrs.font.inheritFrom(attr->font);
					column.bodyAttrs.font.inheritFrom(attr->font);
					column.footerAttrs.font.inheritFrom(attr->font);
					if (!(column.fixed.flagDefined && column.fixed.value)) {
						flagLeft = sl_false;
						attr->nRightColumns = (sl_uint32)(columns.count - 1 - i);
					}
					if (flagLeft) {
						attr->nLeftColumns = (sl_uint32)(i + 1);
					}
				}
			}

		} else if (op == SAppLayoutOperation::Generate) {

			ListElements<SAppLayoutGridColumn> columns(attr->columns);

			LAYOUT_CONTROL_GENERATE(setColumnCount, "%d, slib::UIUpdateMode::Init", columns.count)
			LAYOUT_CONTROL_GENERATE(setLeftColumnCount, "%d, slib::UIUpdateMode::Init", attr->nLeftColumns)
			LAYOUT_CONTROL_GENERATE(setRightColumnCount, "%d, slib::UIUpdateMode::Init", attr->nRightColumns)
			LAYOUT_CONTROL_GENERATE(setBodyRowCount, "%d, slib::UIUpdateMode::Init", attr->body.rows.getCount())
			LAYOUT_CONTROL_GENERATE(setHeaderRowCount, "%d, slib::UIUpdateMode::Init", attr->header.rows.getCount())
			LAYOUT_CONTROL_GENERATE(setFooterRowCount, "%d, slib::UIUpdateMode::Init", attr->footer.rows.getCount())

#define LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(PREFIX, ATTR, ARG_FORMAT, ...) \
			{ \
				{ \
					auto value = _processLayoutResourceControl_Grid_GenerateCellCreator(ATTR); \
					if (value.isNotNull()) { \
						LAYOUT_CONTROL_GENERATE(set##PREFIX##Creator, ARG_FORMAT ", slib::UIUpdateMode::Init", ##__VA_ARGS__) \
					} \
				} \
				LAYOUT_CONTROL_GENERATE_STRING(ATTR.field, set##PREFIX##Field, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_STRING(ATTR.text, set##PREFIX##Text, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.align, set##PREFIX##Alignment, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_FONT(ATTR.font, set##PREFIX##Font, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.cursor, set##PREFIX##Cursor, BASIC, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_STRING(ATTR.toolTip, set##PREFIX##ToolTip, BASIC, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.padding, set##PREFIX##Padding, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.paddingLeft, set##PREFIX##PaddingLeft, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.paddingTop, set##PREFIX##PaddingTop, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.paddingRight, set##PREFIX##PaddingRight, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.paddingBottom, set##PREFIX##PaddingBottom, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.multiLine, set##PREFIX##MultiLine, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.ellipsize, set##PREFIX##Ellipsize, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.lineCount, set##PREFIX##LineCount, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.selectable, set##PREFIX##Selectable, BASIC, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.editable, set##PREFIX##Editable, BASIC, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.antiAlias, set##PREFIX##AntiAlias, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.backgroundAntiAlias, set##PREFIX##BackgroundAntiAlias, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.contentAntiAlias, set##PREFIX##ContentAntiAlias, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.defaultColorFilter, set##PREFIX##UsingDefaultColorFilter, BASIC, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.iconWidth, set##PREFIX##IconWidth, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.iconMargin, set##PREFIX##IconMargin, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.iconMarginLeft, set##PREFIX##IconMarginLeft, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.iconMarginTop, set##PREFIX##IconMarginTop, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.iconMarginRight, set##PREFIX##IconMarginRight, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_DIMENSION(ATTR.iconMarginBottom, set##PREFIX##IconMarginBottom, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.iconScale, set##PREFIX##IconScaleMode, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.iconAlign, set##PREFIX##IconAlignment, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_STATE_MAP(DRAWABLE, ATTR.background, set##PREFIX##Background, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_STATE_MAP(COLOR, ATTR.textColor, set##PREFIX##TextColor, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_STATE_MAP(DRAWABLE, ATTR.icon, set##PREFIX##Icon, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
			}

#define LAYOUT_CONTROL_GENERATE_GRID_SECTION(SECTION, PREFIX) \
			{ \
				auto& section = attr->SECTION; \
				LAYOUT_CONTROL_GENERATE_UI_ATTR(DIMENSION, section.rowHeight, set##PREFIX##RowHeight) \
				LAYOUT_CONTROL_GENERATE_UI_ATTR(BORDER, section.grid, set##PREFIX##Grid) \
				LAYOUT_CONTROL_GENERATE_UI_ATTR(GENERIC, section.verticalGrid, set##PREFIX##VerticalGrid) \
				LAYOUT_CONTROL_GENERATE_UI_ATTR(GENERIC, section.horizontalGrid, set##PREFIX##VerticalGrid) \
				LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(PREFIX, section, "-1, -1, %s", value) \
			}

			LAYOUT_CONTROL_GENERATE_GRID_SECTION(body, Body)
			LAYOUT_CONTROL_GENERATE_GRID_SECTION(header, Header)
			LAYOUT_CONTROL_GENERATE_GRID_SECTION(footer, Footer)

			{
				for (sl_size iCol = 0; iCol < columns.count; iCol++) {
					SAppLayoutGridColumn& column = columns[iCol];
					if (column.name.flagDefined) {
						params->sbDeclare->add(String::format("\t\t\tslib::Ref<slib::GridViewColumn> %s;%n", column.name.value));
						params->sbDefineInit->add(String::format("\t\t\t%s = %s->getColumn(%d);%n", column.name.value, resourceItem->name, iCol));
					}
					LAYOUT_CONTROL_GENERATE_STRING(column.id, setColumnId, BASIC, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_DIMENSION(column.maxWidth, setMaximumColumnWidth, ITEM, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_DIMENSION(column.minWidth, setMinimumColumnWidth, ITEM, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_DIMENSION(column.width, setColumnWidth, ITEM, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(column.visible, setColumnVisible, ITEM, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(column.resizable, setColumnResizable, BASIC, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(column.verticalGrid, setVerticalGrid, ITEM, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(column.bodyVerticalGrid, setBodyVerticalGrid, ITEM, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(column.headerVerticalGrid, setHeaderVerticalGrid, ITEM, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GENERIC(column.footerVerticalGrid, setFooterVerticalGrid, ITEM, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(Column, column, "%d, %s", iCol, value)
				}
			}

#define LAYOUT_CONTROL_GENERATE_GRID_ROWS(SECTION, PREFIX) \
			{ \
				for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
					SAppLayoutGridColumn& column = columns[iCol]; \
					LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(PREFIX, column.SECTION##Attrs, "-1, %d, %s", iCol, value) \
				} \
			} \
			{ \
				auto& section = attr->SECTION; \
				ListElements<SAppLayoutGridRow> rows(section.rows); \
				for (sl_size iRow = 0; iRow < rows.count; iRow++) { \
					SAppLayoutGridRow& row = rows[iRow]; \
					if (row.name.flagDefined) { \
						params->sbDeclare->add(String::format("\t\t\tslib::Ref<slib::GridViewRow> %s;%n", row.name.value)); \
						params->sbDefineInit->add(String::format("\t\t\t%s = %s->get" #PREFIX "Row(%d);%n", row.name.value, resourceItem->name, iRow)); \
					} \
					LAYOUT_CONTROL_GENERATE_STRING(row.id, set##PREFIX##RowId, BASIC, "%d, %s", iRow, value) \
					LAYOUT_CONTROL_GENERATE_DIMENSION(row.height, set##PREFIX##RowHeight, ITEM, "%d, %s", iRow, value) \
					LAYOUT_CONTROL_GENERATE_GENERIC(row.visible, set##PREFIX##RowVisible, ITEM, "%d, %s", iRow, value) \
					LAYOUT_CONTROL_GENERATE_GENERIC(row.horizontalGrid, set##PREFIX##HorizontalGrid, ITEM, "%d, %s", iRow, value) \
					LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(PREFIX, row, "%d, -1, %s", iRow, value) \
					ListElements<SAppLayoutGridCell> cells(row.cells); \
					for (sl_size iCell = 0; iCell < cells.count; iCell++) { \
						SAppLayoutGridCell& cell = cells[iCell]; \
						LAYOUT_CONTROL_GENERATE_STRING(cell.text, set##PREFIX##Text, ITEM, "%d, %d, %s", iRow, iCell, value) \
						LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(PREFIX, cell, "%d, %d, %s", iRow, iCell, value) \
						if (cell.colspan.flagDefined && cell.rowspan.flagDefined) { \
							LAYOUT_CONTROL_GENERATE(set##PREFIX##Span, "%d, %d, %d, %d, slib::UIUpdateMode::Init", iRow, iCell, cell.rowspan.value, cell.colspan.value) \
						} else { \
							LAYOUT_CONTROL_GENERATE_GENERIC(cell.rowspan, set##PREFIX##Rowspan, ITEM, "%d, %d, %s", iRow, iCell, value) \
							LAYOUT_CONTROL_GENERATE_GENERIC(cell.colspan, set##PREFIX##Colspan, ITEM, "%d, %d, %s", iRow, iCell, value) \
						} \
					} \
				} \
			}

			LAYOUT_CONTROL_GENERATE_GRID_ROWS(body, Body)
			LAYOUT_CONTROL_GENERATE_GRID_ROWS(header, Header)
			LAYOUT_CONTROL_GENERATE_GRID_ROWS(footer, Footer)

		} else if (IsSimulateOp(op)) {

			ListElements<SAppLayoutGridColumn> columns(attr->columns);
			if (op == SAppLayoutOperation::SimulateInit) {
				view->setColumnCount((sl_uint32)(columns.count), UIUpdateMode::Init);
				view->setLeftColumnCount(attr->nLeftColumns, UIUpdateMode::Init);
				view->setRightColumnCount(attr->nRightColumns, UIUpdateMode::Init);
				view->setBodyRowCount((sl_uint32)(attr->body.rows.getCount()), UIUpdateMode::Init);
				view->setHeaderRowCount((sl_uint32)(attr->header.rows.getCount()), UIUpdateMode::Init);
				view->setFooterRowCount((sl_uint32)(attr->footer.rows.getCount()), UIUpdateMode::Init);
			}

#define LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(PREFIX, ATTR, ...) \
			{ \
				if (op == SAppLayoutOperation::SimulateInit) { \
					auto creator = _processLayoutResourceControl_Grid_SimulateCellCreator(ATTR); \
					if (creator.isNotNull()) { \
						view->set##PREFIX##Creator(__VA_ARGS__, creator, UIUpdateMode::Init); \
					} \
				} \
				LAYOUT_CONTROL_SIMULATE_STRING(ATTR.field, set##PREFIX##Field, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_STRING(ATTR.text, set##PREFIX##Text, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.align, set##PREFIX##Alignment, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_FONT(ATTR.font, set##PREFIX##Font, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.cursor, set##PREFIX##Cursor, BASIC, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_STRING(ATTR.toolTip, set##PREFIX##ToolTip, BASIC, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.padding, set##PREFIX##Padding, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.paddingLeft, set##PREFIX##PaddingLeft, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.paddingTop, set##PREFIX##PaddingTop, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.paddingRight, set##PREFIX##PaddingRight, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.paddingBottom, set##PREFIX##PaddingBottom, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.multiLine, set##PREFIX##MultiLine, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.ellipsize, set##PREFIX##Ellipsize, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.lineCount, set##PREFIX##LineCount, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.selectable, set##PREFIX##Selectable, BASIC, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.editable, set##PREFIX##Editable, BASIC, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.antiAlias, set##PREFIX##AntiAlias, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.backgroundAntiAlias, set##PREFIX##BackgroundAntiAlias, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.contentAntiAlias, set##PREFIX##ContentAntiAlias, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.defaultColorFilter, set##PREFIX##UsingDefaultColorFilter, BASIC, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.iconWidth, set##PREFIX##IconWidth, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.iconMargin, set##PREFIX##IconMargin, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.iconMarginLeft, set##PREFIX##IconMarginLeft, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.iconMarginTop, set##PREFIX##IconMarginTop, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.iconMarginRight, set##PREFIX##IconMarginRight, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_DIMENSION(ATTR.iconMarginBottom, set##PREFIX##IconMarginBottom, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.iconScale, set##PREFIX##IconScaleMode, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.iconAlign, set##PREFIX##IconAlignment, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_STATE_MAP(DRAWABLE, ATTR.background, set##PREFIX##Background, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_STATE_MAP(COLOR, ATTR.textColor, set##PREFIX##TextColor, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_STATE_MAP(DRAWABLE, ATTR.icon, set##PREFIX##Icon, ITEM, ##__VA_ARGS__, value) \
			}
			
#define LAYOUT_CONTROL_SIMULATE_GRID_SECTION(SECTION, PREFIX) \
			{ \
				auto& section = attr->SECTION; \
				LAYOUT_CONTROL_SIMULATE_UI_ATTR(DIMENSION, section.rowHeight, set##PREFIX##RowHeight) \
				LAYOUT_CONTROL_SIMULATE_UI_ATTR(BORDER, section.grid, set##PREFIX##Grid) \
				LAYOUT_CONTROL_SIMULATE_UI_ATTR(GENERIC, section.verticalGrid, set##PREFIX##VerticalGrid) \
				LAYOUT_CONTROL_SIMULATE_UI_ATTR(GENERIC, section.horizontalGrid, set##PREFIX##HorizontalGrid) \
				LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(PREFIX, section, -1, -1) \
			}

			LAYOUT_CONTROL_SIMULATE_GRID_SECTION(body, Body)
			LAYOUT_CONTROL_SIMULATE_GRID_SECTION(header, Header)
			LAYOUT_CONTROL_SIMULATE_GRID_SECTION(footer, Footer)

			{
				for (sl_size iCol = 0; iCol < columns.count; iCol++) {
					SAppLayoutGridColumn& column = columns[iCol];
					LAYOUT_CONTROL_SIMULATE_STRING(column.id, setColumnId, BASIC, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_DIMENSION(column.maxWidth, setMaximumColumnWidth, ITEM, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_DIMENSION(column.minWidth, setMinimumColumnWidth, ITEM, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_DIMENSION(column.width, setColumnWidth, ITEM, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(column.visible, setColumnVisible, ITEM, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(column.resizable, setColumnResizable, BASIC, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(column.verticalGrid, setVerticalGrid, ITEM, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(column.bodyVerticalGrid, setBodyVerticalGrid, ITEM, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(column.headerVerticalGrid, setHeaderVerticalGrid, ITEM, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_GENERIC(column.footerVerticalGrid, setFooterVerticalGrid, ITEM, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(Column, column, (sl_uint32)iCol)
				}
			}

#define LAYOUT_CONTROL_SIMULATE_GRID_ROWS(SECTION, PREFIX) \
			{ \
				for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
					SAppLayoutGridColumn& column = columns[iCol]; \
					LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(PREFIX, column.SECTION##Attrs, -1, (sl_uint32)iCol) \
				} \
			} \
			{ \
				auto& section = attr->SECTION; \
				ListElements<SAppLayoutGridRow> rows(section.rows); \
				for (sl_size iRow = 0; iRow < rows.count; iRow++) { \
					SAppLayoutGridRow& row = rows[iRow]; \
					LAYOUT_CONTROL_SIMULATE_STRING(row.id, set##PREFIX##RowId, BASIC, (sl_uint32)iRow, value) \
					LAYOUT_CONTROL_SIMULATE_DIMENSION(row.height, set##PREFIX##RowHeight, ITEM, (sl_uint32)iRow, value) \
					LAYOUT_CONTROL_SIMULATE_GENERIC(row.visible, set##PREFIX##RowVisible, ITEM, (sl_uint32)iRow, value) \
					LAYOUT_CONTROL_SIMULATE_GENERIC(row.horizontalGrid, set##PREFIX##HorizontalGrid, ITEM, (sl_uint32)iRow, value) \
					LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(PREFIX, row, (sl_uint32)iRow, -1) \
					ListElements<SAppLayoutGridCell> cells(row.cells); \
					for (sl_size iCell = 0; iCell < cells.count; iCell++) { \
						SAppLayoutGridCell& cell = cells[iCell]; \
						LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(PREFIX, cell, (sl_uint32)iRow, (sl_uint32)iCell) \
						if (cell.colspan.flagDefined && cell.rowspan.flagDefined) { \
							if (op == SAppLayoutOperation::SimulateInit) { \
								view->set##PREFIX##Span((sl_uint32)iRow, (sl_uint32)iCell, cell.rowspan.value, cell.colspan.value, UIUpdateMode::Init); \
							} \
						} else { \
							LAYOUT_CONTROL_SIMULATE_GENERIC(cell.rowspan, set##PREFIX##Rowspan, ITEM, (sl_uint32)iRow, (sl_uint32)iCell, value) \
							LAYOUT_CONTROL_SIMULATE_GENERIC(cell.colspan, set##PREFIX##Colspan, ITEM, (sl_uint32)iRow, (sl_uint32)iCell, value) \
						} \
					} \
				} \
			}

			LAYOUT_CONTROL_SIMULATE_GRID_ROWS(body, Body)
			LAYOUT_CONTROL_SIMULATE_GRID_ROWS(header, Header)
			LAYOUT_CONTROL_SIMULATE_GRID_ROWS(footer, Footer)

			if (op == SAppLayoutOperation::SimulateInit) {
				if (!(attr->recordCount.flagDefined)) {
					view->setRecordCount(100, UIUpdateMode::Init);
				}
			}
		}

		LAYOUT_CONTROL_UI_ATTR(GENERIC, recordCount, setRecordCount)

		LAYOUT_CONTROL_ADD_STATEMENT

	}

	END_PROCESS_LAYOUT_CONTROL

}
