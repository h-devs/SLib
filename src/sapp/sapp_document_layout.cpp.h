/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

	enum LAYOUT_OP
	{
		OP_PARSE,
		OP_GENERATE_CPP,
		OP_SIMULATE
	};

	sl_bool SAppDocument::_parseLayoutStyle(const String& localNamespace, const Ref<XmlElement>& element)
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

		String name = element->getAttribute("name").trim();
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_layout_name_is_empty);
			return sl_false;
		}

		name = getNameInLocalNamespace(localNamespace, name);

		if (m_layoutStyles.find(name)) {
			logError(element, g_str_error_resource_layout_name_redefined, name);
			return sl_false;
		}
		style->name = name;

		if (!(m_layoutStyles.put(name, style))) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		String strInherit = element->getAttribute("inherit").trim();
		if (strInherit.isNotEmpty()) {
			ListElements<String> arr(strInherit.split(","));
			for (sl_size i = 0; i < arr.count; i++) {
				String s = arr[i].trim();
				Ref<SAppLayoutStyle> inheritStyle;
				getItemFromMap(m_layoutStyles, localNamespace, s, sl_null, &inheritStyle);
				if (inheritStyle.isNotNull()) {
					if (!(style->inherit.add_NoLock(Move(inheritStyle)))) {
						logError(element, g_str_error_out_of_memory);
						return sl_false;
					}
				} else {
					logError(element, g_str_error_layout_style_not_found, s);
					return sl_false;
				}
			}
		}

		return sl_true;
	}

	sl_bool SAppDocument::_parseLayoutInclude(const String& localNamespace, const Ref<XmlElement>& element)
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

		String name = element->getAttribute("name").trim();
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_layout_name_is_empty);
			return sl_false;
		}

		name = getNameInLocalNamespace(localNamespace, name);

		if (m_layoutIncludes.find(name)) {
			logError(element, g_str_error_resource_layout_name_redefined, name);
			return sl_false;
		}
		include->name = name;

		if (!(m_layoutIncludes.put(name, include))) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		return sl_true;
	}

	sl_bool SAppDocument::_parseLayoutUnit(const String& localNamespace, const Ref<XmlElement>& element)
	{
		if (element.isNull()) {
			return sl_false;
		}

		String name = element->getAttribute("name").trim();
		if (name.isEmpty()) {
			logError(element, g_str_error_resource_layout_name_is_empty);
			return sl_false;
		}

		name = getNameInLocalNamespace(localNamespace, name);

		if (m_layoutUnits.find(name)) {
			logError(element, g_str_error_resource_layout_name_redefined, name);
			return sl_false;
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

	sl_bool SAppDocument::_parseLayoutResource(const String& filePath, const String& localNamespace, const Ref<XmlElement>& element, const String16& source)
	{
		if (element.isNull()) {
			return sl_false;
		}

		Ref<SAppLayoutResource> layout = new SAppLayoutResource;
		if (layout.isNull()) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		layout->filePath = filePath;
		layout->element = element;

		if (m_layouts.find(localNamespace)) {
			logError(element, g_str_error_resource_layout_name_redefined, localNamespace);
			return sl_false;
		}
		layout->name = localNamespace;

		if (!(_parseLayoutResourceItem(layout.get(), layout.get(), sl_null, source))) {
			return sl_false;
		}

		if (!(m_layouts.put(localNamespace, layout))) {
			logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		return sl_true;
	}

	void SAppDocument::_openLayoutResource(SAppLayoutResource* layout, const String& name)
	{
		Ref<SAppLayoutResource> layoutImport;
		if (!(m_layouts.get(name, &layoutImport))) {
			_openUiResourceByName(name);
			m_layouts.emplace(name, sl_null);
		}
	}

	sl_bool SAppDocument::_parseLayoutResourceItem(SAppLayoutResource* layout, SAppLayoutResourceItem* item, SAppLayoutResourceItem* parent, const String16& source)
	{
		const Ref<XmlElement>& element = item->element;
		if (element.isNull()) {
			return sl_false;
		}
		if (!(_parseStyleAttribute(layout->name, item))) {
			return sl_false;
		}

		if (layout == item) {

			SAppLayoutType type;
			String strType = layout->getXmlAttribute("type");
			if (strType.isEmpty() || strType == "view") {
				type = SAppLayoutType::View;
			} else if (strType == "window") {
				type = SAppLayoutType::Window;
			} else if (strType == "page") {
				type = SAppLayoutType::Page;
			} else {
				logError(element, g_str_error_resource_layout_type_invalid, strType);
				return sl_false;
			}
			layout->layoutType = type;

			layout->baseClassName = layout->getXmlAttribute("base");

		} else {

			String strType = item->getXmlAttribute("type");
			if (strType.isEmpty()) {
				strType = element->getName();
			}
			SAppLayoutItemType type = SAppLayoutResource::getTypeFromName(strType);
			if (type == SAppLayoutItemType::Unknown) {
				logError(element, g_str_error_resource_layout_type_invalid, strType);
				return sl_false;
			}

			item->itemType = type;
			item->itemTypeName = strType;

		}

		if (parent) {
			String name = element->getAttribute("name");
			String arrayName;
			sl_int32 arrayIndex = -1;
			if (name.isNotEmpty()) {
				if (!(SAppUtil::checkNameOrArrayMember(name, &arrayName, &arrayIndex))) {
					logError(element, g_str_error_resource_layout_name_invalid, name);
					return sl_false;
				}
				if (layout->itemsByName.find(name)) {
					logError(element, g_str_error_resource_layout_name_redefined, name);
					return sl_false;
				}
				if (layout->radioGroups.find(name)) {
					logError(element, g_str_error_resource_layout_name_redefined, name);
					return sl_false;
				}
			} else {
				name = layout->getAutoIncreasingName(item->itemType);
				item->flagGeneratedName = sl_true;
			}
			item->name = name;
			item->arrayName = arrayName;
			item->arrayIndex = arrayIndex;
		}

		LayoutControlProcessParams pp;
		pp.op = OP_PARSE;
		pp.source = source;
		pp.resource = layout;
		pp.resourceItem = item;
		pp.parentResourceItem = parent;
		pp.name = item->name;
		if (!(_processLayoutResourceControl(&pp))) {
			return sl_false;
		}

		String customClassName = item->getXmlAttribute("class").trim();
		if (customClassName.isNotNull()) {
			item->className = customClassName;
		}

		if (customClassName.isNotEmpty()) {
			if (!(layout->customClasses.put(customClassName, sl_true))) {
				logError(element, g_str_error_out_of_memory);
				return sl_false;
			}
		}

		if (!parent) {
			String strSP = layout->getXmlAttribute("sp");
			if (!(layout->sp.parse(strSP, this))) {
				logError(element, g_str_error_resource_layout_attribute_invalid, "sp", strSP);
			}
			if (!(layout->sp.checkSP())) {
				logError(element, g_str_error_resource_layout_attribute_invalid, "sp", strSP);
				return sl_false;
			}
		}

		if (parent) {
			if (!(layout->itemsByName.put(item->name, item))) {
				logError(element, g_str_error_out_of_memory);
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

		return sl_true;
	}

	Ref<SAppLayoutResourceItem> SAppDocument::_parseLayoutResourceItemChild(SAppLayoutResource* layout, SAppLayoutResourceItem* parentItem, const Ref<XmlElement>& element, const String16& source)
	{
		Ref<SAppLayoutResourceItem> childItem = new SAppLayoutResourceItem;
		if (childItem.isNull()) {
			logError(element, g_str_error_out_of_memory);
			return sl_null;
		}

		childItem->element = element;

		if (!(_parseLayoutResourceItem(layout, childItem.get(), parentItem, source))) {
			return sl_null;
		}

		return childItem;
	}

	sl_bool SAppDocument::_generateLayoutsCpp(const String& targetPath)
	{
		log(g_str_log_generate_cpp_layouts_begin);

		if (!(File::isDirectory(targetPath + "/ui"))) {
			File::createDirectory(targetPath + "/ui");
			if (!(File::isDirectory(targetPath + "/ui"))) {
				log(g_str_error_directory_create_failed, targetPath + "/ui");
				return sl_false;
			}
		}

		StringBuffer sbHeader, sbHeaderBase, sbCpp;
		sbHeaderBase.add("#pragma once\r\n\r\n#include <slib/ui/resource.h>\r\n\r\n");
		sbHeader.add("#pragma once\r\n\r\n");

		{
			ListElements<String> includes(m_conf.generate_cpp_layout_include_headers);
			for (sl_size i = 0; i < includes.count; i++) {
				if (includes[i].isNotEmpty()) {
					sbHeaderBase.add(String::format("#include \"%s\"%n", includes[i]));
				}
			}
		}

		sbCpp.add(String::format(
								 "#include <slib/ui.h>%n%n"
								 "#include \"layouts.h\"%n%n"
								 "#include \"strings.h\"%n"
								 "#include \"colors.h\"%n"
								 "#include \"drawables.h\"%n"
								 "#include \"menus.h\"%n%n"
								 , m_conf.generate_cpp_namespace));

		{
			ListElements<String> includes(m_conf.generate_cpp_layout_include_headers_in_cpp);
			for (sl_size i = 0; i < includes.count; i++) {
				if (includes[i].isNotEmpty()) {
					sbCpp.add(String::format("#include \"%s\"%n", includes[i]));
				}
			}
		}

		sbHeaderBase.add(String::format("%n" "namespace %s%n" "{%n\tnamespace ui%n\t{%n", m_conf.generate_cpp_namespace));
		{
			for (auto& pair : m_layouts) {
				if (pair.value.isNotNull()) {
					Ref<SAppLayoutResource> layout = pair.value;
					sbHeaderBase.add(String::format("\t\tclass %s;%n", pair.key));
				}
			}
		}
		sbHeaderBase.add("\t}\r\n}\r\n");

		{
			for (auto& pair : m_layouts) {
				if (pair.value.isNotNull()) {
					sbHeader.add(String::format("#include \"ui/%s.h\"%n", pair.key));
					sbCpp.add(String::format("#include \"ui/%s.cpp.inc\"%n", pair.key));
					if (!(_generateLayoutsCpp_Layout(targetPath, pair.value.get()))) {
						return sl_false;
					}
				}
			}
		}

		String pathHeaderBase = targetPath + "/layouts_base.h";
		String contentHeaderBase = sbHeaderBase.merge();
		if (File::readAllTextUTF8(pathHeaderBase) != contentHeaderBase) {
			if (!(File::writeAllTextUTF8(pathHeaderBase, contentHeaderBase))) {
				logError(g_str_error_file_write_failed, pathHeaderBase);
				return sl_false;
			}
		}

		String pathHeader = targetPath + "/layouts.h";
		String contentHeader = sbHeader.merge();
		if (File::readAllTextUTF8(pathHeader) != contentHeader) {
			if (!(File::writeAllTextUTF8(pathHeader, contentHeader))) {
				logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}

		String pathCpp = targetPath + "/layouts.cpp";
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

		sbHeader.add("#pragma once\r\n\r\n#include \"../layouts_base.h\"\r\n\r\n");

		String namespacePrefix = String::format("namespace %s%n" "{%n\tnamespace ui%n\t{%n" , m_conf.generate_cpp_namespace);
		sbHeader.add(namespacePrefix);
		sbCpp.add(namespacePrefix);

		if (layout->baseClassName.isNotEmpty()) {
			sbHeader.add(String::format("\t\tSLIB_DECLARE_UILAYOUT_BEGIN(%s, %s)%n", name, layout->baseClassName));
			sbCpp.add(String::format("\t\tSLIB_DEFINE_UILAYOUT(%s, %s)%n%n", name, layout->baseClassName));
		} else {
			if (layout->layoutType == SAppLayoutType::Window) {
				sbHeader.add(String::format("\t\tSLIB_DECLARE_WINDOW_LAYOUT_BEGIN(%s)%n", name));
				sbCpp.add(String::format("\t\tSLIB_DEFINE_WINDOW_LAYOUT(%s)%n%n", name));
			} else if (layout->layoutType == SAppLayoutType::Page) {
				sbHeader.add(String::format("\t\tSLIB_DECLARE_PAGE_LAYOUT_BEGIN(%s)%n", name));
				sbCpp.add(String::format("\t\tSLIB_DEFINE_PAGE_LAYOUT(%s)%n%n", name));
			} else if (layout->layoutType == SAppLayoutType::View) {
				sbHeader.add(String::format("\t\tSLIB_DECLARE_VIEW_LAYOUT_BEGIN(%s)%n", name));
				sbCpp.add(String::format("\t\tSLIB_DEFINE_VIEW_LAYOUT(%s)%n%n", name));
			} else {
				return sl_false;
			}
		}

		sbCpp.add(String::format("\t\tvoid %s::initialize()%n\t\t{%n", name));

		{
			ListElements<String> radioGroups(layout->radioGroups.getAllKeys());
			for (sl_size i = 0; i < radioGroups.count; i++) {
				sbHeader.add(String::format("\t\t\tslib::Ref<slib::RadioGroup> %s;%n", radioGroups[i]));
				sbCpp.add(String::format("\t\t\t%s = new slib::RadioGroup;%n", radioGroups[i]));
			}
			if (radioGroups.count > 0) {
				sbHeader.add("\r\n");
				sbCpp.add("\r\n");
			}
		}
		{
			ObjectLocker lock(&(layout->itemArrays));
			for (auto& item : layout->itemArrays) {
				sbHeader.add(String::format("\t\t\tslib::Ref<%s> %s[%d];%n", item.value.className, item.key, item.value.itemCount));
			}
			if (layout->itemArrays.isNotEmpty()) {
				sbHeader.add("\r\n");
			}
		}

		StringBuffer sbLayout;

		if (layout->sp.flagDefined) {
			if (layout->sp.isNeededOnLayoutFunction()) {
				sbLayout.add(String::format("%n\t\t\tsetScaledPixel(%s);%n", layout->sp.getAccessString()));
			} else {
				sbCpp.add(String::format("%n\t\t\tsetScaledPixel(%s);%n%n", layout->sp.getAccessString()));
			}
		}

		if (!(_generateLayoutsCpp_Item(layout, layout, sl_null, sbHeader, sbCpp, sbLayout, sl_null))) {
			return sl_false;
		}

		sbCpp.add(String::format("\t\t}%n%n\t\tvoid %s::layoutViews(sl_ui_len CONTENT_WIDTH, sl_ui_len CONTENT_HEIGHT)%n\t\t{%n", name));
		sbCpp.link(sbLayout);
		static sl_char8 strEndCpp[] = "\t\t}\r\n\r\n";
		sbCpp.addStatic(strEndCpp, sizeof(strEndCpp)-1);

		if (layout->baseClassName.isNotEmpty()) {
			static sl_char8 strEndHeader[] = "\t\tSLIB_DECLARE_UILAYOUT_END\r\n\r\n";
			sbHeader.addStatic(strEndHeader, sizeof(strEndHeader)-1);
		} else {
			if (layout->layoutType == SAppLayoutType::Window) {
				static sl_char8 strEndHeader[] = "\t\tSLIB_DECLARE_WINDOW_LAYOUT_END\r\n\r\n";
				sbHeader.addStatic(strEndHeader, sizeof(strEndHeader)-1);
			} else if (layout->layoutType == SAppLayoutType::Page) {
				static sl_char8 strEndHeader[] = "\t\tSLIB_DECLARE_PAGE_LAYOUT_END\r\n\r\n";
				sbHeader.addStatic(strEndHeader, sizeof(strEndHeader)-1);
			} else if (layout->layoutType == SAppLayoutType::View) {
				static sl_char8 strEndHeader[] = "\t\tSLIB_DECLARE_VIEW_LAYOUT_END\r\n\r\n";
				sbHeader.addStatic(strEndHeader, sizeof(strEndHeader)-1);
			}
		}

		sbHeader.add("\t}\r\n}\r\n");
		sbCpp.add("\t}\r\n}\r\n");

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

	sl_bool SAppDocument::_generateLayoutsCpp_Item(SAppLayoutResource* layout, SAppLayoutResourceItem* item, SAppLayoutResourceItem* parent, StringBuffer& sbDeclare, StringBuffer& sbDefineInit, StringBuffer& sbDefineLayout, const String& addStatement)
	{
		String name;
		if (parent) {
			name = item->name;
			if (item->arrayIndex < 0) {
				sbDeclare.add(String::format("\t\t\tslib::Ref<%s> %s;%n", item->className, name));
			}
			sbDefineInit.add(String::format("\t\t\t%2$s = new %1$s;%n", item->className, name));
		} else {
			name = "this";
		}

		LayoutControlProcessParams pp;
		pp.op = OP_GENERATE_CPP;
		pp.resource = layout;
		pp.resourceItem = item;
		pp.parentResourceItem = parent;
		pp.addStatement = addStatement;
		pp.name = name;
		pp.sbDeclare = &sbDeclare;
		pp.sbDefineInit = &sbDefineInit;
		pp.sbDefineLayout = &sbDefineLayout;

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
			if (simulatorWidth.parse(layout->getXmlAttribute("simulatorWidth"), sl_null)) {
				if (simulatorWidth.flagDefined && simulatorWidth.checkForWindowSize()) {
					param.pageSize.x = _getDimensionValue(simulatorWidth);
					window->setSavingPageSize(sl_false);
				}
			}
			SAppDimensionValue simulatorHeight;
			if (simulatorHeight.parse(layout->getXmlAttribute("simulatorHeight"), sl_null)) {
				if (simulatorHeight.flagDefined && simulatorHeight.checkForWindowSize()) {
					param.pageSize.y = _getDimensionValue(simulatorHeight);
					window->setSavingPageSize(sl_false);
				}
			}
			if (layout->layoutType == SAppLayoutType::Window) {
				window->setOnClose(Function<void(Window*,UIEvent*)>::from(param.onCloseWindow));
			} else {
				window->setClientSize(param.pageSize);
				window->setOnClose(Function<void(Window*,UIEvent*)>::from(param.onClosePage));
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

	Ref<View> SAppDocument::_simulateLayoutCreateOrLayoutView(SAppLayoutSimulator* simulator, SAppLayoutResourceItem* item, SAppLayoutResourceItem* parent, View* parentView, sl_bool flagOnLayout)
	{
		Ref<SAppLayoutSimulationWindow> window = simulator->getSimulationWindow();
		if (window.isNull()) {
			return sl_null;
		}
		Ref<SAppLayoutResource> layout = simulator->getLayoutResource();
		if (layout.isNull()) {
			return sl_null;
		}

		Ref<View> view;
		if (parent) {
			if (flagOnLayout) {
				view = simulator->getViewByName(item->name);
				if (view.isNull()) {
					return sl_null;
				}
			}
		} else {
			view = simulator->getSimulationContentView();
			if (view.isNull()) {
				return sl_null;
			}
			if (layout->layoutType == SAppLayoutType::Page) {
				UISize size = window->getClientSize();
				m_layoutSimulationParams.screenWidth = size.x;
				m_layoutSimulationParams.screenHeight = size.y;
				m_layoutSimulationParams.viewportWidth = view->getWidth();
				m_layoutSimulationParams.viewportHeight = view->getHeight();
			} else {
				UISize size = UI::getScreenSize();
				m_layoutSimulationParams.screenWidth = size.x;
				m_layoutSimulationParams.screenHeight = size.y;
				size = window->getClientSize();
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
		pp.op = OP_SIMULATE;
		pp.resource = layout.get();
		pp.resourceItem = item;
		pp.parentResourceItem = parent;
		pp.simulator = simulator;
		pp.window = window.get();
		pp.view = view;
		pp.parentView = parentView;
		pp.name = item->name;
		pp.flagOnLayout = flagOnLayout;
		if (!(_processLayoutResourceControl(&pp))) {
			return sl_null;
		}

		if (parent) {
			if (!flagOnLayout) {
				view = pp.view;
				if (view.isNull()) {
					return sl_null;
				}
				simulator->registerViewByName(item->name, view);
			}
		}

		return view;

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

	sl_bool SAppDocument::_getFontAccessString(const String& localNamespace, const SAppFontValue& value, String& result)
	{
		String strSize;
		if (value.size.flagDefined) {
			strSize = value.size.getAccessString();
		} else {
			strSize = "slib::UI::getDefaultFontSize()";
		}
		String strFamily;
		if (value.family.flagDefined) {
			if (!(_getStringAccessString(localNamespace, value.family, strFamily))) {
				return sl_false;
			}
		} else {
			strFamily = "slib::UI::getDefaultFontFamily()";
		}
		result = String::format("slib::Font::create(%s, %s, %s, %s, %s)", strFamily, strSize, value.bold.value?"sl_true":"sl_false", value.italic.value?"sl_true":"sl_false", value.underline.value?"sl_true":"sl_false");
		return sl_true;
	}

	sl_bool SAppDocument::_getFontValue(const String& localNamespace, const SAppFontValue& value, Ref<Font>& result)
	{
		sl_real size;
		if (value.size.flagDefined) {
			size = _getDimensionValue(value.size);
		} else {
			size = UI::getDefaultFontSize();
		}
		String family;
		if (value.family.flagDefined) {
			if (!(_getStringValue(localNamespace, value.family, family))) {
				return sl_false;
			}
		} else {
			family = UI::getDefaultFontFamily();
		}
		result = Font::create(family, size, value.bold.value, value.italic.value, value.underline.value);
		return sl_true;
	}

	sl_bool SAppDocument::_getBorderAccessString(const String& localNamespace, const SAppBorderValue& value, String& result)
	{
		String strStyle;
		if (value.style.flagDefined) {
			strStyle = value.style.getAccessString();
		} else {
			strStyle = "slib::PenStyle::Default";
		}
		String strWidth;
		if (value.width.flagDefined) {
			strWidth = value.width.getAccessString();
		} else {
			strWidth = "-1.0f";
		}
		String strColor;
		if (value.color.flagDefined) {
			if (!(_getColorAccessString(localNamespace, value.color, strColor))) {
				return sl_false;
			}
		} else {
			strColor = "slib::Color::zero()";
		}
		result = String::format("slib::PenDesc(%s, %s, %s)", strStyle, strWidth, strColor);
		return sl_true;
	}

	sl_bool SAppDocument::_getBorderValue(const String& localNamespace, const SAppBorderValue& value, PenDesc& result)
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
			if (!(_getColorValue(localNamespace, value.color, result.color))) {
				return sl_false;
			}
		} else {
			result.color.setZero();
		}
		return sl_true;
	}

	sl_bool SAppDocument::_parseStyleAttribute(const String& localNamespace, SAppLayoutXmlItem* item)
	{
		if (item->element.isNull()) {
			return sl_true;
		}
		String strStyles = item->element->getAttribute("style").trim();
		if (strStyles.isNotEmpty()) {
			ListElements<String> arr(strStyles.split(","));
			for (sl_size i = 0; i < arr.count; i++) {
				String s = arr[i].trim();
				Ref<SAppLayoutStyle> style;
				getItemFromMap(m_layoutStyles, localNamespace, s, sl_null, &style);
				if (style.isNotNull()) {
					if (!(item->styles.add_NoLock(Move(style)))) {
						logError(item->element, g_str_error_out_of_memory);
						return sl_false;
					}
				} else {
					logError(item->element, g_str_error_layout_style_not_found, s);
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	List< Ref<XmlElement> > SAppDocument::_getXmlChildElements(const String& localNamespace, SAppLayoutXmlItem* item, const String& tagName)
	{
		List< Ref<XmlElement> > ret;
		if (!_addXmlChildElements(ret, item->element, localNamespace, tagName)) {
			return sl_null;
		}
		{
			ListElements< Ref<SAppLayoutStyle> > _styles(item->styles);
			for (sl_size i = 0; i < _styles.count; i++) {
				Ref<SAppLayoutStyle> style = _styles[i];
				if (style.isNotNull()) {
					if (!_addXmlChildElements(ret, style.get(), localNamespace, tagName)) {
						return sl_null;
					}
				}
			}
		}
		return ret;
	}

	sl_bool SAppDocument::_addXmlChildElements(List< Ref<XmlElement> >& list, SAppLayoutStyle* style, const String& localNamespace, const String& tagName)
	{
		{
			ListElements< Ref<SAppLayoutStyle> > _styles(style->inherit);
			for (sl_size i = 0; i < _styles.count; i++) {
				Ref<SAppLayoutStyle> other = _styles[i];
				if (other.isNotNull()) {
					if (!_addXmlChildElements(list, other.get(), localNamespace, tagName)) {
						return sl_false;
					}
				}
			}
		}
		return _addXmlChildElements(list, style->element, localNamespace, tagName);
	}

	sl_bool SAppDocument::_addXmlChildElements(List< Ref<XmlElement> >& list, const Ref<XmlElement>& parent, const String& localNamespace, const String& tagName)
	{
		{
			ListElements< Ref<XmlElement> > children(parent->getChildElements());
			for (sl_size i = 0; i < children.count; i++) {
				Ref<XmlElement>& child = children[i];
				if (child.isNotNull()) {
					String name = child->getName();
					if (name == "include") {
						String src = child->getAttribute("src");
						if (src.isEmpty()) {
							logError(child, g_str_error_resource_layout_attribute_invalid, "src", name);
							return sl_false;
						}
						Ref<SAppLayoutInclude> include;
						getItemFromMap(m_layoutIncludes, localNamespace, src, sl_null, &include);
						if (include.isNotNull()) {
							if (!_addXmlChildElements(list, include->element, localNamespace, tagName)) {
								return sl_false;
							}
						} else {
							logError(child, g_str_error_layout_include_not_found, name);
							return sl_false;
						}
					} else if (tagName.isEmpty() || name == tagName) {
						if (!(list.add_NoLock(Move(child)))) {
							logError(child, g_str_error_out_of_memory);
							return sl_false;
						}
					}
				}
			}
		}
		return sl_true;
	}

#define PROCESS_CONTROL_SWITCH(NAME) \
	case SAppLayoutItemType::NAME: \
		if (!(_processLayoutResourceControl_##NAME(params))) { \
			return sl_false; \
		} \
		break;

	sl_bool SAppDocument::_processLayoutResourceControl(LayoutControlProcessParams* params)
	{
		m_currentLocalNamespace = params->resource->name;
		SAppLayoutResourceItem* resourceItem = params->resourceItem;
		int op = params->op;
		switch (resourceItem->itemType)
		{
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
			PROCESS_CONTROL_SWITCH(List)
			PROCESS_CONTROL_SWITCH(Collection)
			PROCESS_CONTROL_SWITCH(Table)
			PROCESS_CONTROL_SWITCH(ListControl)
			PROCESS_CONTROL_SWITCH(Render)
			PROCESS_CONTROL_SWITCH(Tab)
			PROCESS_CONTROL_SWITCH(Tree)
			PROCESS_CONTROL_SWITCH(Split)
			PROCESS_CONTROL_SWITCH(Web)
			PROCESS_CONTROL_SWITCH(Progress)
			PROCESS_CONTROL_SWITCH(Slider)
			PROCESS_CONTROL_SWITCH(Switch)
			PROCESS_CONTROL_SWITCH(Picker)
			PROCESS_CONTROL_SWITCH(DatePicker)
			PROCESS_CONTROL_SWITCH(Pager)
			PROCESS_CONTROL_SWITCH(Navigation)
			PROCESS_CONTROL_SWITCH(Video)
			PROCESS_CONTROL_SWITCH(Camera)
			PROCESS_CONTROL_SWITCH(Drawer)
			PROCESS_CONTROL_SWITCH(Refresh)
			PROCESS_CONTROL_SWITCH(ListBox)
			PROCESS_CONTROL_SWITCH(LabelList)
			PROCESS_CONTROL_SWITCH(TileLayout)
			PROCESS_CONTROL_SWITCH(Pdf)
			PROCESS_CONTROL_SWITCH(GroupBox)
			PROCESS_CONTROL_SWITCH(Grid)
			PROCESS_CONTROL_SWITCH(XControl)
			PROCESS_CONTROL_SWITCH(XButton)
			PROCESS_CONTROL_SWITCH(XEdit)
			PROCESS_CONTROL_SWITCH(XPassword)
			default:
				return sl_false;
		}

		switch (resourceItem->itemType) {
			case SAppLayoutItemType::Scroll:
				break;
			default:
				if (op == OP_PARSE) {
					do {
						if (params->resourceItem->flagNoChildren) {
							break;
						}
						ListElements< Ref<XmlElement> > children(_getXmlChildElements(params->resource->name, resourceItem, sl_null));
						for (sl_size i = 0; i < children.count; i++) {
							const Ref<XmlElement>& child = children[i];
							String tagName = child->getName();
							if (tagName == "item" || tagName == "column" || tagName == "row" || tagName == "header" || tagName == "footer" || tagName == "body") {
								continue;
							}
							Ref<SAppLayoutResourceItem> childItem = _parseLayoutResourceItemChild(params->resource, resourceItem, child, params->source);
							if (childItem.isNull()) {
								return sl_false;
							}
							if (resourceItem->itemType == SAppLayoutItemType::Linear) {
								SAppLayoutLinearAttributes* attrs = (SAppLayoutLinearAttributes*)(resourceItem->attrs.get());
								if (!(attrs->orientation.flagDefined) || attrs->orientation.value == LayoutOrientation::Vertical) {
									childItem->attrs->topMode = PositionMode::Free;
									childItem->attrs->bottomMode = PositionMode::Free;
								} else {
									childItem->attrs->leftMode = PositionMode::Free;
									childItem->attrs->rightMode = PositionMode::Free;
								}
							} else if (resourceItem->itemType == SAppLayoutItemType::Refresh) {
								childItem->attrs->width.flagDefined = sl_true;
								childItem->attrs->width.amount = 1;
								childItem->attrs->width.unit = SAppDimensionValue::FILL;
								childItem->attrs->height.flagDefined = sl_true;
								childItem->attrs->height.amount = 1;
								childItem->attrs->height.unit = SAppDimensionValue::FILL;
							}
							if (!(resourceItem->children.add_NoLock(Move(childItem)))) {
								logError(resourceItem->element, g_str_error_out_of_memory);
								return sl_false;
							}
						}
					} while (0);
					return sl_true;
				} else if (op == OP_GENERATE_CPP) {
					String name;
					if (params->parentResourceItem) {
						name = params->name;
					} else {
						static sl_char8 strEnd[] = "\r\n";
						params->sbDefineInit->addStatic(strEnd, sizeof(strEnd)-1);
						name = "m_contentView";
					}
					ListElements< Ref<SAppLayoutResourceItem> > children(resourceItem->children);
					for (sl_size i = 0; i < children.count; i++) {
						Ref<SAppLayoutResourceItem>& child = children[i];
						if (child.isNotNull()) {
							String addStatement = String::format("\t\t\t%s->addChild(%s, slib::UIUpdateMode::Init);%n%n", name, child->name);
							if (!(_generateLayoutsCpp_Item(params->resource, child.get(), resourceItem, *(params->sbDeclare), *(params->sbDefineInit), *(params->sbDefineLayout), addStatement) )) {
								return sl_false;
							}
						}
					}
				} else if (op == OP_SIMULATE) {
					if (!(params->parentResourceItem) && params->resource->layoutType != SAppLayoutType::Window) {
						m_layoutSimulationParams.viewportWidth = params->view->getWidth();
						m_layoutSimulationParams.viewportHeight = params->view->getHeight();
					}
					ListElements< Ref<SAppLayoutResourceItem> > children(resourceItem->children);
					for (sl_size i = 0; i < children.count; i++) {
						Ref<SAppLayoutResourceItem>& child = children[i];
						if (child.isNotNull()) {
							Ref<View> childView = _simulateLayoutCreateOrLayoutView(params->simulator, child.get(), resourceItem, params->view.get(), params->flagOnLayout);
							if (childView.isNotNull()) {
								if (!(params->flagOnLayout)) {
									params->view->addChild(childView, UIUpdateMode::Init);
								}
							} else {
								return sl_false;
							}
						}
					}
				}
				break;
		}

		return sl_true;

	}

#define BEGIN_PROCESS_LAYOUT_CONTROL(NAME, VIEWTYPE) \
	sl_bool SAppDocument::_processLayoutResourceControl_##NAME(LayoutControlProcessParams* params) \
	{ \
		StringView strTab = StringView::literal("\t\t\t"); \
		SAppLayoutResource* resource = params->resource; \
		SLIB_UNUSED(resource) \
		SAppLayoutResourceItem* resourceItem = params->resourceItem; \
		const Ref<XmlElement>& element = resourceItem->element; \
		int op = params->op; \
		const String& name = params->name; \
		sl_bool flagOnLayout = params->flagOnLayout; \
		Ref<SAppLayout##NAME##Attributes>& attr = Ref<SAppLayout##NAME##Attributes>::from(resourceItem->attrs); \
		if (op == OP_PARSE) { \
			if (attr.isNull()) { \
				attr = new SAppLayout##NAME##Attributes; \
				if (attr.isNull()) { \
					logError(element, g_str_error_out_of_memory); \
					return sl_false; \
				} \
			} \
			if (resourceItem->className.isEmpty()) { \
				resourceItem->className = "slib::" #VIEWTYPE; \
			} \
		} else { \
			if (op == OP_SIMULATE) { \
				if (!flagOnLayout && params->view.isNull()) { \
					params->view = new VIEWTYPE; \
				} \
			} \
		} \
		VIEWTYPE* view = (VIEWTYPE*)(params->view.get()); \
		SLIB_UNUSED(view)


#define END_PROCESS_LAYOUT_CONTROL \
		return sl_true; \
	}

#define LAYOUT_CONTROL_DEFINE_XML(NAME, ...) \
	SAppLayoutXmlItem NAME(__VA_ARGS__); \
	if (!(_parseStyleAttribute(resource->name, &NAME))) { \
		return sl_false; \
	}

#define LAYOUT_CONTROL_GET_XML_CHILDREN(XML, TAG) _getXmlChildElements(resource->name, &XML, TAG)
#define LAYOUT_CONTROL_DEFINE_XML_CHILDREN(NAME, XML, TAG) \
	ListElements< Ref<XmlElement> > NAME(LAYOUT_CONTROL_GET_XML_CHILDREN(XML, TAG));

#define LAYOUT_CONTROL_GET_ITEM_CHILDREN(TAG) _getXmlChildElements(resource->name, resourceItem, TAG)
#define LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(NAME, TAG) \
	ListElements< Ref<XmlElement> > NAME(LAYOUT_CONTROL_GET_ITEM_CHILDREN(TAG));

#define LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME) \
	logError((XML).element, g_str_error_resource_layout_attribute_invalid, NAME, (XML).getXmlAttribute(NAME));
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
	String _strValue = (XML).getXmlAttribute(NAME); \
	if (!(VAR.parse(_strValue, ##__VA_ARGS__))) { \
		LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME) \
		return sl_false; \
	}
#define LAYOUT_CONTROL_PARSE(XML, NAME, VAR, ...) { PRIV_LAYOUT_CONTROL_PARSE(XML, NAME, VAR, ##__VA_ARGS__) }

#define PRIV_LAYOUT_CONTROL_GENERATE(BUF_TYPE, SETFUNC, ARG_FORMAT, ...) \
	params->sbDefine##BUF_TYPE->add(String::format("%s%s->" #SETFUNC "(" ARG_FORMAT ");%n", strTab, name, ##__VA_ARGS__));
#define LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT, ...) PRIV_LAYOUT_CONTROL_GENERATE(Init, SETFUNC, ARG_FORMAT, ##__VA_ARGS__)
#define LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, ARG_FORMAT, ...) PRIV_LAYOUT_CONTROL_GENERATE(Layout, SETFUNC, ARG_FORMAT, ##__VA_ARGS__)

#define LAYOUT_CONTROL_PARSE_GENERIC(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE(XML, NAME SUFFIX, VAR)
#define LAYOUT_CONTROL_GENERATE_GENERIC(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		String value = VAR.getAccessString(); \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_GENERIC(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && !flagOnLayout) { \
		auto& value = VAR.value; \
		view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
	}

#define LAYOUT_CONTROL_PARSE_BOOLEAN(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE(XML, NAME SUFFIX, VAR)
#define LAYOUT_CONTROL_GENERATE_BOOLEAN(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined && VAR.value) { \
		LAYOUT_CONTROL_GENERATE(SETFUNC, GEN_UPDATE1(CATEGORY, UI, Init)) \
	}
#define LAYOUT_CONTROL_SIMULATE_BOOLEAN(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && VAR.value && !flagOnLayout) { \
		view->SETFUNC(USE_UPDATE1(CATEGORY, UI, Init)); \
	}

	namespace {
		SLIB_INLINE static sl_bool Xor(sl_bool l, sl_bool r)
		{
			if (l) {
				return !r;
			} else {
				return r;
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
#define PRIV_LAYOUT_CONTROL_SIMULATE_DIMENSION(VAR, SETFUNC, CATEGORY, ...) \
	if (Xor(SAppDimensionValue::isAbsoluteUnit(VAR.unit), flagOnLayout)) { \
		auto value = _getDimensionValue(VAR); \
		if (flagOnLayout) { \
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
	if (VAR.flagDefined) { \
		String value; \
		if (!(_getStringAccessString(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_STRING(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && !flagOnLayout) { \
		String value; \
		if (!(_getStringValue(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
	}

#define LAYOUT_CONTROL_PARSE_DRAWABLE(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE(XML, NAME SUFFIX, VAR, this, (XML).element)
#define LAYOUT_CONTROL_GENERATE_DRAWABLE(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		String value; \
		if (!(_getDrawableAccessString(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_DRAWABLE(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && Xor(VAR.isAbsoluteUnit(), flagOnLayout)) { \
		Ref<Drawable> value; \
		if (!(_getDrawableValue(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		if (flagOnLayout) { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, ITEM, None)); \
		} else { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
		} \
	}

#define LAYOUT_CONTROL_PARSE_COLOR(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE_REFERING(XML, NAME, SUFFIX, VAR)
#define LAYOUT_CONTROL_GENERATE_COLOR(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		String value; \
		if (!(_getColorAccessString(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_COLOR(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && !flagOnLayout) { \
		Color value; \
		if (!(_getColorValue(resource->name, VAR, value))) { \
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
		if (!(_getFontAccessString(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		if (VAR.size.isNeededOnLayoutFunction()) { \
			LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, CONTROL, UpdateLayout) GEN_UPDATE2(CATEGORY, ITEM, None), ##__VA_ARGS__) \
		} else { \
			LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
		} \
	}
#define LAYOUT_CONTROL_SIMULATE_FONT(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && Xor(SAppDimensionValue::isAbsoluteUnit(VAR.size.unit), flagOnLayout)) { \
		Ref<Font> value; \
		if (!(_getFontValue(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		if (flagOnLayout) { \
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
		if (!(_getBorderAccessString(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		if (VAR.width.isNeededOnLayoutFunction()) { \
			LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, ITEM, None), ##__VA_ARGS__) \
		} else { \
			LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
		} \
	}
#define LAYOUT_CONTROL_SIMULATE_BORDER(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && Xor(SAppDimensionValue::isAbsoluteUnit(VAR.width.unit), flagOnLayout)) { \
		PenDesc value; \
		if (!(_getBorderValue(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		if (flagOnLayout) { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, ITEM, None)); \
		} else { \
			view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
		} \
	}

#define LAYOUT_CONTROL_PARSE_MENU(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE_REFERING(XML, NAME, SUFFIX, VAR)
#define LAYOUT_CONTROL_GENERATE_MENU(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		String value; \
		if (!(_getMenuAccessString(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_MENU(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.flagDefined && !flagOnLayout) { \
		Ref<Menu> value; \
		if (!(_getMenuValue(resource->name, VAR, value))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
	}
	
#define LAYOUT_CONTROL_PARSE_SIZE(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE_DIMENSION(XML, NAME, SUFFIX, VAR, checkSize)
#define LAYOUT_CONTROL_GENERATE_SIZE(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.unit == SAppDimensionValue::FILL) { \
		String value = String::format("%ff", VAR.amount); \
		LAYOUT_CONTROL_GENERATE(SETFUNC##Filling, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	} else if (VAR.unit == SAppDimensionValue::WRAP) { \
		StringView value = StringView::literal("sl_true"); \
		LAYOUT_CONTROL_GENERATE(SETFUNC##Wrapping, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	} else if (VAR.unit == SAppDimensionValue::WEIGHT) { \
		String value = String::format("%ff", VAR.amount); \
		LAYOUT_CONTROL_GENERATE(SETFUNC##Weight, ARG_FORMAT GEN_UPDATE2(CATEGORY, UI, Init), ##__VA_ARGS__) \
	} else { \
		PRIV_LAYOUT_CONTROL_GENERATE_DIMENSION(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ##__VA_ARGS__) \
	}
#define LAYOUT_CONTROL_SIMULATE_SIZE(VAR, SETFUNC, CATEGORY, ...) \
	if (VAR.unit == SAppDimensionValue::FILL) { \
		if (!flagOnLayout) { \
			auto value = VAR.amount; \
			view->SETFUNC##Filling(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
		} \
	} else if (VAR.unit == SAppDimensionValue::WRAP) { \
		if (!flagOnLayout) { \
			sl_bool value = sl_true; \
			view->SETFUNC##Wrapping(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
		} \
	} else if (VAR.unit == SAppDimensionValue::WEIGHT) { \
		if (!flagOnLayout) { \
			auto value = VAR.amount; \
			view->SETFUNC##Weight(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
		} \
	} else { \
		PRIV_LAYOUT_CONTROL_SIMULATE_DIMENSION(VAR, SETFUNC, CATEGORY, ##__VA_ARGS__) \
	}

#define LAYOUT_CONTROL_PARSE_MARGIN(XML, NAME, SUFFIX, VAR, ...) LAYOUT_CONTROL_PARSE_DIMENSION(XML, NAME, SUFFIX, VAR, checkMargin)
#define LAYOUT_CONTROL_GENERATE_MARGIN(VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	if (VAR.flagDefined) { \
		if (VAR.unit == SAppDimensionValue::WEIGHT) { \
			String value = String::format("%ff", VAR.amount); \
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
			if (!flagOnLayout) { \
				auto& value = VAR.amount; \
				view->SETFUNC##Weight(__VA_ARGS__ USE_UPDATE2(CATEGORY, UI, Init)); \
			} \
		} else { \
 			if (!(SAppDimensionValue::isSpecialUnit(VAR.unit))) { \
				PRIV_LAYOUT_CONTROL_SIMULATE_DIMENSION(VAR, SETFUNC, CATEGORY, ##__VA_ARGS__) \
			} \
		} \
	}

#define LAYOUT_CONTROL_PARSE_XML(TYPE, XML, ATTR, NAME, ...) LAYOUT_CONTROL_PARSE_##TYPE(XML, #NAME, , ATTR NAME, ##__VA_ARGS__)
#define LAYOUT_CONTROL_PARSE_ATTR(TYPE, ATTR, NAME, ...) LAYOUT_CONTROL_PARSE_XML(TYPE, *resourceItem, ATTR, NAME, ##__VA_ARGS__)

#define PRIV_LAYOUT_CONTROL_GENERATE_ATTR(TYPE, VAR, SETFUNC, CATEGORY) LAYOUT_CONTROL_GENERATE_##TYPE(VAR, SETFUNC, CATEGORY, "%s", value)
#define LAYOUT_CONTROL_GENERATE_ATTR(TYPE, VAR, SETFUNC) PRIV_LAYOUT_CONTROL_GENERATE_ATTR(TYPE, VAR, SETFUNC, BASIC)
#define LAYOUT_CONTROL_GENERATE_UI_ATTR(TYPE, VAR, SETFUNC) PRIV_LAYOUT_CONTROL_GENERATE_ATTR(TYPE, VAR, SETFUNC, CONTROL)

#define PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, VAR, SETFUNC, CATEGORY) LAYOUT_CONTROL_SIMULATE_##TYPE(VAR, SETFUNC, CATEGORY, value)
#define LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, VAR, SETFUNC) PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, VAR, SETFUNC, BASIC)
#define LAYOUT_CONTROL_SIMULATE_UI_ATTR(TYPE, VAR, SETFUNC) PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, VAR, SETFUNC, CONTROL)

#define PRIV_LAYOUT_CONTROL_ATTR(TYPE, NAME, SETFUNC, CATEGORY, ...) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_ATTR(TYPE, attr->, NAME, ##__VA_ARGS__) \
	} else if (op == OP_GENERATE_CPP) { \
		PRIV_LAYOUT_CONTROL_GENERATE_ATTR(TYPE, attr->NAME, SETFUNC, CATEGORY) \
	} else if (op == OP_SIMULATE) { \
		PRIV_LAYOUT_CONTROL_SIMULATE_ATTR(TYPE, attr->NAME, SETFUNC, CATEGORY) \
	}
#define LAYOUT_CONTROL_ATTR(TYPE, NAME, SETFUNC, ...) PRIV_LAYOUT_CONTROL_ATTR(TYPE, NAME, SETFUNC, BASIC, ##__VA_ARGS__)
#define LAYOUT_CONTROL_UI_ATTR(TYPE, NAME, SETFUNC, ...) PRIV_LAYOUT_CONTROL_ATTR(TYPE, NAME, SETFUNC, CONTROL, ##__VA_ARGS__)

#define LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, XML, NAME, SUFFIX, VAR, ...) \
	{ \
		for (sl_size i = 0; i < CountOfArray(g_stateDefines); i++) { \
			typename RemoveConstReference<typename decltype(VAR)>::Type::VALUE value; \
			LAYOUT_CONTROL_PARSE_##TYPE(XML, NAME, + StringView(g_stateDefines[i].suffix) SUFFIX, value, ##__VA_ARGS__) \
			if (value.flagDefined) { \
				VAR.values.put_NoLock(g_stateDefines[i].state, value); \
			} \
		} \
	}
#define LAYOUT_CONTROL_PARSE_STATE_MAP_XML(TYPE, XML, ATTR, NAME, ...) LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, XML, #NAME, , ATTR NAME, ##__VA_ARGS__)
#define LAYOUT_CONTROL_PARSE_STATE_MAP_ATTR(TYPE, ATTR, NAME, ...) LAYOUT_CONTROL_PARSE_STATE_MAP_XML(TYPE, *resourceItem, ATTR, NAME, ##__VA_ARGS__)
#define LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, VAR, SETFUNC, CATEGORY, ARG_FORMAT, ...) \
	{ \
		for (auto& item : VAR.values) { \
			const char* state = GetViewStateAcessString(item.key); \
			LAYOUT_CONTROL_GENERATE_##TYPE(item.value, SETFUNC, CATEGORY, ARG_FORMAT ", %s", ##__VA_ARGS__, state) \
		} \
	}
#define LAYOUT_CONTROL_SIMULATE_STATE_MAP(TYPE, VAR, SETFUNC, CATEGORY, ...) \
	{ \
		for (auto& item : VAR.values) { \
			LAYOUT_CONTROL_SIMULATE_##TYPE(item.value, SETFUNC, CATEGORY, ##__VA_ARGS__, item.key) \
		} \
	}

#define LAYOUT_CONTROL_STATE_MAP(TYPE, NAME, SETFUNC, ...) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_STATE_MAP_ATTR(TYPE, attr->, NAME, ##__VA_ARGS__) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, attr->NAME, SETFUNC, CONTROL, "%s", value) \
	} else if (op == OP_SIMULATE) { \
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
	if (op == OP_GENERATE_CPP) { \
		params->sbDefineInit->add(params->addStatement); \
	}

#define LAYOUT_CONTROL_SET_NATIVE_WIDGET \
	if (op == OP_GENERATE_CPP) { \
		if (attr->isNotRequiredNative(sl_false)) { \
			if (!(attr->nativeWidget.flagDefined)) { \
				LAYOUT_CONTROL_GENERATE(setCreatingNativeWidget, "sl_false") \
			} \
		} \
	} else if (op == OP_SIMULATE) { \
		if (!flagOnLayout) { \
			if (attr->isNotRequiredNative(sl_false)) { \
				if (!(attr->nativeWidget.flagDefined)) { \
					view->setCreatingNativeWidget(sl_false); \
				} \
			} \
		} \
	}

#define LAYOUT_CONTROL_SET_NATIVE_WIDGET_CHECK_BACKGROUND_COLOR \
	if (op == OP_GENERATE_CPP) { \
		if (attr->isNotRequiredNative(sl_true)) { \
			if (!(attr->nativeWidget.flagDefined)) { \
				LAYOUT_CONTROL_GENERATE(setCreatingNativeWidget, "sl_false") \
			} \
		} \
	} else if (op == OP_SIMULATE) { \
		if (!flagOnLayout) { \
			if (attr->isNotRequiredNative(sl_true)) { \
				if (!(attr->nativeWidget.flagDefined)) { \
					view->setCreatingNativeWidget(sl_false); \
				} \
			} \
		} \
	}

	namespace {
		struct SAppStateDefine
		{
			ViewState state;
			const char* suffix;
		};
		SAppStateDefine g_stateDefines[] = {
			{ViewState::Default, sl_null},
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
					return "slib::ViewState::Default";
			}
		}
	}

	BEGIN_PROCESS_LAYOUT_CONTROL(View, View)
	{
		sl_bool flagRoot = params->parentResourceItem == sl_null;
		sl_bool flagView = !flagRoot || params->resource->layoutType != SAppLayoutType::Window;

		if (flagView) {
			LAYOUT_CONTROL_ATTR(STRING, id, setId)
			LAYOUT_CONTROL_UI_ATTR(SIZE, width, setWidth)
			LAYOUT_CONTROL_UI_ATTR(SIZE, height, setHeight)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, left, setLeft, checkPosition)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, top, setTop, checkPosition)

			if (op == OP_PARSE) {

				attr->leftMode = PositionMode::Free;
				attr->topMode = PositionMode::Free;
				attr->rightMode = PositionMode::Free;
				attr->bottomMode = PositionMode::Free;

				SAppAlignLayoutValue alignLeft;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , alignLeft)
				if (alignLeft.flagDefined) {
					if (alignLeft.flagAlignParent) {
						attr->leftMode = PositionMode::ParentEdge;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(alignLeft.referingView)) {
							attr->leftMode = PositionMode::OtherStart;
							attr->leftReferingView = alignLeft.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR("alignLeft")
							return sl_false;
						}
					}
				}
				SAppNameValue toRightOf;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , toRightOf)
				if (toRightOf.flagDefined) {
					if (!flagRoot && params->resource->itemsByName.find(toRightOf.value)) {
						attr->leftMode = PositionMode::OtherEnd;
						attr->leftReferingView = toRightOf.value;
					} else {
						LOG_ERROR_LAYOUT_CONTROL_ATTR("toRightOf")
						return sl_false;
					}
				}
				SAppAlignLayoutValue alignTop;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , alignTop)
				if (alignTop.flagDefined) {
					if (alignTop.flagAlignParent) {
						attr->topMode = PositionMode::ParentEdge;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(alignTop.referingView)) {
							attr->topMode = PositionMode::OtherStart;
							attr->topReferingView = alignTop.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR("alignTop")
							return sl_false;
						}
					}
				}
				SAppNameValue below;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , below)
				if (below.flagDefined) {
					if (!flagRoot && params->resource->itemsByName.find(below.value)) {
						attr->topMode = PositionMode::OtherEnd;
						attr->topReferingView = below.value;
					} else {
						LOG_ERROR_LAYOUT_CONTROL_ATTR("below")
						return sl_false;
					}
				}
				SAppAlignLayoutValue alignRight;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , alignRight)
				if (alignRight.flagDefined) {
					if (alignRight.flagAlignParent) {
						attr->rightMode = PositionMode::ParentEdge;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(alignRight.referingView)) {
							attr->rightMode = PositionMode::OtherEnd;
							attr->rightReferingView = alignRight.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR("alignRight")
							return sl_false;
						}
					}
				}
				SAppNameValue toLeftOf;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , toLeftOf)
				if (toLeftOf.flagDefined) {
					if (!flagRoot && params->resource->itemsByName.find(toLeftOf.value)) {
						attr->rightMode = PositionMode::OtherStart;
						attr->rightReferingView = toLeftOf.value;
					} else {
						LOG_ERROR_LAYOUT_CONTROL_ATTR("toLeftOf")
						return sl_false;
					}
				}
				SAppAlignLayoutValue alignBottom;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , alignBottom)
				if (alignBottom.flagDefined) {
					if (alignBottom.flagAlignParent) {
						attr->bottomMode = PositionMode::ParentEdge;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(alignBottom.referingView)) {
							attr->bottomMode = PositionMode::OtherEnd;
							attr->bottomReferingView = alignBottom.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR("alignBottom")
							return sl_false;
						}
					}
				}
				SAppNameValue above;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , above)
				if (above.flagDefined) {
					if (!flagRoot && params->resource->itemsByName.find(above.value)) {
						attr->bottomMode = PositionMode::OtherStart;
						attr->bottomReferingView = above.value;
					} else {
						LOG_ERROR_LAYOUT_CONTROL_ATTR("above")
						return sl_false;
					}
				}
				SAppAlignLayoutValue centerHorizontal;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , centerHorizontal)
				if (centerHorizontal.flagDefined) {
					if (centerHorizontal.flagAlignParent) {
						attr->leftMode = PositionMode::CenterInParent;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(centerHorizontal.referingView)) {
							attr->leftMode = PositionMode::CenterInOther;
							attr->leftReferingView = centerHorizontal.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR("centerHorizontal")
							return sl_false;
						}
					}
				}
				SAppAlignLayoutValue centerVertical;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , centerVertical)
				if (centerVertical.flagDefined) {
					if (centerVertical.flagAlignParent) {
						attr->topMode = PositionMode::CenterInParent;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(centerVertical.referingView)) {
							attr->topMode = PositionMode::CenterInOther;
							attr->topReferingView = centerVertical.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR("centerVertical")
							return sl_false;
						}
					}
				}
				SAppAlignLayoutValue alignCenter;
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , alignCenter)
				if (alignCenter.flagDefined) {
					if (alignCenter.flagAlignParent) {
						attr->leftMode = PositionMode::CenterInParent;
						attr->topMode = PositionMode::CenterInParent;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(alignCenter.referingView)) {
							attr->leftMode = PositionMode::CenterInOther;
							attr->topMode = PositionMode::CenterInOther;
							attr->leftReferingView = alignCenter.referingView;
							attr->topReferingView = alignCenter.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR("alignCenter")
							return sl_false;
						}
					}
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

			} else if (op == OP_GENERATE_CPP) {
				if (attr->leftMode == PositionMode::CenterInParent) {
					LAYOUT_CONTROL_GENERATE(setCenterHorizontal, "slib::UIUpdateMode::Init")
				} else if (attr->leftMode == PositionMode::CenterInOther) {
					LAYOUT_CONTROL_GENERATE(setAlignCenterHorizontal, "%s, slib::UIUpdateMode::Init", attr->leftReferingView)
				} else {
					if (attr->leftMode == PositionMode::ParentEdge) {
						LAYOUT_CONTROL_GENERATE(setAlignParentLeft, "slib::UIUpdateMode::Init")
					} else if (attr->leftMode == PositionMode::OtherStart) {
						LAYOUT_CONTROL_GENERATE(setAlignLeft, "%s, slib::UIUpdateMode::Init", attr->leftReferingView)
					} else if (attr->leftMode == PositionMode::OtherEnd) {
						LAYOUT_CONTROL_GENERATE(setRightOf, "%s, slib::UIUpdateMode::Init", attr->leftReferingView)
					}
					if (attr->rightMode == PositionMode::ParentEdge) {
						LAYOUT_CONTROL_GENERATE(setAlignParentRight, "slib::UIUpdateMode::Init")
					} else if (attr->rightMode == PositionMode::OtherStart) {
						LAYOUT_CONTROL_GENERATE(setLeftOf, "%s, slib::UIUpdateMode::Init", attr->rightReferingView)
					} else if (attr->rightMode == PositionMode::OtherEnd) {
						LAYOUT_CONTROL_GENERATE(setAlignRight, "%s, slib::UIUpdateMode::Init", attr->rightReferingView)
					}
				}
				if (attr->topMode == PositionMode::CenterInParent) {
					LAYOUT_CONTROL_GENERATE(setCenterVertical, "slib::UIUpdateMode::Init")
				} else if (attr->topMode == PositionMode::CenterInOther) {
					LAYOUT_CONTROL_GENERATE(setAlignCenterVertical, "%s, slib::UIUpdateMode::Init", attr->topReferingView)
				} else {
					if (attr->topMode == PositionMode::ParentEdge) {
						LAYOUT_CONTROL_GENERATE(setAlignParentTop, "slib::UIUpdateMode::Init")
					} else if (attr->topMode == PositionMode::OtherStart) {
						LAYOUT_CONTROL_GENERATE(setAlignTop, "%s, slib::UIUpdateMode::Init", attr->topReferingView)
					} else if (attr->topMode == PositionMode::OtherEnd) {
						LAYOUT_CONTROL_GENERATE(setBelow, "%s, slib::UIUpdateMode::Init", attr->topReferingView)
					}
					if (attr->bottomMode == PositionMode::ParentEdge) {
						LAYOUT_CONTROL_GENERATE(setAlignParentBottom, "slib::UIUpdateMode::Init")
					} else if (attr->bottomMode == PositionMode::OtherStart) {
						LAYOUT_CONTROL_GENERATE(setAbove, "%s, slib::UIUpdateMode::Init", attr->bottomReferingView)
					} else if (attr->bottomMode == PositionMode::OtherEnd) {
						LAYOUT_CONTROL_GENERATE(setAlignBottom, "%s, slib::UIUpdateMode::Init", attr->bottomReferingView)
					}
				}
			} else if (op == OP_SIMULATE) {
				if (!flagOnLayout) {
					if (attr->leftMode == PositionMode::CenterInParent) {
						view->setCenterHorizontal(UIUpdateMode::Init);
					} else if (attr->leftMode == PositionMode::CenterInOther) {
						view->setAlignCenterHorizontal(params->simulator->getViewByName(attr->leftReferingView), UIUpdateMode::Init);
					} else {
						if (attr->leftMode == PositionMode::ParentEdge) {
							view->setAlignParentLeft(UIUpdateMode::Init);
						} else if (attr->leftMode == PositionMode::OtherStart) {
							view->setAlignLeft(params->simulator->getViewByName(attr->leftReferingView), UIUpdateMode::Init);
						} else if (attr->leftMode == PositionMode::OtherEnd) {
							view->setRightOf(params->simulator->getViewByName(attr->leftReferingView), UIUpdateMode::Init);
						}
						if (attr->rightMode == PositionMode::ParentEdge) {
							view->setAlignParentRight(UIUpdateMode::Init);
						} else if (attr->rightMode == PositionMode::OtherStart) {
							view->setLeftOf(params->simulator->getViewByName(attr->rightReferingView), UIUpdateMode::Init);
						} else if (attr->rightMode == PositionMode::OtherEnd) {
							view->setAlignRight(params->simulator->getViewByName(attr->rightReferingView), UIUpdateMode::Init);
						}
					}
					if (attr->topMode == PositionMode::CenterInParent) {
						view->setCenterVertical(UIUpdateMode::Init);
					} else if (attr->topMode == PositionMode::CenterInOther) {
						view->setAlignCenterVertical(params->simulator->getViewByName(attr->topReferingView), UIUpdateMode::Init);
					} else {
						if (attr->topMode == PositionMode::ParentEdge) {
							view->setAlignParentTop(UIUpdateMode::Init);
						} else if (attr->topMode == PositionMode::OtherStart) {
							view->setAlignTop(params->simulator->getViewByName(attr->topReferingView), UIUpdateMode::Init);
						} else if (attr->topMode == PositionMode::OtherEnd) {
							view->setBelow(params->simulator->getViewByName(attr->topReferingView), UIUpdateMode::Init);
						}
						if (attr->bottomMode == PositionMode::ParentEdge) {
							view->setAlignParentBottom(UIUpdateMode::Init);
						} else if (attr->bottomMode == PositionMode::OtherStart) {
							view->setAbove(params->simulator->getViewByName(attr->bottomReferingView), UIUpdateMode::Init);
						} else if (attr->bottomMode == PositionMode::OtherEnd) {
							view->setAlignBottom(params->simulator->getViewByName(attr->bottomReferingView), UIUpdateMode::Init);
						}
					}
				}
			}

			LAYOUT_CONTROL_UI_ATTR(DIMENSION, minWidth, setMinimumWidth, checkScalarSize)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, maxWidth, setMaximumWidth, checkScalarSize)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, minHeight, setMinimumHeight, checkScalarSize)
			LAYOUT_CONTROL_UI_ATTR(DIMENSION, maxHeight, setMaximumHeight, checkScalarSize)

			LAYOUT_CONTROL_UI_ATTR(GENERIC, aspectRatio, setAspectRatio)

			if (op == OP_PARSE) {
				if (!flagRoot || resource->layoutType == SAppLayoutType::View) {
					if (!(attr->width.flagDefined) && attr->leftMode != PositionMode::Free && attr->rightMode != PositionMode::Free) {
						attr->width.flagDefined = sl_true;
						attr->width.amount = 1;
						attr->width.unit = SAppDimensionValue::FILL;
					}
					if (!(attr->height.flagDefined) && attr->topMode != PositionMode::Free && attr->bottomMode != PositionMode::Free) {
						attr->height.flagDefined = sl_true;
						attr->height.amount = 1;
						attr->height.unit = SAppDimensionValue::FILL;
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
			if (op == OP_GENERATE_CPP) {
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
			} else if (op == OP_SIMULATE) {
				if (attr->aspectRatio.flagDefined) {
					if (!flagOnLayout) {
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
			}

			LAYOUT_CONTROL_UI_ATTR(MARGIN, marginLeft, setMarginLeft)
			LAYOUT_CONTROL_UI_ATTR(MARGIN, marginTop, setMarginTop)
			LAYOUT_CONTROL_UI_ATTR(MARGIN, marginRight, setMarginRight)
			LAYOUT_CONTROL_UI_ATTR(MARGIN, marginBottom, setMarginBottom)
			if (op == OP_PARSE) {
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
		if (op == OP_PARSE) {
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
			if (op == OP_PARSE) {
				LAYOUT_CONTROL_PARSE_ATTR(GENERIC, , border)
			} else if (op == OP_GENERATE_CPP) {
				LAYOUT_CONTROL_GENERATE_UI_ATTR(GENERIC, border, setBorder)
			} else if (op == OP_SIMULATE) {
				LAYOUT_CONTROL_SIMULATE_UI_ATTR(GENERIC, border, setBorder)
			}
			LAYOUT_CONTROL_STATE_MAP(BORDER, border, setBorder)
			if (op == OP_PARSE) {
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

		LAYOUT_CONTROL_UI_ATTR(FONT, font, setFont)
		if (op == OP_PARSE) {
			if (params->parentResourceItem) {
				attr->font.inheritFrom(params->parentResourceItem->attrs->font);
			}
		}

		if (flagView) {
			LAYOUT_CONTROL_UI_ATTR(GENERIC, alpha, setAlpha)
			LAYOUT_CONTROL_UI_ATTR(GENERIC, antiAlias, setAntiAlias)
		}

		LAYOUT_CONTROL_UI_ATTR(GENERIC, opaque, setOpaque)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, layer, setLayer)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, shadowOpacity, setShadowOpacity)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, shadowRadius, setShadowRadius, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, shadowOffsetX, setShadowOffsetX, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, shadowOffsetY, setShadowOffsetY, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(COLOR, shadowColor, setShadowColor)

		if (op == OP_PARSE) {
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
		} else if (op == OP_GENERATE_CPP) {
			if (attr->scrolling.flagDefined) {
				LAYOUT_CONTROL_GENERATE(setHorizontalScrolling, "%s, slib::UIUpdateMode::Init", attr->scrolling.horizontal?"sl_true":"sl_false")
				LAYOUT_CONTROL_GENERATE(setVerticalScrolling, "%s, slib::UIUpdateMode::Init", attr->scrolling.vertical?"sl_true":"sl_false")
			}
		} else if (op == OP_SIMULATE) {
			if (!flagOnLayout && attr->scrolling.flagDefined) {
				view->setHorizontalScrolling(attr->scrolling.horizontal?sl_true:sl_false, UIUpdateMode::Init);
				view->setVerticalScrolling(attr->scrolling.vertical?sl_true:sl_false, UIUpdateMode::Init);
			}
		}

		LAYOUT_CONTROL_ATTR(GENERIC, paging, setPaging)
		LAYOUT_CONTROL_ATTR(DIMENSION, pageWidth, setPageWidth, checkScalarSize)
		LAYOUT_CONTROL_ATTR(DIMENSION, pageHeight, setPageHeight, checkScalarSize)

		if (op == OP_PARSE) {
			LAYOUT_CONTROL_PARSE_ATTR(GENERIC, attr->, scrollBars)
		} else if (op == OP_GENERATE_CPP) {
			if (attr->scrollBars.flagDefined) {
				LAYOUT_CONTROL_GENERATE(setScrollBarsVisible, "%s, %s, slib::UIUpdateMode::Init", attr->scrollBars.horizontalScrollBar ? "sl_true" : "sl_false", attr->scrollBars.verticalScrollBar ? "sl_true" : "sl_false")
			}
		} else if (op == OP_SIMULATE) {
			if (!flagOnLayout && attr->scrollBars.flagDefined) {
				view->setScrollBarsVisible(attr->scrollBars.horizontalScrollBar, attr->scrollBars.verticalScrollBar, UIUpdateMode::Init);
			}
		}
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, hscrollThumb, setHorizontalScrollThumb)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, hscrollTrack, setHorizontalScrollThumb)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, vscrollThumb, setVerticalScrollThumb)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, vscrollTrack, setVerticalScrollThumb)
		if (op == OP_PARSE) {
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

		LAYOUT_CONTROL_ATTR(GENERIC, focusable, setFocusable)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, focus, setFocus)
		LAYOUT_CONTROL_ATTR(GENERIC, hitTest, setHitTestable)
		LAYOUT_CONTROL_ATTR(GENERIC, touchMultipleChildren, setTouchMultipleChildren)
		if (flagView) {
			LAYOUT_CONTROL_ATTR(GENERIC, tabStop, setTabStopEnabled)
			LAYOUT_CONTROL_ATTR(GENERIC, cursor, setCursor)
			LAYOUT_CONTROL_ATTR(STRING, toolTip, setToolTip)
		}
		if (!flagRoot) {
			if (op == OP_PARSE) {
				attr->nextTabStop = resourceItem->getXmlAttribute("nextTabStop");
				attr->previousTabStop = resourceItem->getXmlAttribute("previousTabStop");
			} else if (op == OP_GENERATE_CPP) {
				if (params->resource->itemsByName.find(attr->nextTabStop)) {
					LAYOUT_CONTROL_GENERATE(setNextTabStop, "%s", attr->nextTabStop)
				}
				if (params->resource->itemsByName.find(attr->previousTabStop)) {
					LAYOUT_CONTROL_GENERATE(setPreviousTabStop, "%s", attr->previousTabStop)
				}
			} else if (op == OP_SIMULATE) {
				if (!flagOnLayout) {
					view->setNextTabStop(params->simulator->getViewByName(attr->nextTabStop));
					view->setPreviousTabStop(params->simulator->getViewByName(attr->previousTabStop));
				}
			}
		}

		if (flagView) {
			LAYOUT_CONTROL_UI_ATTR(GENERIC, visibility, setVisibility)
			LAYOUT_CONTROL_UI_ATTR(GENERIC, visible, setVisible)
			LAYOUT_CONTROL_UI_ATTR(GENERIC, enabled, setEnabled)
			LAYOUT_CONTROL_UI_ATTR(GENERIC, clipping, setClipping)
			if (op == OP_SIMULATE) {
				if (attr->clipping.flagDefined) {
					if (attr->boundShape.flagDefined || attr->boundRadius.flagDefined || attr->boundRadiusX.flagDefined || attr->boundRadiusY.flagDefined) {
						if (flagOnLayout) {
							view->setClipping(attr->clipping.value, UIUpdateMode::None);
						}
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

		if (flagView) {
			LAYOUT_CONTROL_ATTR(GENERIC, okCancelEnabled, setOkCancelEnabled)
			LAYOUT_CONTROL_ATTR(BOOLEAN, ok, setOkOnClick)
			LAYOUT_CONTROL_ATTR(BOOLEAN, cancel, setCancelOnClick)
			LAYOUT_CONTROL_ATTR(GENERIC, mnemonicKey, setMnemonicKey)
			LAYOUT_CONTROL_ATTR(GENERIC, keepKeyboard, setKeepKeyboard)
			LAYOUT_CONTROL_ATTR(GENERIC, playSoundOnClick, setPlaySoundOnClick)
			LAYOUT_CONTROL_ATTR(GENERIC, clientEdge, setClientEdge)
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Window, View)
	{
		Window* view = params->window;

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
		LAYOUT_CONTROL_ATTR(GENERIC, transparent, setTransparent)
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
		if (op == OP_PARSE) {
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

		LAYOUT_CONTROL_ATTR(MENU, menu, setMenu)
		LAYOUT_CONTROL_ATTR(STRING, title, setTitle)
		LAYOUT_CONTROL_ATTR(DIMENSION, left, setLeft, checkForWindow)
		LAYOUT_CONTROL_ATTR(DIMENSION, top, setTop, checkForWindow)
		if (op == OP_PARSE) {
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, attr->, width, checkForWindowSize)
			LAYOUT_CONTROL_PARSE_ATTR(DIMENSION, attr->, height, checkForWindowSize)
		} else if (op == OP_GENERATE_CPP) {
			if (attr->width.flagDefined) {
				if (attr->width.unit == SAppDimensionValue::WRAP) {
					LAYOUT_CONTROL_GENERATE(setWidthWrapping, "sl_true, slib::UIUpdateMode::Init")
				} else if (attr->width.unit == SAppDimensionValue::FILL) {
					LAYOUT_CONTROL_GENERATE(setWidthFilling, "sl_true, slib::UIUpdateMode::Init")
				} else {
					LAYOUT_CONTROL_GENERATE_ATTR(DIMENSION, attr->width, setClientWidth)
				}
			}
			if (attr->height.flagDefined) {
				if (attr->height.unit == SAppDimensionValue::WRAP) {
					LAYOUT_CONTROL_GENERATE(setHeightWrapping, "sl_true, slib::UIUpdateMode::Init")
				} else if (attr->height.unit == SAppDimensionValue::FILL) {
					LAYOUT_CONTROL_GENERATE(setHeightFilling, "sl_true, slib::UIUpdateMode::Init")
				} else {
					LAYOUT_CONTROL_GENERATE_ATTR(DIMENSION, attr->height, setClientHeight)
				}
			}
		} else if (op == OP_SIMULATE) {
			if (!flagOnLayout) {
				flagOnLayout = sl_true;
				if (attr->width.flagDefined) {
					if (attr->width.unit == SAppDimensionValue::WRAP) {
						view->setWidthWrapping(sl_true, UIUpdateMode::Init);
					} else if (attr->width.unit == SAppDimensionValue::FILL) {
						view->setWidthFilling(sl_true, UIUpdateMode::Init);
					} else {
						LAYOUT_CONTROL_SIMULATE_ATTR(DIMENSION, attr->width, setClientWidth)
					}
				}
				if (attr->height.flagDefined) {
					if (attr->height.unit == SAppDimensionValue::WRAP) {
						view->setHeightWrapping(sl_true, UIUpdateMode::Init);
					} else if (attr->height.unit == SAppDimensionValue::FILL) {
						view->setHeightFilling(sl_true, UIUpdateMode::Init);
					} else {
						LAYOUT_CONTROL_SIMULATE_ATTR(DIMENSION, attr->height, setClientHeight)
					}
				}
				flagOnLayout = sl_false;
			}
		}

		params->name = "m_contentView";
		if (!(_processLayoutResourceControl_View(params))) {
			attr->resetLayout();
			return sl_false;
		}

		if (op == OP_PARSE) {
			if (!(attr->backgroundColor.flagDefined)) {
				SAppDrawableValue background;
				if (attr->background.values.get_NoLock(ViewState::Default, &background)) {
					if (background.flagDefined && background.flagColor) {
						attr->backgroundColor.flagDefined = sl_true;
						attr->backgroundColor.color = background.color;
						attr->backgroundColor.resourceName = background.resourceName;
						attr->background.values.remove_NoLock(ViewState::Default);
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

		if (op == OP_PARSE) {
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
		if (op == OP_PARSE) {
			attr->layout = resourceItem->getXmlAttribute("layout");
			if (attr->layout.isEmpty()) {
				LOG_ERROR_LAYOUT_CONTROL_ATTR("layout")
				return sl_false;
			}
			resourceItem->className = attr->layout;
		} else {
			_openLayoutResource(resource, attr->layout);
		}
		if (op == OP_GENERATE_CPP) {
			Ref<SAppLayoutResource> layoutImport;
			m_layouts.get(attr->layout, &layoutImport);
			if (layoutImport.isNull()) {
				logError(element, g_str_error_layout_not_found, attr->layout);
				return sl_false;
			}
			if (layoutImport->layoutType != SAppLayoutType::View && layoutImport->layoutType != SAppLayoutType::Page) {
				logError(element, g_str_error_layout_is_not_view, attr->layout);
				return sl_false;
			}
		} else if (op == OP_SIMULATE) {
			Ref<SAppLayoutResource> layoutImport;
			m_layouts.get(attr->layout, &layoutImport);
			if (layoutImport.isNull()) {
				logError(element, g_str_error_layout_not_found, attr->layout);
				return sl_false;
			}
			if (layoutImport->layoutType != SAppLayoutType::View && layoutImport->layoutType != SAppLayoutType::Page) {
				logError(element, g_str_error_layout_is_not_view, attr->layout);
				return sl_false;
			}
			if (!flagOnLayout) {
				Ref<SAppLayoutImportView> _view = new SAppLayoutImportView;
				if (_view.isNotNull()) {
					_view->initialize(params->simulator, layoutImport.get());
				} else {
					return sl_false;
				}
				params->view = _view;
				view = _view.get();
			} else {
				if (!view) {
					return sl_false;
				}
			}
		}

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == OP_SIMULATE) {
			if (flagOnLayout) {
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
		if (op == OP_PARSE) {
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
		if (op == OP_PARSE) {
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
		if (op == OP_PARSE) {
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

		LAYOUT_CONTROL_STATE_MAP(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, icon, setIcon)
		LAYOUT_CONTROL_STATE_MAP(COLOR, colorOverlay, setColorOverlay)

#define LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(TYPE, NAME, SETFUNC, ...) \
		if (op == OP_PARSE) { \
			LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, *(resourceItem), #NAME, +suffix, category.NAME, ##__VA_ARGS__) \
		} else if (op == OP_GENERATE_CPP) { \
			LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, category.NAME, SETFUNC, CONTROL, "%d, %s", i, value) \
		} else if (op == OP_SIMULATE) { \
			LAYOUT_CONTROL_SIMULATE_STATE_MAP(TYPE, category.NAME, SETFUNC, CONTROL, (sl_uint32)i, value) \
		}

		for (sl_size i = 0; i < CountOfArray(attr->categories); i++) {
			SAppLayoutButtonCategory& category = attr->categories[i];
			String suffix = String::fromSize(i);
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(COLOR, textColor, setTextColor)
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(DRAWABLE, icon, setIcon)
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(DRAWABLE, background, setBackground)
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(BORDER, border, setBorder)
			LAYOUT_CONTRL_BUTTON_CATEGORY_ATTR(COLOR, colorOverlay, setColorOverlay)
		}

		LAYOUT_CONTROL_SET_NATIVE_WIDGET_CHECK_BACKGROUND_COLOR

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Label, LabelView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(STRING, text, setText)
		LAYOUT_CONTROL_UI_ATTR(STRING, hyperText, setHyperText)
		LAYOUT_CONTROL_UI_ATTR(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, multiLine, setMultiLine)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, ellipsize, setEllipsize)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, lines, setLineCount)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, linksInText, setDetectingHyperlinksInPlainText);
		LAYOUT_CONTROL_UI_ATTR(COLOR, linkColor, setLinkColor)
		LAYOUT_CONTROL_ATTR(GENERIC, mnemonic, setMnemonic)

		if (op == OP_PARSE) {
			if (!(attr->text.flagDefined) && !(attr->hyperText.flagDefined)) {
				resourceItem->flagNoChildren = sl_true;
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
			if (op == OP_PARSE) {
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

		if (op == OP_PARSE) {
			attr->group = resourceItem->getXmlAttribute("group");
			if (attr->group.isNotEmpty()) {
				if (!(SAppUtil::checkName(attr->group.getData(), attr->group.getLength()))) {
					logError(element, g_str_error_resource_layout_name_invalid, attr->group);
					return sl_false;
				}
				if (params->resource->itemsByName.find(attr->group)) {
					logError(element, g_str_error_resource_layout_name_redefined, attr->group);
					return sl_false;
				}
				params->resource->radioGroups.put(attr->group, sl_true);
			}
		} else if (op == OP_GENERATE_CPP) {
			if (attr->group.isNotEmpty()) {
				params->sbDefineInit->add(String::format("%s%s->add(%s);%n", strTab, attr->group, name));
			}
		} else if (op == OP_SIMULATE) {
			if (!flagOnLayout) {
				if (attr->group.isNotEmpty()) {
					Ref<RadioGroup> group = params->simulator->getRadioGroup(attr->group);
					if (group.isNotNull()) {
						group->add(view);
					}
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

#define PROCESS_EDIT_ATTRS \
		LAYOUT_CONTROL_UI_ATTR(STRING, text, setText) \
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity) \
		LAYOUT_CONTROL_UI_ATTR(COLOR, textColor, setTextColor) \
		LAYOUT_CONTROL_UI_ATTR(STRING, hintText, setHintText) \
		LAYOUT_CONTROL_UI_ATTR(GENERIC, hintGravity, setHintGravity) \
		LAYOUT_CONTROL_UI_ATTR(COLOR, hintTextColor, setHintTextColor) \
		LAYOUT_CONTROL_UI_ATTR(FONT, hintFont, setHintFont) \
		if (op == OP_PARSE) { \
			if (attr->hintFont.flagDefined) { \
				attr->hintFont.inheritFrom(attr->font); \
			} \
		} \
		LAYOUT_CONTROL_UI_ATTR(GENERIC, readOnly, setReadOnly) \
		LAYOUT_CONTROL_UI_ATTR(GENERIC, password, setPassword) \
		LAYOUT_CONTROL_UI_ATTR(GENERIC, number, setNumber) \
		LAYOUT_CONTROL_UI_ATTR(GENERIC, lowercase, setLowercase) \
		LAYOUT_CONTROL_UI_ATTR(GENERIC, uppercase, setUppercase) \
		LAYOUT_CONTROL_UI_ATTR(GENERIC, multiLine, setMultiLine) \
		LAYOUT_CONTROL_ATTR(GENERIC, returnKey, setReturnKeyType) \
		LAYOUT_CONTROL_ATTR(GENERIC, keyboard, setKeyboardType) \
		LAYOUT_CONTROL_ATTR(GENERIC, autoCap, setAutoCapitalizationType) \
		LAYOUT_CONTROL_ATTR(BOOLEAN, focusNextOnReturnKey, setFocusNextOnReturnKey)

	BEGIN_PROCESS_LAYOUT_CONTROL(Edit, EditView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		PROCESS_EDIT_ATTRS

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

		if (op == OP_PARSE) {
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
			if (op == OP_GENERATE_CPP) {
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
			} else if (op == OP_SIMULATE) {
				if (!flagOnLayout) {
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
		}

		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, src, setSource)
		LAYOUT_CONTROL_ATTR(STRING, url, loadUrl)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

#define LAYOUT_CONTROL_PROCESS_SELECT_ITEMS \
		if (op == OP_PARSE) { \
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
		} else if (op == OP_GENERATE_CPP) { \
			ListElements<SAppLayoutSelectItem> selectItems(attr->items); \
			if (selectItems.count > 0) { \
				sl_size i; \
				for (i = 0; i < selectItems.count; i++) { \
					SAppLayoutSelectItem& selectItem = selectItems[i]; \
					String strSelectedItemTitle; \
					if (!(_getStringAccessString(resource->name, selectItem.title, strSelectedItemTitle))) { \
						return sl_false; \
					} \
					String strSelectedItemValue; \
					if (!(_getStringAccessString(resource->name, selectItem.value, strSelectedItemValue))) { \
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
		} else if (op == OP_SIMULATE) { \
			if (!flagOnLayout) { \
				ListElements<SAppLayoutSelectItem> selectItems(attr->items); \
				if (selectItems.count > 0) { \
					sl_uint32 n = (sl_uint32)(selectItems.count); \
					sl_uint32 i; \
					for (i = 0; i < n; i++) { \
						SAppLayoutSelectItem& selectItem = selectItems[i]; \
						String selectedItemTitle; \
						if (!(_getStringValue(resource->name, selectItem.title, selectedItemTitle))) { \
							return sl_false; \
						} \
						String selectedItemValue; \
						if (!(_getStringValue(resource->name, selectItem.value, selectedItemValue))) { \
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
			} \
		}

	BEGIN_PROCESS_LAYOUT_CONTROL(Select, SelectView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(COLOR, textColor, setTextColor)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)

		LAYOUT_CONTROL_SET_NATIVE_WIDGET

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS

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
		if (op == OP_PARSE) {
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

		if (op == OP_PARSE) {
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(childXmls, sl_null)
			if (childXmls.count > 0) {
				if (childXmls.count != 1) {
					logError(element, g_str_error_resource_layout_scrollview_must_contain_one_child);
					return sl_false;
				}
				Ref<SAppLayoutResourceItem> contentItem = _parseLayoutResourceItemChild(params->resource, resourceItem, childXmls[0], params->source);
				if (contentItem.isNull()) {
					return sl_false;
				}
				if (!(attr->scrolling.flagDefined) || attr->scrolling.vertical) {
					contentItem->attrs->topMode = PositionMode::Free;
					contentItem->attrs->bottomMode = PositionMode::Free;
				}
				if (!(attr->scrolling.flagDefined) || attr->scrolling.horizontal) {
					contentItem->attrs->leftMode = PositionMode::Free;
					contentItem->attrs->rightMode = PositionMode::Free;
				}
				attr->content = contentItem;
			}
		} else if (op == OP_GENERATE_CPP) {
			if (attr->content.isNotNull()) {
				String addChildStatement = String::format("%s%s->setContentView(%s, slib::UIUpdateMode::Init);%n%n", strTab, name, attr->content->name);
				if (!(_generateLayoutsCpp_Item(params->resource, attr->content.get(), resourceItem, *(params->sbDeclare), *(params->sbDefineInit), *(params->sbDefineLayout), addChildStatement))) {
					return sl_false;
				}
			}
		} else if (op == OP_SIMULATE) {
			if (attr->content.isNotNull()) {
				Ref<View> contentView = _simulateLayoutCreateOrLayoutView(params->simulator, attr->content.get(), resourceItem, view, flagOnLayout);
				if (contentView.isNotNull()) {
					if (!flagOnLayout) {
						view->setContentView(contentView, UIUpdateMode::Init);
					}
				} else {
					return sl_false;
				}
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Linear, LinearLayout)
	{
		LAYOUT_CONTROL_UI_ATTR(GENERIC, orientation, setOrientation)
		if (!(attr->orientation.flagDefined)) {
			if (op == OP_PARSE) {
				if (resourceItem->itemTypeName == "hlinear") {
					attr->orientation.flagDefined = sl_true;
					attr->orientation.value = LayoutOrientation::Horizontal;
				} else if (resourceItem->itemTypeName == "vlinear") {
					attr->orientation.flagDefined = sl_true;
					attr->orientation.value = LayoutOrientation::Vertical;
				}
			}
		}

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	namespace {
		class SimuationListViewAdapter : public ViewAdapter
		{
		public:
			WeakRef<CRef> refer;
			SAppLayoutSimulator* simulator;
			Ref<SAppLayoutResource> layout;

		public:
			sl_uint64 getItemCount() override
			{
				return 100;
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

		if (op == OP_PARSE) {
			attr->itemLayout = resourceItem->getXmlAttribute("item");
		} else if (op == OP_SIMULATE) {
			_openLayoutResource(resource, attr->itemLayout);

			if (attr->itemLayout.isNotEmpty() && !(view->getProperty("setAdapter").getBoolean())) {

				Ref<SAppLayoutResource> layoutItem;
				m_layouts.get(attr->itemLayout, &layoutItem);
				if (layoutItem.isNull()) {
					logError(element, g_str_error_layout_not_found, attr->itemLayout);
					return sl_false;
				}
				if (layoutItem->layoutType != SAppLayoutType::View) {
					logError(element, g_str_error_layout_is_not_view, attr->itemLayout);
					return sl_false;
				}

				Ref<SimuationListViewAdapter> adapter = new SimuationListViewAdapter;
				adapter->refer = params->simulator->getRef();
				adapter->simulator = params->simulator;
				adapter->layout = layoutItem;

				view->setAdapter(adapter);
				view->setProperty("setAdapter", sl_true);
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Collection, CollectionView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		if (op == OP_PARSE) {
			attr->itemLayout = resourceItem->getXmlAttribute("item");
		} else if (op == OP_SIMULATE) {
			_openLayoutResource(resource, attr->itemLayout);

			if (attr->itemLayout.isNotEmpty() && !(view->getProperty("setAdapter").getBoolean())) {

				Ref<SAppLayoutResource> layoutItem;
				m_layouts.get(attr->itemLayout, &layoutItem);
				if (layoutItem.isNull()) {
					logError(element, g_str_error_layout_not_found, attr->itemLayout);
					return sl_false;
				}
				if (layoutItem->layoutType != SAppLayoutType::View) {
					logError(element, g_str_error_layout_is_not_view, attr->itemLayout);
					return sl_false;
				}

				Ref<SimuationListViewAdapter> adapter = new SimuationListViewAdapter;
				adapter->refer = params->simulator->getRef();
				adapter->simulator = params->simulator;
				adapter->layout = layoutItem;

				view->setAdapter(adapter);
				view->setProperty("setAdapter", sl_true);
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Table, TableLayout)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		if (op == OP_PARSE) {
			{
				LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(columnXmls, "column")
				for (sl_size i = 0; i < columnXmls.count; i++) {
					LAYOUT_CONTROL_DEFINE_XML(columnXml, columnXmls[i])
					SAppLayoutTableColumn column;
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
					sl_uint32 iCell = 0;
					LAYOUT_CONTROL_DEFINE_XML_CHILDREN(childXmls, rowXml, sl_null)
					for (sl_size k = 0; k < childXmls.count; k++) {
						LAYOUT_CONTROL_DEFINE_XML(xmlView, childXmls[k])
						SAppLayoutTableCell cell;
						if (xmlView.getTagName() != "cell") {
							Ref<SAppLayoutResourceItem> subItemView = _parseLayoutResourceItemChild(params->resource, resourceItem, xmlView.element, params->source);
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
		} else if (op == OP_GENERATE_CPP) {
			ListElements<SAppLayoutTableColumn> cols(attr->columns);
			ListElements<SAppLayoutTableRow> rows(attr->rows);
			sl_uint32 iRow, iCol;
			sl_uint32 nCols = (sl_uint32)(cols.count);
			sl_uint32 nRows = (sl_uint32)(rows.count);
			LAYOUT_CONTROL_GENERATE(setColumnCount, "%d, slib::UIUpdateMode::Init", nCols)
			LAYOUT_CONTROL_GENERATE(setRowCount, "%d, slib::UIUpdateMode::Init", nRows)
			for (iCol = 0; iCol < nCols; iCol++) {
				SAppLayoutTableColumn& col = cols[iCol];
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
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				SAppLayoutTableRow& row = rows[iRow];
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
			}
		} else if (op == OP_SIMULATE) {
			ListElements<SAppLayoutTableColumn> cols(attr->columns);
			ListElements<SAppLayoutTableRow> rows(attr->rows);
			sl_uint32 iRow, iCol;
			sl_uint32 nCols = (sl_uint32)(cols.count);
			sl_uint32 nRows = (sl_uint32)(rows.count);
			if (!flagOnLayout) {
				view->setColumnCount(nCols, UIUpdateMode::Init);
				view->setRowCount(nRows, UIUpdateMode::Init);
			}
			for (iCol = 0; iCol < nCols; iCol++) {
				SAppLayoutTableColumn& col = cols[iCol];
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
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				SAppLayoutTableRow& row = rows[iRow];
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
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				SAppLayoutTableRow& row = rows[iRow];
				ListElements<SAppLayoutTableCell> cells(row.cells);
				sl_uint32 nCells = Math::min((sl_uint32)(cells.count), nCols);
				for (iCol = 0; iCol < nCells; iCol++) {
					SAppLayoutTableCell& cell = cells[iCol];
					if (cell.view.isNotNull()) {
						Ref<View> contentView = _simulateLayoutCreateOrLayoutView(params->simulator, cell.view.get(), resourceItem, view, flagOnLayout);
						if (contentView.isNotNull()) {
							if (!flagOnLayout) {
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

		if (op == OP_GENERATE_CPP) {
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
						if (!(_generateLayoutsCpp_Item(params->resource, cell.view.get(), resourceItem, *(params->sbDeclare), *(params->sbDefineInit), *(params->sbDefineLayout), addChildStatement))) {
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

		if (op == OP_PARSE) {
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
		} else if (op == OP_GENERATE_CPP) {
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
		} else if (op == OP_SIMULATE) {
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
		LAYOUT_CONTROL_UI_ATTR(GENERIC, tabAlign, setTabAlignment)

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabPaddingLeft, setTabPaddingLeft, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabPaddingTop, setTabPaddingTop, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabPaddingRight, setTabPaddingRight, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, tabPaddingBottom, setTabPaddingBottom, checkPosition)
		if (op == OP_PARSE) {
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
		if (op == OP_PARSE) {
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

		if (op == OP_PARSE) {
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
						logError(itemXml.element, g_str_error_resource_layout_item_must_contain_one_child);
						return sl_false;
					}
					Ref<SAppLayoutResourceItem> subItemView = _parseLayoutResourceItemChild(params->resource, resourceItem, childXmls[0], params->source);
					if (subItemView.isNull()) {
						return sl_false;
					}
					subItemView->attrs->resetLayout();
					subItem.view = subItemView;
				}
				if (!(attr->items.add_NoLock(Move(subItem)))) {
					logError(itemXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == OP_GENERATE_CPP) {
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
		} else if (op == OP_SIMULATE) {
			ListElements<SAppLayoutTabItem> subItems(attr->items);
			if (subItems.count > 0) {
				sl_uint32 nSubItems = (sl_uint32)(subItems.count);
				if (!flagOnLayout) {
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
						Ref<View> contentView = _simulateLayoutCreateOrLayoutView(params->simulator, subItem.view.get(), resourceItem, view, flagOnLayout);
						if (contentView.isNotNull()) {
							if (!flagOnLayout) {
								view->setTabContentView(i, contentView, UIUpdateMode::Init);
							}
						} else {
							return sl_false;
						}
					}
				}

				if (flagSelected) {
					if (!flagOnLayout) {
						view->selectTab(indexSelected, UIUpdateMode::Init);
					}
				}
			}

		}

		LAYOUT_CONTROL_SET_NATIVE_WIDGET

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == OP_GENERATE_CPP) {
			ListElements<SAppLayoutTabItem> subItems(attr->items);
			for (sl_size i = 0; i < subItems.count; i++) {
				SAppLayoutTabItem& subItem = subItems[i];
				if (subItem.view.isNotNull()) {
					String addChildStatement = String::format("%s%s->setTabContentView(%d, %s, slib::UIUpdateMode::Init);%n%n", strTab, name, i, subItem.view->name);
					if (!(_generateLayoutsCpp_Item(params->resource, subItem.view.get(), resourceItem, *(params->sbDeclare), *(params->sbDefineInit), *(params->sbDefineLayout), addChildStatement))) {
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

		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, itemIcon, setItemIcon)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, opendIcon, setOpenedItemIcon)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, closedIcon, setClosedItemIcon)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, collapsedIcon, setCollapsedIcon)
		LAYOUT_CONTROL_UI_ATTR(DRAWABLE, expandedIcon, setExpandedIcon)
		LAYOUT_CONTROL_STATE_MAP(DRAWABLE, itemBackground, setItemBackground)
		LAYOUT_CONTROL_STATE_MAP(COLOR, textColor, setItemTextColor)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemHeight, setItemHeight, checkScalarSize)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemPadding, setItemPadding, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, itemIndent, setItemIndent, checkPosition)
		LAYOUT_CONTROL_UI_ATTR(DIMENSION, textIndent, setTextIndent, checkPosition)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Split, SplitLayout)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(GENERIC, orientation, setOrientation)
		if (!(attr->orientation.flagDefined)) {
			if (op == OP_PARSE) {
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

		sl_bool flagRelayoutOnInit = sl_false;
		sl_bool flagRelayoutOnLayout = sl_false;

		if (op == OP_PARSE) {
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
						logError(itemXml.element, g_str_error_resource_layout_item_must_contain_one_child);
						return sl_false;
					}
					Ref<SAppLayoutResourceItem> subItemView = _parseLayoutResourceItemChild(params->resource, resourceItem, childXmls[0], params->source);
					if (subItemView.isNull()) {
						return sl_false;
					}
					subItemView->attrs->width.flagDefined = sl_false;
					subItemView->attrs->height.flagDefined = sl_false;
					subItemView->attrs->leftMode = PositionMode::Free;
					subItemView->attrs->topMode = PositionMode::Free;
					subItem.view = subItemView;
				}
				if (!(attr->items.add_NoLock(Move(subItem)))) {
					logError(itemXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == OP_GENERATE_CPP) {
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
		} else if (op == OP_SIMULATE) {
			ListElements<SAppLayoutSplitItem> subItems(attr->items);
			if (subItems.count > 0) {
				if (!flagOnLayout) {
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
						Ref<View> contentView = _simulateLayoutCreateOrLayoutView(params->simulator, subItem.view.get(), resourceItem, view, flagOnLayout);
						if (contentView.isNotNull()) {
							if (!flagOnLayout) {
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

		if (op == OP_GENERATE_CPP) {
			ListElements<SAppLayoutSplitItem> subItems(attr->items);
			for (sl_size i = 0; i < subItems.count; i++) {
				SAppLayoutSplitItem& subItem = subItems[i];
				if (subItem.view.isNotNull()) {
					String addChildStatement = String::format("%s%s->setItemView(%d, %s, slib::UIUpdateMode::Init);%n%n", strTab, name, i, subItem.view->name);
					if (!(_generateLayoutsCpp_Item(params->resource, subItem.view.get(), resourceItem, *(params->sbDeclare), *(params->sbDefineInit), *(params->sbDefineLayout), addChildStatement))) {
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

		if (op == OP_PARSE) {
			LAYOUT_CONTROL_PARSE_ATTR(STRING, attr->, url)
			LAYOUT_CONTROL_PARSE_ATTR(STRING, attr->, html)
		} else {
			if (op == OP_GENERATE_CPP) {
				String strUrl;
				if (!(_getStringAccessString(resource->name, attr->url, strUrl))) {
					return sl_false;
				}
				String strHtml;
				if (!(_getStringAccessString(resource->name, attr->html, strHtml))) {
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
			} else if (op == OP_SIMULATE) {
				if (!flagOnLayout) {
					String _url;
					if (!(_getStringValue(resource->name, attr->url, _url))) {
						return sl_false;
					}
					String _html;
					if (!(_getStringValue(resource->name, attr->html, _html))) {
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
		LAYOUT_CONTROL_UI_ATTR(GENERIC, value, setValue)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, value2, setSecondaryValue)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, dual, setDualValues)
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
		if (op == OP_PARSE) {
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
		if (op == OP_PARSE) { \
			LAYOUT_CONTROL_PARSE_##TYPE(*(resourceItem), #NAME "Off", , attr->NAME##s[0], ##__VA_ARGS__) \
			LAYOUT_CONTROL_PARSE_##TYPE(*(resourceItem), #NAME "On", , attr->NAME##s[1], ##__VA_ARGS__) \
		} else if (op == OP_GENERATE_CPP) { \
			LAYOUT_CONTROL_GENERATE_##TYPE(attr->NAME##s[0], SETFUNC, CONTROL, "slib::SwitchValue::Off, %s", value) \
			LAYOUT_CONTROL_GENERATE_##TYPE(attr->NAME##s[1], SETFUNC, CONTROL, "slib::SwitchValue::On, %s", value) \
		} else if (op == OP_SIMULATE) { \
			LAYOUT_CONTROL_SIMULATE_##TYPE(attr->NAME##s[0], SETFUNC, CONTROL, SwitchValue::Off, value) \
			LAYOUT_CONTROL_SIMULATE_##TYPE(attr->NAME##s[1], SETFUNC, CONTROL, SwitchValue::On, value) \
		}

#define LAYOUT_CONTROL_SWITCH_STATE_MAP(TYPE, NAME, SETFUNC, ...) \
		if (op == OP_PARSE) { \
			LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, *(resourceItem), #NAME "Off", , attr->NAME##s[0], ##__VA_ARGS__) \
			LAYOUT_CONTROL_PARSE_STATE_MAP(TYPE, *(resourceItem), #NAME "On", , attr->NAME##s[1], ##__VA_ARGS__) \
		} else if (op == OP_GENERATE_CPP) { \
			LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, attr->NAME##s[0], SETFUNC, CONTROL, "slib::SwitchValue::Off, %s", value) \
			LAYOUT_CONTROL_GENERATE_STATE_MAP(TYPE, attr->NAME##s[1], SETFUNC, CONTROL, "slib::SwitchValue::On, %s", value) \
		} else if (op == OP_SIMULATE) { \
			LAYOUT_CONTROL_SIMULATE_STATE_MAP(TYPE, attr->NAME##s[0], SETFUNC, CONTROL, SwitchValue::Off, value) \
			LAYOUT_CONTROL_SIMULATE_STATE_MAP(TYPE, attr->NAME##s[1], SETFUNC, CONTROL, SwitchValue::On, value) \
		}

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

		if (op == OP_PARSE) {
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(itemXmls, "item")
			for (sl_size i = 0; i < itemXmls.count; i++) {
				LAYOUT_CONTROL_DEFINE_XML(itemXml, itemXmls[i])
				SAppLayoutPagerItem subItem;
				LAYOUT_CONTROL_PARSE_XML(GENERIC, itemXml, subItem., selected)
				LAYOUT_CONTROL_DEFINE_XML_CHILDREN(childXmls, itemXml, sl_null)
				if (childXmls.count > 0) {
					if (childXmls.count != 1) {
						logError(itemXml.element, g_str_error_resource_layout_item_must_contain_one_child);
						return sl_false;
					}
					Ref<SAppLayoutResourceItem> subItemView = _parseLayoutResourceItemChild(params->resource, resourceItem, childXmls[0], params->source);
					if (subItemView.isNull()) {
						return sl_false;
					}
					subItemView->attrs->resetLayout();
					subItem.view = subItemView;
				}
				if (!(attr->items.add_NoLock(Move(subItem)))) {
					logError(itemXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == OP_GENERATE_CPP) {
			/* see below */
		} else if (op == OP_SIMULATE) {
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
						Ref<View> contentView = _simulateLayoutCreateOrLayoutView(params->simulator, subItem.view.get(), resourceItem, view, flagOnLayout);
						if (contentView.isNotNull()) {
							if (!flagOnLayout) {
								view->addPage(contentView, UIUpdateMode::Init);
							}
						} else {
							return sl_false;
						}
					}

				}

				if (!flagOnLayout) {
					view->selectPage(indexSelected);
				}

			}

		}

		LAYOUT_CONTROL_ADD_STATEMENT

		if (op == OP_GENERATE_CPP) {
			ListElements<SAppLayoutPagerItem> subItems(attr->items);
			if (subItems.count > 0) {
				sl_size indexSelected = 0;
				for (sl_size i = 0; i < subItems.count; i++) {
					SAppLayoutPagerItem& subItem = subItems[i];
					if (subItem.view.isNotNull()) {
						String addChildStatement = String::format("%s%s->addPage(%s, slib::UIUpdateMode::Init);%n%n", strTab, name, subItem.view->name);
						if (!(_generateLayoutsCpp_Item(params->resource, subItem.view.get(), resourceItem, *(params->sbDeclare), *(params->sbDefineInit), *(params->sbDefineLayout), addChildStatement))) {
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

	BEGIN_PROCESS_LAYOUT_CONTROL(Video, VideoView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ATTR(GENERIC, repeat, setRepeat)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, rotation, setRotation)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, flip, setFlip)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, scale, setScaleMode)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, gravity, setGravity)
		LAYOUT_CONTROL_UI_ATTR(GENERIC, controls, setControlsVisible)

		if (op == OP_PARSE || op == OP_GENERATE_CPP) {
			LAYOUT_CONTROL_ATTR(STRING, src, setSource)
		} else {
			if (!flagOnLayout) {
				if (attr->src.flagDefined) {
					String value;
					if (!(_getStringValue(resource->name, attr->src, value))) {
						return sl_false;
					}
					if (value.startsWith("asset://")) {
						value = String::concat(m_pathApp, "/asset/", value.substring(8));
					}
					view->setSource(value);
				}
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

		if (op == OP_PARSE) {
			sl_size nChildren = element->getChildElementCount();
			if (nChildren > 0) {
				if (nChildren != 1) {
					logError(element, g_str_error_resource_layout_refreshview_must_contain_one_child);
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

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS

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

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Pdf, PdfView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(GroupBox, GroupBox)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(STRING, label, setLabel)
		LAYOUT_CONTROL_UI_ATTR(COLOR, labelColor, setLabelColor)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	namespace {
		static sl_bool GetGridCellCreator(SAppLayoutGridCell::Creator& creator, const String& tagName)
		{
#define DEFINE_CHECK_GRID_CELL_CREATOR(NAME, TAG) \
			if (tagName == TAG) { \
				creator = SAppLayoutGridCell::Creator::NAME; \
				return sl_true; \
			}

			DEFINE_CHECK_GRID_CELL_CREATOR(Label, "label")
			DEFINE_CHECK_GRID_CELL_CREATOR(Text, "text")
			DEFINE_CHECK_GRID_CELL_CREATOR(HyperText, "hyper")
			DEFINE_CHECK_GRID_CELL_CREATOR(Numero, "no")
			return sl_false;
		}
	}

	BEGIN_PROCESS_LAYOUT_CONTROL(Grid, GridView)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_UI_ATTR(DIMENSION, rowHeight, setRowHeight, checkScalarSize)

		if (op == OP_PARSE) {

#define LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(ATTR, XML) \
			{ \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., align) \
				LAYOUT_CONTROL_PARSE_XML(FONT, XML, ATTR., font) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., multiLine) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., ellipsize) \
				LAYOUT_CONTROL_PARSE_XML(GENERIC, XML, ATTR., lineCount) \
				LAYOUT_CONTROL_PARSE_STATE_MAP_XML(DRAWABLE, XML, ATTR., background) \
				LAYOUT_CONTROL_PARSE_STATE_MAP_XML(COLOR, XML, ATTR., textColor) \
			}
			{
				LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(columnXmls, "column")
				sl_bool flagLeft = sl_true;
				for (sl_size i = 0; i < columnXmls.count; i++) {
					LAYOUT_CONTROL_DEFINE_XML(columnXml, columnXmls[i])
					SAppLayoutGridColumn column;
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, columnXml, column., width, checkScalarSize)
					LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(column, columnXml)
					SAppLayoutXmlItem header(LAYOUT_CONTROL_GET_XML_CHILDREN(columnXml, "header").getFirstValue_NoLock());
					if (header.element.isNotNull()) {
						LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(column.headerAttrs, header)
					}
					SAppLayoutXmlItem body(LAYOUT_CONTROL_GET_XML_CHILDREN(columnXml, "body").getFirstValue_NoLock());
					if (body.element.isNotNull()) {
						LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(column.bodyAttrs, body)
					}
					SAppLayoutXmlItem footer(LAYOUT_CONTROL_GET_XML_CHILDREN(columnXml, "footer").getFirstValue_NoLock());
					if (footer.element.isNotNull()) {
						LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(column.footerAttrs, footer)
					}
					SAppBooleanValue fixed;
					LAYOUT_CONTROL_PARSE_XML(GENERIC, columnXml, ,fixed)
					if (!(fixed.flagDefined && fixed.value)) {
						flagLeft = sl_false;
						attr->nRightColumns = (sl_uint32)(columnXmls.count - 1 - i);
					}
					if (flagLeft) {
						attr->nLeftColumns = (sl_uint32)(i + 1);
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
					LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(row, rowXml) \
					row.font.inheritFrom(attr->SECTION.font); \
					LAYOUT_CONTROL_PARSE_XML(DIMENSION, rowXml, row., height, checkScalarSize) \
					sl_uint32 iCell = 0; \
					LAYOUT_CONTROL_DEFINE_XML_CHILDREN(cellXmls, rowXml, sl_null) \
					for (sl_size k = 0; k < cellXmls.count; k++) { \
						LAYOUT_CONTROL_DEFINE_XML(cellXml, cellXmls[k]) \
						SAppLayoutGridCell cell; \
						if (!(GetGridCellCreator(cell.creator, cellXml.getTagName()))) { \
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
			{ \
				LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(attr->SECTION, XML) \
				LAYOUT_CONTROL_PARSE_XML(DIMENSION, XML, attr->SECTION., rowHeight, checkScalarSize) \
				LAYOUT_CONTROL_DEFINE_XML_CHILDREN(rowXmls, XML, "row") \
				LAYOUT_CONTROL_PARSE_GRID_ROWS(SECTION) \
				attr->SECTION.font.inheritFrom(attr->font); \
			}

			LAYOUT_CONTROL_DEFINE_XML(header, LAYOUT_CONTROL_GET_ITEM_CHILDREN("header").getFirstValue_NoLock())
			if (header.element.isNotNull()) {
				LAYOUT_CONTROL_PARSE_GRID_SECTION(header, header)
			}
			LAYOUT_CONTROL_DEFINE_XML(body, LAYOUT_CONTROL_GET_ITEM_CHILDREN("body").getFirstValue_NoLock())
			if (body.element.isNotNull()) {
				LAYOUT_CONTROL_PARSE_GRID_SECTION(body, body)
			} else {
				LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(rowXmls, "row")
				LAYOUT_CONTROL_PARSE_GRID_ROWS(body)
			}
			LAYOUT_CONTROL_DEFINE_XML(footer, LAYOUT_CONTROL_GET_ITEM_CHILDREN("footer").getFirstValue_NoLock())
			if (footer.element.isNotNull()) {
				LAYOUT_CONTROL_PARSE_GRID_SECTION(footer, footer)
			}

			{
				ListElements<SAppLayoutGridColumn> columns(attr->columns);
				for (sl_size i = 0; i < columns.count; i++) {
					SAppLayoutGridColumn& column = columns[i];
					column.font.inheritFrom(attr->font);
					column.headerAttrs.font.inheritFrom(attr->font);
					column.bodyAttrs.font.inheritFrom(attr->font);
					column.footerAttrs.font.inheritFrom(attr->font);
				}
			}

		} else if (op == OP_GENERATE_CPP) {
		
			ListElements<SAppLayoutGridColumn> columns(attr->columns);
			
			LAYOUT_CONTROL_GENERATE(setColumnCount, "%d, slib::UIUpdateMode::Init", columns.count)
			LAYOUT_CONTROL_GENERATE(setLeftColumnCount, "%d, slib::UIUpdateMode::Init", attr->nLeftColumns)
			LAYOUT_CONTROL_GENERATE(setRightColumnCount, "%d, slib::UIUpdateMode::Init", attr->nRightColumns)
			LAYOUT_CONTROL_GENERATE(setBodyRowCount, "%d, slib::UIUpdateMode::Init", attr->body.rows.getCount())
			LAYOUT_CONTROL_GENERATE(setHeaderRowCount, "%d, slib::UIUpdateMode::Init", attr->header.rows.getCount())
			LAYOUT_CONTROL_GENERATE(setFooterRowCount, "%d, slib::UIUpdateMode::Init", attr->footer.rows.getCount())

#define LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(PREFIX, ATTR, ARG_FORMAT, ...) \
			{ \
				LAYOUT_CONTROL_GENERATE_STRING(ATTR.text, set##PREFIX##Text, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.align, set##PREFIX##Alignment, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_FONT(ATTR.font, set##PREFIX##Font, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.multiLine, set##PREFIX##MultiLine, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.ellipsize, set##PREFIX##Ellipsize, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_GENERIC(ATTR.lineCount, set##PREFIX##LineCount, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_STATE_MAP(DRAWABLE, ATTR.background, set##PREFIX##Background, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
				LAYOUT_CONTROL_GENERATE_STATE_MAP(COLOR, ATTR.textColor, set##PREFIX##TextColor, ITEM, ARG_FORMAT, ##__VA_ARGS__) \
			}

			{
				for (sl_size iCol = 0; iCol < columns.count; iCol++) {
					SAppLayoutGridColumn& column = columns[iCol];
					LAYOUT_CONTROL_GENERATE_DIMENSION(column.width, setColumnWidth, ITEM, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(Column, column, "%d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(Body, column.bodyAttrs, "-1, %d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(Header, column.headerAttrs, "-1, %d, %s", iCol, value)
					LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(Footer, column.footerAttrs, "-1, %d, %s", iCol, value)
				}
			}
			
#define LAYOUT_CONTROL_GENERATE_GRID_SECTION(SECTION, PREFIX) \
			{ \
				auto& section = attr->SECTION; \
				LAYOUT_CONTROL_GENERATE_UI_ATTR(DIMENSION, section.rowHeight, set##PREFIX##RowHeight) \
				LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(PREFIX, section, "-1, -1, %s", value) \
				ListElements<SAppLayoutGridRow> rows(section.rows); \
				for (sl_size iRow = 0; iRow < rows.count; iRow++) { \
					SAppLayoutGridRow& row = rows[iRow]; \
					LAYOUT_CONTROL_GENERATE_DIMENSION(row.height, set##PREFIX##RowHeight, ITEM, "%d, %s", iRow, value) \
					LAYOUT_CONTROL_GENERATE_GRID_CELL_ATTRIBUTES(PREFIX, section, "%d, -1, %s", iRow, value) \
					ListElements<SAppLayoutGridCell> cells(row.cells); \
					for (sl_size iCell = 0; iCell < cells.count; iCell++) { \
						SAppLayoutGridCell& cell = cells[iCell]; \
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

			LAYOUT_CONTROL_GENERATE_GRID_SECTION(body, Body)
			LAYOUT_CONTROL_GENERATE_GRID_SECTION(header, Header)
			LAYOUT_CONTROL_GENERATE_GRID_SECTION(footer, Footer)

		} else if (op == OP_SIMULATE) {

			ListElements<SAppLayoutGridColumn> columns(attr->columns);
			if (!flagOnLayout) {
				view->setColumnCount((sl_uint32)(columns.count), UIUpdateMode::Init);
				view->setLeftColumnCount(attr->nLeftColumns, UIUpdateMode::Init);
				view->setRightColumnCount(attr->nRightColumns, UIUpdateMode::Init);
				view->setBodyRowCount((sl_uint32)(attr->body.rows.getCount()), UIUpdateMode::Init);
				view->setHeaderRowCount((sl_uint32)(attr->header.rows.getCount()), UIUpdateMode::Init);
				view->setFooterRowCount((sl_uint32)(attr->footer.rows.getCount()), UIUpdateMode::Init);
			}

#define LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(PREFIX, ATTR, ...) \
			{ \
				LAYOUT_CONTROL_SIMULATE_STRING(ATTR.text, set##PREFIX##Text, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.align, set##PREFIX##Alignment, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_FONT(ATTR.font, set##PREFIX##Font, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.multiLine, set##PREFIX##MultiLine, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.ellipsize, set##PREFIX##Ellipsize, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_GENERIC(ATTR.lineCount, set##PREFIX##LineCount, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_STATE_MAP(DRAWABLE, ATTR.background, set##PREFIX##Background, ITEM, ##__VA_ARGS__, value) \
				LAYOUT_CONTROL_SIMULATE_STATE_MAP(COLOR, ATTR.textColor, set##PREFIX##TextColor, ITEM, ##__VA_ARGS__, value) \
			}

			{
				for (sl_size iCol = 0; iCol < columns.count; iCol++) {
					SAppLayoutGridColumn& column = columns[iCol];
					LAYOUT_CONTROL_SIMULATE_DIMENSION(column.width, setColumnWidth, ITEM, (sl_uint32)iCol, value)
					LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(Column, column, (sl_uint32)iCol)
					LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(Body, column.bodyAttrs, -1, (sl_uint32)iCol)
					LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(Header, column.headerAttrs, -1, (sl_uint32)iCol)
					LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(Footer, column.footerAttrs, -1, (sl_uint32)iCol)
				}
			}

#define LAYOUT_CONTROL_SIMULATE_GRID_SECTION(SECTION, PREFIX) \
			{ \
				auto& section = attr->SECTION; \
				LAYOUT_CONTROL_SIMULATE_UI_ATTR(DIMENSION, section.rowHeight, set##PREFIX##RowHeight) \
				LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(PREFIX, section, -1, -1) \
				ListElements<SAppLayoutGridRow> rows(section.rows); \
				for (sl_size iRow = 0; iRow < rows.count; iRow++) { \
					SAppLayoutGridRow& row = rows[iRow]; \
					LAYOUT_CONTROL_SIMULATE_DIMENSION(row.height, set##PREFIX##RowHeight, ITEM, (sl_uint32)iRow, value) \
					LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(PREFIX, section, (sl_uint32)iRow, -1) \
					ListElements<SAppLayoutGridCell> cells(row.cells); \
					for (sl_size iCell = 0; iCell < cells.count; iCell++) { \
						SAppLayoutGridCell& cell = cells[iCell]; \
						LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(PREFIX, cell, (sl_uint32)iRow, (sl_uint32)iCell) \
						if (cell.colspan.flagDefined && cell.rowspan.flagDefined && !flagOnLayout) { \
							view->set##PREFIX##Span((sl_uint32)iRow, (sl_uint32)iCell, cell.rowspan.value, cell.colspan.value, UIUpdateMode::Init); \
						} else { \
							LAYOUT_CONTROL_SIMULATE_GENERIC(cell.rowspan, set##PREFIX##Rowspan, ITEM, (sl_uint32)iRow, (sl_uint32)iCell, value) \
							LAYOUT_CONTROL_SIMULATE_GENERIC(cell.colspan, set##PREFIX##Colspan, ITEM, (sl_uint32)iRow, (sl_uint32)iCell, value) \
						} \
					} \
				} \
			}

			LAYOUT_CONTROL_SIMULATE_GRID_SECTION(body, Body)
			LAYOUT_CONTROL_SIMULATE_GRID_SECTION(header, Header)
			LAYOUT_CONTROL_SIMULATE_GRID_SECTION(footer, Footer)
		}

		LAYOUT_CONTROL_UI_ATTR(GENERIC, recordCount, setRecordCount)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(XControl, XControl)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(XButton, XButton)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(Button)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(XEdit, XEditView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(XControl)

		PROCESS_EDIT_ATTRS

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(XPassword, XPasswordView)
	{
		if (!(_processLayoutResourceControl_XEdit(params))) {
			return sl_false;
		}
	}
	END_PROCESS_LAYOUT_CONTROL

}
