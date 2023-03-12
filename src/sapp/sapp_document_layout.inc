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
			_logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		style->element = element;

		String name = element->getAttribute("name").trim();
		if (name.isEmpty()) {
			_logError(element, g_str_error_resource_layout_name_is_empty);
			return sl_false;
		}

		name = getNameInLocalNamespace(localNamespace, name);

		if (m_layoutStyles.find(name)) {
			_logError(element, g_str_error_resource_layout_name_redefined, name);
			return sl_false;
		}
		style->name = name;

		if (!(m_layoutStyles.put(name, style))) {
			_logError(element, g_str_error_out_of_memory);
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
						_logError(element, g_str_error_out_of_memory);
						return sl_false;
					}
				} else {
					_logError(element, g_str_error_layout_style_not_found, s);
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
			_logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		include->element = element;

		String name = element->getAttribute("name").trim();
		if (name.isEmpty()) {
			_logError(element, g_str_error_resource_layout_name_is_empty);
			return sl_false;
		}

		name = getNameInLocalNamespace(localNamespace, name);

		if (m_layoutIncludes.find(name)) {
			_logError(element, g_str_error_resource_layout_name_redefined, name);
			return sl_false;
		}
		include->name = name;

		if (!(m_layoutIncludes.put(name, include))) {
			_logError(element, g_str_error_out_of_memory);
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
			_logError(element, g_str_error_resource_layout_name_is_empty);
			return sl_false;
		}

		name = getNameInLocalNamespace(localNamespace, name);

		if (m_layoutUnits.find(name)) {
			_logError(element, g_str_error_resource_layout_name_redefined, name);
			return sl_false;
		}

		String strValue = element->getText();

		SAppDimensionValue value;
		if (!(value.parse(strValue, this))) {
			_logError(element, g_str_error_resource_layout_value_invalid, strValue);
			return sl_false;
		}

		if (!(m_layoutUnits.put(name, value))) {
			_logError(element, g_str_error_out_of_memory);
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
			_logError(element, g_str_error_out_of_memory);
			return sl_false;
		}

		layout->filePath = filePath;
		layout->element = element;

		if (m_layouts.find(localNamespace)) {
			_logError(element, g_str_error_resource_layout_name_redefined, localNamespace);
			return sl_false;
		}
		layout->name = localNamespace;

		if (!(_parseLayoutResourceItem(layout.get(), layout.get(), sl_null, source))) {
			return sl_false;
		}

		if (!(m_layouts.put(localNamespace, layout))) {
			_logError(element, g_str_error_out_of_memory);
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
				_logError(element, g_str_error_resource_layout_type_invalid, strType);
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
				_logError(element, g_str_error_resource_layout_type_invalid, strType);
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
					_logError(element, g_str_error_resource_layout_name_invalid, name);
					return sl_false;
				}
				if (layout->itemsByName.find(name)) {
					_logError(element, g_str_error_resource_layout_name_redefined, name);
					return sl_false;
				}
				if (layout->radioGroups.find(name)) {
					_logError(element, g_str_error_resource_layout_name_redefined, name);
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
				_logError(element, g_str_error_out_of_memory);
				return sl_false;
			}
		}

		if (!parent) {
			String strSP = layout->getXmlAttribute("sp");
			if (!(layout->sp.parse(strSP, this))) {
				_logError(element, g_str_error_resource_layout_attribute_invalid, "sp", strSP);
			}
			if (!(layout->sp.checkSP())) {
				_logError(element, g_str_error_resource_layout_attribute_invalid, "sp", strSP);
				return sl_false;
			}
		}

		if (parent) {
			if (!(layout->itemsByName.put(item->name, item))) {
				_logError(element, g_str_error_out_of_memory);
				return sl_false;
			}
		}
		if (item->arrayIndex >= 0) {
			sl_uint32 n = item->arrayIndex + 1;
			SAppLayoutResource::ItemArrayDesc desc;
			if (layout->itemArrays.get(item->arrayName, &desc)) {
				if (desc.className != item->className) {
					_logError(element, g_str_error_resource_layout_name_array_item_class_different, item->name);
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
			_logError(element, g_str_error_out_of_memory);
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
		_log(g_str_log_generate_cpp_layouts_begin);

		if (!(File::isDirectory(targetPath + "/ui"))) {
			File::createDirectory(targetPath + "/ui");
			if (!(File::isDirectory(targetPath + "/ui"))) {
				_log(g_str_error_directory_create_failed, targetPath + "/ui");
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
				_logError(g_str_error_file_write_failed, pathHeaderBase);
				return sl_false;
			}
		}

		String pathHeader = targetPath + "/layouts.h";
		String contentHeader = sbHeader.merge();
		if (File::readAllTextUTF8(pathHeader) != contentHeader) {
			if (!(File::writeAllTextUTF8(pathHeader, contentHeader))) {
				_logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}

		String pathCpp = targetPath + "/layouts.cpp";
		String contentCpp = sbCpp.merge();
		if (File::readAllTextUTF8(pathCpp) != contentCpp) {
			if (!(File::writeAllTextUTF8(pathCpp, contentCpp))) {
				_logError(g_str_error_file_write_failed, pathCpp);
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
				_logError(g_str_error_file_write_failed, pathHeader);
				return sl_false;
			}
		}

		String pathCpp = String::concat(targetPath, "/ui/", name, ".cpp.inc");
		String contentCpp = sbCpp.merge();
		if (File::readAllTextUTF8(pathCpp) != contentCpp) {
			if (!(File::writeAllTextUTF8(pathCpp, contentCpp))) {
				_logError(g_str_error_file_write_failed, pathCpp);
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
						_logError(item->element, g_str_error_out_of_memory);
						return sl_false;
					}
				} else {
					_logError(item->element, g_str_error_layout_style_not_found, s);
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
							_logError(child, g_str_error_resource_layout_attribute_invalid, "src", name);
							return sl_false;
						}
						Ref<SAppLayoutInclude> include;
						getItemFromMap(m_layoutIncludes, localNamespace, src, sl_null, &include);
						if (include.isNotNull()) {
							if (!_addXmlChildElements(list, include->element, localNamespace, tagName)) {
								return sl_false;
							}
						} else {
							_logError(child, g_str_error_layout_include_not_found, name);
							return sl_false;
						}
					} else if (tagName.isEmpty() || name == tagName) {
						if (!(list.add_NoLock(Move(child)))) {
							_logError(child, g_str_error_out_of_memory);
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
								_logError(resourceItem->element, g_str_error_out_of_memory);
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
					_logError(element, g_str_error_out_of_memory); \
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
	_logError(XML.element, g_str_error_resource_layout_attribute_invalid, #NAME, str_##NAME);
#define LOG_ERROR_LAYOUT_CONTROL_ATTR(NAME) LOG_ERROR_LAYOUT_CONTROL_XML_ATTR((*resourceItem), NAME)

#define LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR, NAME) \
	String str_##NAME = XML.getXmlAttribute(#NAME); \
	if (!(ATTR NAME.parse(str_##NAME))) { \
		LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME) \
		return sl_false; \
	}
#define LAYOUT_CONTROL_PARSE_ATTR(ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_ATTR((*resourceItem), ATTR, NAME)

#define PRIV_LAYOUT_CONTROL_GENERATE(BUF_TYPE, SETFUNC, ARG_FORMAT, ...) \
	params->sbDefine##BUF_TYPE->add(String::format("%s%s->" #SETFUNC "(" ARG_FORMAT ");%n", strTab, name, ##__VA_ARGS__));
#define LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT, ...) PRIV_LAYOUT_CONTROL_GENERATE(Init, SETFUNC, ARG_FORMAT, ##__VA_ARGS__)
#define LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, ARG_FORMAT, ...) PRIV_LAYOUT_CONTROL_GENERATE(Layout, SETFUNC, ARG_FORMAT, ##__VA_ARGS__)

#define PRIV_LAYOUT_CONTROL_GENERATE_ATTR(VAR, SETFUNC, ARG_FORMAT) \
	if (VAR.flagDefined) { \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT, VAR.getAccessString()) \
	}
#define LAYOUT_CONTROL_GENERATE_ATTR(VAR, SETFUNC, ...) PRIV_LAYOUT_CONTROL_GENERATE_ATTR(VAR, SETFUNC, __VA_ARGS__ "%s")
#define LAYOUT_CONTROL_GENERATE_ATTR_UPDATE(VAR, SETFUNC, ...) PRIV_LAYOUT_CONTROL_GENERATE_ATTR(VAR, SETFUNC, __VA_ARGS__ "%s, slib::UIUpdateMode::Init")

#define LAYOUT_CONTROL_SIMULATE_ATTR(VAR, SETFUNC, ...) \
	if (!flagOnLayout && VAR.flagDefined) { \
		view->SETFUNC(__VA_ARGS__ VAR.value); \
	}
#define LAYOUT_CONTROL_SIMULATE_ATTR_UPDATE(VAR, SETFUNC, ...) \
	if (!flagOnLayout && VAR.flagDefined) { \
		view->SETFUNC(__VA_ARGS__ VAR.value, UIUpdateMode::Init); \
	}

#define LAYOUT_CONTROL_GENERIC_ATTR(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_ATTR(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_ATTR(attr->NAME, SETFUNC) \
	}
#define LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_ATTR_UPDATE(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_ATTR_UPDATE(attr->NAME, SETFUNC) \
	}
	
#define LAYOUT_CONTROL_BOOLEAN_ATTR(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		if (attr->NAME.flagDefined && attr->NAME.value) { \
			LAYOUT_CONTROL_GENERATE(SETFUNC, ""); \
		} \
	} else if (op == OP_SIMULATE) { \
		if (!flagOnLayout && attr->NAME.flagDefined && attr->NAME.value) { \
			view->SETFUNC(); \
		} \
	}

#define LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(XML, ATTR, NAME, CHECKFUNC) \
	String str_##NAME = XML.getXmlAttribute(#NAME); \
	if (!(ATTR NAME.parse(str_##NAME, this))) { \
		LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME) \
		return sl_false; \
	} \
	if (!(ATTR NAME.CHECKFUNC())) { \
		LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME) \
		return sl_false; \
	}
#define LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(ATTR, NAME, CHECKFUNC) LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR((*resourceItem), ATTR, NAME, CHECKFUNC)

#define LAYOUT_CONTROL_GENERATE_DIMENSION_ATTR(VAR, SETFUNC, ...) \
	if (VAR.flagDefined) { \
		if (!(SAppDimensionValue::isRelativeUnit(VAR.unit))) { \
			if (VAR.isNeededOnLayoutFunction()) { \
				LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, __VA_ARGS__ "%s", VAR.getAccessString()) \
			} else { \
				LAYOUT_CONTROL_GENERATE(SETFUNC, __VA_ARGS__ "%s", VAR.getAccessString()) \
			} \
		} \
	}
#define LAYOUT_CONTROL_GENERATE_DIMENSION_ATTR_UPDATE(VAR, SETFUNC, ...) \
	if (VAR.flagDefined) { \
		if (!(SAppDimensionValue::isRelativeUnit(VAR.unit))) { \
			if (VAR.isNeededOnLayoutFunction()) { \
				LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, __VA_ARGS__ "%s", VAR.getAccessString()) \
			} else { \
				LAYOUT_CONTROL_GENERATE(SETFUNC, __VA_ARGS__ "%s, slib::UIUpdateMode::Init", VAR.getAccessString()) \
			} \
		} \
	}

#define LAYOUT_CONTROL_SIMULATE_DIMENSION_ATTR(VAR, SETFUNC, ...) \
	if (flagOnLayout && VAR.flagDefined) { \
		if (!(SAppDimensionValue::isRelativeUnit(VAR.unit))) { \
			view->SETFUNC(__VA_ARGS__ _getDimensionValue(VAR)); \
		} \
	}
	
#define LAYOUT_CONTROL_DIMENSION_ATTR(NAME, SETFUNC, CHECKFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(attr->, NAME, CHECKFUNC) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_DIMENSION_ATTR(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_DIMENSION_ATTR(attr->NAME, SETFUNC) \
	}
#define LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(NAME, SETFUNC, CHECKFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(attr->, NAME, CHECKFUNC) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_DIMENSION_ATTR_UPDATE(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_DIMENSION_ATTR(attr->NAME, SETFUNC) \
	}

#define LAYOUT_CONTROL_PARSE_XML_REFERING_ATTR(XML, ATTR, NAME) \
	String str_##NAME = XML.getXmlAttribute(#NAME); \
	if (!(ATTR NAME.parse(str_##NAME, XML.element))) { \
		LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME) \
		return sl_false; \
	}
#define LAYOUT_CONTROL_PARSE_REFERING_ATTR(ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_REFERING_ATTR((*resourceItem), ATTR, NAME)

#define LAYOUT_CONTROL_PARSE_XML_STRING_ATTR(XML, ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_REFERING_ATTR(XML, ATTR, NAME)
#define LAYOUT_CONTROL_PARSE_STRING_ATTR(ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_STRING_ATTR((*resourceItem), ATTR, NAME)

#define PRIV_LAYOUT_CONTROL_GENERATE_STRING_ATTR(VAR, SETFUNC, ARG_FORMAT) \
	if (VAR.flagDefined) { \
		String _accessString; \
		if (!(_getStringAccessString(resource->name, VAR, _accessString))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT, _accessString) \
	}
#define LAYOUT_CONTROL_GENERATE_STRING_ATTR(VAR, SETFUNC, ...) PRIV_LAYOUT_CONTROL_GENERATE_STRING_ATTR(VAR, SETFUNC, __VA_ARGS__ "%s")
#define LAYOUT_CONTROL_GENERATE_STRING_ATTR_UPDATE(VAR, SETFUNC, ...) PRIV_LAYOUT_CONTROL_GENERATE_STRING_ATTR(VAR, SETFUNC, __VA_ARGS__ "%s, slib::UIUpdateMode::Init")

#define LAYOUT_CONTROL_SIMULATE_STRING_ATTR(VAR, SETFUNC, ...) \
	if (!flagOnLayout && VAR.flagDefined) { \
		String _stringValue; \
		if (!(_getStringValue(resource->name, VAR, _stringValue))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ _stringValue); \
	}
#define LAYOUT_CONTROL_SIMULATE_STRING_ATTR_UPDATE(VAR, SETFUNC, ...) \
	if (!flagOnLayout && VAR.flagDefined) { \
		String _stringValue; \
		if (!(_getStringValue(resource->name, VAR, _stringValue))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ _stringValue, UIUpdateMode::Init); \
	}

#define LAYOUT_CONTROL_STRING_ATTR(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_STRING_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_STRING_ATTR(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_STRING_ATTR(attr->NAME, SETFUNC) \
	}
#define LAYOUT_CONTROL_STRING_ATTR_UPDATE(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_STRING_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_STRING_ATTR_UPDATE(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_STRING_ATTR_UPDATE(attr->NAME, SETFUNC) \
	}

#define LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(XML, ATTR, NAME) \
	String str_##NAME = XML.getXmlAttribute(#NAME); \
	if (!(ATTR NAME.parse(str_##NAME, this, XML.element))) { \
		LOG_ERROR_LAYOUT_CONTROL_XML_ATTR(XML, NAME) \
		return sl_false; \
	}
#define LAYOUT_CONTROL_PARSE_DRAWABLE_ATTR(ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR((*resourceItem), ATTR, NAME)

#define LAYOUT_CONTROL_GENERATE_DRAWABLE_ATTR(VAR, SETFUNC, ...) \
	if (VAR.flagDefined) { \
		String _accessString; \
		if (!(_getDrawableAccessString(resource->name, VAR, _accessString))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, __VA_ARGS__ "%s, slib::UIUpdateMode::Init", _accessString) \
	}

#define LAYOUT_CONTROL_SIMULATE_DRAWABLE_ATTR(VAR, SETFUNC, ...) \
	if (flagOnLayout && VAR.flagDefined) { \
		Ref<Drawable> _drawableValue; \
		if (!(_getDrawableValue(resource->name, VAR, _drawableValue))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ _drawableValue); \
	}

#define LAYOUT_CONTROL_DRAWABLE_ATTR(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_DRAWABLE_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_DRAWABLE_ATTR(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_DRAWABLE_ATTR(attr->NAME, SETFUNC) \
	}

#define LAYOUT_CONTROL_PARSE_XML_COLOR_ATTR(XML, ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_REFERING_ATTR(XML, ATTR, NAME)
#define LAYOUT_CONTROL_PARSE_COLOR_ATTR(ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_COLOR_ATTR((*resourceItem), ATTR, NAME)

#define PRIV_LAYOUT_CONTROL_GENERATE_COLOR_ATTR(VAR, SETFUNC, ARG_FORMAT) \
	if (VAR.flagDefined) { \
		String _accessString; \
		if (!(_getColorAccessString(resource->name, VAR, _accessString))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, ARG_FORMAT, _accessString) \
	}
#define LAYOUT_CONTROL_GENERATE_COLOR_ATTR(VAR, SETFUNC, ...) PRIV_LAYOUT_CONTROL_GENERATE_COLOR_ATTR(VAR, SETFUNC, __VA_ARGS__ "%s")
#define LAYOUT_CONTROL_GENERATE_COLOR_ATTR_UPDATE(VAR, SETFUNC, ...) PRIV_LAYOUT_CONTROL_GENERATE_COLOR_ATTR(VAR, SETFUNC, __VA_ARGS__ "%s, slib::UIUpdateMode::Init")

#define LAYOUT_CONTROL_SIMULATE_COLOR_ATTR(VAR, SETFUNC, ...) \
	if (!flagOnLayout && VAR.flagDefined) { \
		Color _colorValue; \
		if (!(_getColorValue(resource->name, VAR, _colorValue))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ _colorValue); \
	}
#define LAYOUT_CONTROL_SIMULATE_COLOR_ATTR_UPDATE(VAR, SETFUNC, ...) \
	if (!flagOnLayout && VAR.flagDefined) { \
		Color _colorValue; \
		if (!(_getColorValue(resource->name, VAR, _colorValue))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ _colorValue, UIUpdateMode::Init); \
	}

#define LAYOUT_CONTROL_COLOR_ATTR(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_COLOR_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_COLOR_ATTR(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_COLOR_ATTR(attr->NAME, SETFUNC) \
	}
#define LAYOUT_CONTROL_COLOR_ATTR_UPDATE(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_COLOR_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_COLOR_ATTR_UPDATE(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_COLOR_ATTR_UPDATE(attr->NAME, SETFUNC) \
	}

#define LAYOUT_CONTROL_PARSE_XML_FONT_ATTR(XML, ATTR, NAME) \
	if (!(ATTR NAME.parse(&XML, #NAME, this, !(params->parentResourceItem)))) { \
		return sl_false; \
	}
#define LAYOUT_CONTROL_PARSE_FONT_ATTR(ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_FONT_ATTR(*resourceItem, ATTR, NAME)

#define LAYOUT_CONTROL_GENERATE_FONT_ATTR(VAR, SETFUNC, ...) \
	if (VAR.flagDefined) { \
		String _accessString; \
		if (!(_getFontAccessString(resource->name, VAR, _accessString))) { \
			return sl_false; \
		} \
		if (VAR.size.isNeededOnLayoutFunction()) { \
			LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, __VA_ARGS__ "%s, slib::UIUpdateMode::UpdateLayout", _accessString) \
		} else { \
			LAYOUT_CONTROL_GENERATE(SETFUNC, __VA_ARGS__ "%s, slib::UIUpdateMode::Init", _accessString) \
		} \
	}

#define LAYOUT_CONTROL_SIMULATE_FONT_ATTR(VAR, SETFUNC, ...) \
	if (flagOnLayout && VAR.flagDefined) { \
		Ref<Font> _fontValue; \
		if (!(_getFontValue(resource->name, VAR, _fontValue))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ _fontValue); \
	}
	
#define LAYOUT_CONTROL_FONT_ATTR(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_FONT_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_FONT_ATTR(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_FONT_ATTR(attr->NAME, SETFUNC) \
	}


#define LAYOUT_CONTROL_PARSE_XML_MENU_ATTR(XML, ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_REFERING_ATTR(XML, ATTR, NAME)
#define LAYOUT_CONTROL_PARSE_MENU_ATTR(ATTR, NAME) LAYOUT_CONTROL_PARSE_XML_MENU_ATTR((*resourceItem), ATTR, NAME)

#define LAYOUT_CONTROL_GENERATE_MENU_ATTR(VAR, SETFUNC, ...) \
	if (VAR.flagDefined) { \
		String _accessString; \
		if (!(_getMenuAccessString(resource->name, VAR, _accessString))) { \
			return sl_false; \
		} \
		LAYOUT_CONTROL_GENERATE(SETFUNC, __VA_ARGS__ "%s", _accessString) \
	}

#define LAYOUT_CONTROL_SIMULATE_MENU_ATTR(VAR, SETFUNC, ...) \
	if (!flagOnLayout && VAR.flagDefined) { \
		Ref<Menu> _menuValue; \
		if (!(_getMenuValue(resource->name, VAR, _menuValue))) { \
			return sl_false; \
		} \
		view->SETFUNC(__VA_ARGS__ _menuValue); \
	}

#define LAYOUT_CONTROL_MENU_ATTR(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_MENU_ATTR(attr->, NAME) \
	} else if (op == OP_GENERATE_CPP) { \
		LAYOUT_CONTROL_GENERATE_MENU_ATTR(attr->NAME, SETFUNC) \
	} else if (op == OP_SIMULATE) { \
		LAYOUT_CONTROL_SIMULATE_MENU_ATTR(attr->NAME, SETFUNC) \
	}

#define LAYOUT_CONTROL_MARGIN_ATTR_UPDATE(NAME, SETFUNC) \
	if (op == OP_PARSE) { \
		LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(attr->, NAME, checkAll) \
		if (params->parentResourceItem) { \
			if (!(attr->NAME.checkMargin())) { \
				LOG_ERROR_LAYOUT_CONTROL_ATTR(NAME); \
				return sl_false; \
			} \
		} else { \
			if (!(attr->NAME.checkForRootViewMargin())) { \
				LOG_ERROR_LAYOUT_CONTROL_ATTR(NAME); \
				return sl_false; \
			} \
		} \
	} else if (op == OP_GENERATE_CPP) { \
		if (attr->NAME.flagDefined) { \
			if (attr->NAME.isNeededOnLayoutFunction()) { \
				if (attr->NAME.unit == SAppDimensionValue::WEIGHT) { \
					LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC##Weight, "%ff", attr->NAME.amount) \
				} else { \
					LAYOUT_CONTROL_GENERATE_LAYOUT(SETFUNC, "%s", attr->NAME.getAccessString()) \
				} \
			} else { \
				if (attr->NAME.unit == SAppDimensionValue::WEIGHT) { \
					LAYOUT_CONTROL_GENERATE(SETFUNC##Weight, "%ff, slib::UIUpdateMode::Init", attr->NAME.amount) \
				} else { \
					LAYOUT_CONTROL_GENERATE(SETFUNC, "%s, slib::UIUpdateMode::Init", attr->NAME.getAccessString()) \
				} \
			} \
		} \
	} else if (op == OP_SIMULATE) { \
		if (flagOnLayout) { \
			if (attr->NAME.flagDefined) { \
				if (attr->NAME.unit == SAppDimensionValue::WEIGHT) { \
					view->SETFUNC##Weight(attr->NAME.amount); \
				} else { \
					view->SETFUNC(_getDimensionValue(attr->NAME)); \
				} \
			} \
		} \
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
		if (resourceItem->attrs->isNotRequiredNative(sl_false) || attr->isNotRequiredNative()) { \
			if (!(resourceItem->attrs->nativeWidget.flagDefined)) { \
				LAYOUT_CONTROL_GENERATE(setCreatingNativeWidget, "sl_false") \
			} \
		} \
	} else if (op == OP_SIMULATE) { \
		if (!flagOnLayout) { \
			if (resourceItem->attrs->isNotRequiredNative(sl_false) || attr->isNotRequiredNative()) { \
				if (!(resourceItem->attrs->nativeWidget.flagDefined)) { \
					view->setCreatingNativeWidget(sl_false); \
				} \
			} \
		} \
	}

#define LAYOUT_CONTROL_SET_NATIVE_WIDGET_CHECK_BACKGROUND_COLOR \
	if (op == OP_GENERATE_CPP) { \
		if (resourceItem->attrs->isNotRequiredNative(sl_true) || attr->isNotRequiredNative()) { \
			if (!(resourceItem->attrs->nativeWidget.flagDefined)) { \
				LAYOUT_CONTROL_GENERATE(setCreatingNativeWidget, "sl_false") \
			} \
		} \
	} else if (op == OP_SIMULATE) { \
		if (!flagOnLayout) { \
			if (resourceItem->attrs->isNotRequiredNative(sl_true) || attr->isNotRequiredNative()) { \
				if (!(resourceItem->attrs->nativeWidget.flagDefined)) { \
					view->setCreatingNativeWidget(sl_false); \
				} \
			} \
		} \
	}


	BEGIN_PROCESS_LAYOUT_CONTROL(View, View)
	{
		sl_bool flagView = params->parentResourceItem != sl_null || params->resource->layoutType != SAppLayoutType::Window;
		sl_bool flagRoot = params->parentResourceItem == sl_null;

		if (flagView) {
			LAYOUT_CONTROL_STRING_ATTR(id, setId)
			if (op == OP_PARSE) {
				if (flagRoot) {
					LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(width, setWidth, checkForRootViewSize)
					LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(height, setHeight, checkForRootViewSize)
				} else {
					LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(width, setWidth, checkSize)
					LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(height, setHeight, checkSize)
				}
			} else if (op == OP_GENERATE_CPP) {
				if (attr->width.flagDefined) {
					if (attr->width.unit == SAppDimensionValue::FILL) {
						params->sbDefineInit->add(String::format("%s%s->setWidthFilling(%ff, slib::UIUpdateMode::Init);%n", strTab, name, attr->width.amount));
					} else if (attr->width.unit == SAppDimensionValue::WRAP) {
						params->sbDefineInit->add(String::format("%s%s->setWidthWrapping(slib::UIUpdateMode::Init);%n", strTab, name));
					} else if (attr->width.unit == SAppDimensionValue::WEIGHT) {
						params->sbDefineInit->add(String::format("%s%s->setWidthWeight(%ff, slib::UIUpdateMode::Init);%n", strTab, name, attr->width.amount));
					} else {
						LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(width, setWidth, checkSize)
					}
				}
				if (attr->height.flagDefined) {
					if (attr->height.unit == SAppDimensionValue::FILL) {
						params->sbDefineInit->add(String::format("%s%s->setHeightFilling(%ff, slib::UIUpdateMode::Init);%n", strTab, name, attr->height.amount));
					} else if (attr->height.unit == SAppDimensionValue::WRAP) {
						params->sbDefineInit->add(String::format("%s%s->setHeightWrapping(slib::UIUpdateMode::Init);%n", strTab, name));
					} else if (attr->height.unit == SAppDimensionValue::WEIGHT) {
						params->sbDefineInit->add(String::format("%s%s->setHeightWeight(%ff, slib::UIUpdateMode::Init);%n", strTab, name, attr->height.amount));
					} else {
						LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(height, setHeight, checkSize)
					}
				}
			} else if (op == OP_SIMULATE) {
				if (attr->width.flagDefined) {
					if (attr->width.unit == SAppDimensionValue::FILL) {
						if (!flagOnLayout) {
							view->setWidthFilling(attr->width.amount, UIUpdateMode::Init);
						}
					} else if (attr->width.unit == SAppDimensionValue::WRAP) {
						if (!flagOnLayout) {
							view->setWidthWrapping(UIUpdateMode::Init);
						}
					} else if (attr->width.unit == SAppDimensionValue::WEIGHT) {
						if (!flagOnLayout) {
							view->setWidthWeight(attr->width.amount, UIUpdateMode::Init);
						}
					} else {
						LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(width, setWidth, checkSize)
					}
				}
				if (attr->height.flagDefined) {
					if (attr->height.unit == SAppDimensionValue::FILL) {
						if (!flagOnLayout) {
							view->setHeightFilling(attr->height.amount, UIUpdateMode::Init);
						}
					} else if (attr->height.unit == SAppDimensionValue::WRAP) {
						if (!flagOnLayout) {
							view->setHeightWrapping(UIUpdateMode::Init);
						}
					} else if (attr->height.unit == SAppDimensionValue::WEIGHT) {
						if (!flagOnLayout) {
							view->setHeightWeight(attr->height.amount, UIUpdateMode::Init);
						}
					} else {
						LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(height, setHeight, checkSize)
					}
				}
			}

			if (flagRoot) {
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(left, setLeft, checkForRootViewPosition)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(top, setTop, checkForRootViewPosition)
			} else {
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(left, setLeft, checkPosition)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(top, setTop, checkPosition)
			}

			if (op == OP_PARSE) {

				attr->leftMode = PositionMode::Free;
				attr->topMode = PositionMode::Free;
				attr->rightMode = PositionMode::Free;
				attr->bottomMode = PositionMode::Free;

				SAppAlignLayoutValue alignLeft;
				LAYOUT_CONTROL_PARSE_ATTR(, alignLeft)
				if (alignLeft.flagDefined) {
					if (alignLeft.flagAlignParent) {
						attr->leftMode = PositionMode::ParentEdge;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(alignLeft.referingView)) {
							attr->leftMode = PositionMode::OtherStart;
							attr->leftReferingView = alignLeft.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR(alignLeft)
							return sl_false;
						}
					}
				}
				SAppNameValue toRightOf;
				LAYOUT_CONTROL_PARSE_ATTR(, toRightOf)
				if (toRightOf.flagDefined) {
					if (!flagRoot && params->resource->itemsByName.find(toRightOf.value)) {
						attr->leftMode = PositionMode::OtherEnd;
						attr->leftReferingView = toRightOf.value;
					} else {
						LOG_ERROR_LAYOUT_CONTROL_ATTR(toRightOf)
						return sl_false;
					}
				}
				SAppAlignLayoutValue alignTop;
				LAYOUT_CONTROL_PARSE_ATTR(, alignTop)
				if (alignTop.flagDefined) {
					if (alignTop.flagAlignParent) {
						attr->topMode = PositionMode::ParentEdge;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(alignTop.referingView)) {
							attr->topMode = PositionMode::OtherStart;
							attr->topReferingView = alignTop.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR(alignTop)
							return sl_false;
						}
					}
				}
				SAppNameValue below;
				LAYOUT_CONTROL_PARSE_ATTR(, below)
				if (below.flagDefined) {
					if (!flagRoot && params->resource->itemsByName.find(below.value)) {
						attr->topMode = PositionMode::OtherEnd;
						attr->topReferingView = below.value;
					} else {
						LOG_ERROR_LAYOUT_CONTROL_ATTR(below)
						return sl_false;
					}
				}
				SAppAlignLayoutValue alignRight;
				LAYOUT_CONTROL_PARSE_ATTR(, alignRight)
				if (alignRight.flagDefined) {
					if (alignRight.flagAlignParent) {
						attr->rightMode = PositionMode::ParentEdge;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(alignRight.referingView)) {
							attr->rightMode = PositionMode::OtherEnd;
							attr->rightReferingView = alignRight.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR(alignRight)
							return sl_false;
						}
					}
				}
				SAppNameValue toLeftOf;
				LAYOUT_CONTROL_PARSE_ATTR(, toLeftOf)
				if (toLeftOf.flagDefined) {
					if (!flagRoot && params->resource->itemsByName.find(toLeftOf.value)) {
						attr->rightMode = PositionMode::OtherStart;
						attr->rightReferingView = toLeftOf.value;
					} else {
						LOG_ERROR_LAYOUT_CONTROL_ATTR(toLeftOf)
						return sl_false;
					}
				}
				SAppAlignLayoutValue alignBottom;
				LAYOUT_CONTROL_PARSE_ATTR(, alignBottom)
				if (alignBottom.flagDefined) {
					if (alignBottom.flagAlignParent) {
						attr->bottomMode = PositionMode::ParentEdge;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(alignBottom.referingView)) {
							attr->bottomMode = PositionMode::OtherEnd;
							attr->bottomReferingView = alignBottom.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR(alignBottom)
							return sl_false;
						}
					}
				}
				SAppNameValue above;
				LAYOUT_CONTROL_PARSE_ATTR(, above)
				if (above.flagDefined) {
					if (!flagRoot && params->resource->itemsByName.find(above.value)) {
						attr->bottomMode = PositionMode::OtherStart;
						attr->bottomReferingView = above.value;
					} else {
						LOG_ERROR_LAYOUT_CONTROL_ATTR(above)
						return sl_false;
					}
				}
				SAppAlignLayoutValue centerHorizontal;
				LAYOUT_CONTROL_PARSE_ATTR(, centerHorizontal)
				if (centerHorizontal.flagDefined) {
					if (centerHorizontal.flagAlignParent) {
						attr->leftMode = PositionMode::CenterInParent;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(centerHorizontal.referingView)) {
							attr->leftMode = PositionMode::CenterInOther;
							attr->leftReferingView = centerHorizontal.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR(centerHorizontal)
							return sl_false;
						}
					}
				}
				SAppAlignLayoutValue centerVertical;
				LAYOUT_CONTROL_PARSE_ATTR(, centerVertical)
				if (centerVertical.flagDefined) {
					if (centerVertical.flagAlignParent) {
						attr->topMode = PositionMode::CenterInParent;
					} else {
						if (!flagRoot && params->resource->itemsByName.find(centerVertical.referingView)) {
							attr->topMode = PositionMode::CenterInOther;
							attr->topReferingView = centerVertical.referingView;
						} else {
							LOG_ERROR_LAYOUT_CONTROL_ATTR(centerVertical)
							return sl_false;
						}
					}
				}
				SAppAlignLayoutValue alignCenter;
				LAYOUT_CONTROL_PARSE_ATTR(, alignCenter)
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
							LOG_ERROR_LAYOUT_CONTROL_ATTR(alignCenter)
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
					params->sbDefineInit->add(String::format("%s%s->setCenterHorizontal(slib::UIUpdateMode::Init);%n", strTab, name));
				} else if (attr->leftMode == PositionMode::CenterInOther) {
					params->sbDefineInit->add(String::format("%s%s->setAlignCenterHorizontal(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->leftReferingView));
				} else {
					if (attr->leftMode == PositionMode::ParentEdge) {
						params->sbDefineInit->add(String::format("%s%s->setAlignParentLeft(slib::UIUpdateMode::Init);%n", strTab, name));
					} else if (attr->leftMode == PositionMode::OtherStart) {
						params->sbDefineInit->add(String::format("%s%s->setAlignLeft(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->leftReferingView));
					} else if (attr->leftMode == PositionMode::OtherEnd) {
						params->sbDefineInit->add(String::format("%s%s->setRightOf(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->leftReferingView));
					}
					if (attr->rightMode == PositionMode::ParentEdge) {
						params->sbDefineInit->add(String::format("%s%s->setAlignParentRight(slib::UIUpdateMode::Init);%n", strTab, name));
					} else if (attr->rightMode == PositionMode::OtherStart) {
						params->sbDefineInit->add(String::format("%s%s->setLeftOf(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->rightReferingView));
					} else if (attr->rightMode == PositionMode::OtherEnd) {
						params->sbDefineInit->add(String::format("%s%s->setAlignRight(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->rightReferingView));
					}
				}

				if (attr->topMode == PositionMode::CenterInParent) {
					params->sbDefineInit->add(String::format("%s%s->setCenterVertical(slib::UIUpdateMode::Init);%n", strTab, name));
				} else if (attr->topMode == PositionMode::CenterInOther) {
					params->sbDefineInit->add(String::format("%s%s->setAlignCenterVertical(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->topReferingView));
				} else {
					if (attr->topMode == PositionMode::ParentEdge) {
						params->sbDefineInit->add(String::format("%s%s->setAlignParentTop(slib::UIUpdateMode::Init);%n", strTab, name));
					} else if (attr->topMode == PositionMode::OtherStart) {
						params->sbDefineInit->add(String::format("%s%s->setAlignTop(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->topReferingView));
					} else if (attr->topMode == PositionMode::OtherEnd) {
						params->sbDefineInit->add(String::format("%s%s->setBelow(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->topReferingView));
					}
					if (attr->bottomMode == PositionMode::ParentEdge) {
						params->sbDefineInit->add(String::format("%s%s->setAlignParentBottom(slib::UIUpdateMode::Init);%n", strTab, name));
					} else if (attr->bottomMode == PositionMode::OtherStart) {
						params->sbDefineInit->add(String::format("%s%s->setAbove(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->bottomReferingView));
					} else if (attr->bottomMode == PositionMode::OtherEnd) {
						params->sbDefineInit->add(String::format("%s%s->setAlignBottom(%s, slib::UIUpdateMode::Init);%n", strTab, name, attr->bottomReferingView));
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

			if (flagRoot) {
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(minWidth, setMinimumWidth, checkForRootViewScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(maxWidth, setMaximumWidth, checkForRootViewScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(minHeight, setMinimumHeight, checkForRootViewScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(maxHeight, setMaximumHeight, checkForRootViewScalarSize)
			} else {
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(minWidth, setMinimumWidth, checkScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(maxWidth, setMaximumWidth, checkScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(minHeight, setMinimumHeight, checkScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(maxHeight, setMaximumHeight, checkScalarSize)
			}

			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(aspectRatio, setAspectRatio)

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
							params->sbDefineInit->add(String::format("%s%s->setAspectRatioMode(slib::AspectRatioMode::AdjustHeight, slib::UIUpdateMode::Init);%n", strTab, name));
						}
					} else {
						if (attr->height.flagDefined) {
							params->sbDefineInit->add(String::format("%s%s->setAspectRatioMode(slib::AspectRatioMode::AdjustWidth, slib::UIUpdateMode::Init);%n", strTab, name));
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

			LAYOUT_CONTROL_MARGIN_ATTR_UPDATE(marginLeft, setMarginLeft)
			LAYOUT_CONTROL_MARGIN_ATTR_UPDATE(marginTop, setMarginTop)
			LAYOUT_CONTROL_MARGIN_ATTR_UPDATE(marginRight, setMarginRight)
			LAYOUT_CONTROL_MARGIN_ATTR_UPDATE(marginBottom, setMarginBottom)
			if (op == OP_PARSE) {
				SAppDimensionValue margin;
				LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, margin, checkAll)
				if (flagRoot) {
					if (!(margin.checkForRootViewMargin())) {
						LOG_ERROR_LAYOUT_CONTROL_ATTR(margin)
						return sl_false;
					}
				} else {
					if (!(margin.checkMargin())) {
						LOG_ERROR_LAYOUT_CONTROL_ATTR(margin)
						return sl_false;
					}
				}
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

		LAYOUT_CONTROL_MARGIN_ATTR_UPDATE(paddingLeft, setPaddingLeft)
		LAYOUT_CONTROL_MARGIN_ATTR_UPDATE(paddingTop, setPaddingTop)
		LAYOUT_CONTROL_MARGIN_ATTR_UPDATE(paddingRight, setPaddingRight)
		LAYOUT_CONTROL_MARGIN_ATTR_UPDATE(paddingBottom, setPaddingBottom)
		if (op == OP_PARSE) {
			SAppDimensionValue padding;
			LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, padding, checkAll)
			if (flagRoot) {
				if (!(padding.checkForRootViewMargin())) {
					LOG_ERROR_LAYOUT_CONTROL_ATTR(padding)
					return sl_false;
				}
			} else {
				if (!(padding.checkMargin())) {
					LOG_ERROR_LAYOUT_CONTROL_ATTR(padding)
					return sl_false;
				}
			}
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

		LAYOUT_CONTROL_DRAWABLE_ATTR(background, setBackground)
		LAYOUT_CONTROL_DRAWABLE_ATTR(pressedBackground, setPressedBackground)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverBackground, setHoverBackground)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(backgroundScale, setBackgroundScaleMode)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(backgroundAlign, setBackgroundAlignment)

		if (flagView) {
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(border, setBorder)
			if (op == OP_PARSE) {
				if (flagRoot) {
					LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(borderWidth, setBorderWidth, checkForRootViewScalarSize)
				} else {
					LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(borderWidth, setBorderWidth, checkScalarSize)
				}
				LAYOUT_CONTROL_COLOR_ATTR_UPDATE(borderColor, setBorderColor)
				LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(borderStyle, setBorderStyle)
				if (!(attr->borderWidth.flagDefined)) {
					if (attr->borderColor.flagDefined || attr->borderStyle.flagDefined) {
						attr->borderWidth.flagDefined = sl_true;
						attr->borderWidth.amount = 1;
						attr->borderWidth.unit = SAppDimensionValue::PX;
					}
				}
			} else if (op == OP_GENERATE_CPP){
				if (attr->borderWidth.flagDefined) {
					if (Math::isAlmostZero(attr->borderWidth.amount)) {
						params->sbDefineInit->add(String::format("%s%s->setBorder(sl_null, slib::UIUpdateMode::Init);%n", strTab, name));
					} else {
						if (attr->borderColor.flagDefined && attr->borderStyle.flagDefined) {
							String strBorderColor;
							if (_getColorAccessString(resource->name, attr->borderColor, strBorderColor)) {
								if (attr->borderWidth.isNeededOnLayoutFunction()) {
									params->sbDefineLayout->add(String::format("%s%s->setBorder(slib::Pen::create(%s, %s, %s));%n", strTab, name, attr->borderStyle.getAccessString(), attr->borderWidth.getAccessString(), strBorderColor));
								} else {
									params->sbDefineInit->add(String::format("%s%s->setBorder(slib::Pen::create(%s, %s, %s), slib::UIUpdateMode::Init);%n", strTab, name, attr->borderStyle.getAccessString(), attr->borderWidth.getAccessString(), strBorderColor));
								}
							} else {
								return sl_false;
							}
						} else {
							LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(borderWidth, setBorderWidth, checkScalarSize)
							LAYOUT_CONTROL_COLOR_ATTR_UPDATE(borderColor, setBorderColor)
							LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(borderStyle, setBorderStyle)
						}
					}
				} else {
					LAYOUT_CONTROL_COLOR_ATTR_UPDATE(borderColor, setBorderColor)
					LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(borderStyle, setBorderStyle)
				}
			} else if (op == OP_SIMULATE) {
				if (attr->borderWidth.flagDefined) {
					if (Math::isAlmostZero(attr->borderWidth.amount)) {
						if (!flagOnLayout) {
							view->setBorder(sl_null, UIUpdateMode::Init);
						}
					} else {
						if (attr->borderColor.flagDefined && attr->borderStyle.flagDefined) {
							if (flagOnLayout) {
								Color borderColor;
								if (_getColorValue(resource->name, attr->borderColor, borderColor)) {
									view->setBorder(Pen::create(attr->borderStyle.value, _getDimensionValue(attr->borderWidth), borderColor), UIUpdateMode::None);
								} else {
									return sl_false;
								}
							}
						} else {
							LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(borderWidth, setBorderWidth, checkScalarSize)
							LAYOUT_CONTROL_COLOR_ATTR_UPDATE(borderColor, setBorderColor)
							LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(borderStyle, setBorderStyle)
						}
					}
				} else {
					LAYOUT_CONTROL_COLOR_ATTR_UPDATE(borderColor, setBorderColor)
					LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(borderStyle, setBorderStyle)
				}
			}
		}

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(drawing, setDrawing)

		{
			if (flagRoot) {
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(boundRadius, setBoundRadius, checkForRootViewScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(boundRadiusX, setBoundRadiusX, checkForRootViewScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(boundRadiusY, setBoundRadiusY, checkForRootViewScalarSize)
			} else {
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(boundRadius, setBoundRadius, checkScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(boundRadiusX, setBoundRadiusX, checkScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(boundRadiusY, setBoundRadiusY, checkScalarSize)
			}
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(boundShape, setBoundShape)
			if (flagRoot) {
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(contentRadius, setContentRadius, checkForRootViewScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(contentRadiusX, setContentRadiusX, checkForRootViewScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(contentRadiusY, setContentRadiusY, checkForRootViewScalarSize)
			} else {
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(contentRadius, setContentRadius, checkScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(contentRadiusX, setContentRadiusX, checkScalarSize)
				LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(contentRadiusY, setContentRadiusY, checkScalarSize)
			}
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(contentShape, setContentShape)
		}

		LAYOUT_CONTROL_FONT_ATTR(font, setFont)
		if (op == OP_PARSE) {
			if (!flagRoot) {
				attr->font.inheritFrom(params->parentResourceItem->attrs->font);
			}
		}

		if (flagView) {
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(alpha, setAlpha)
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(antiAlias, setAntiAlias)
		}

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(opaque, setOpaque)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(layer, setLayer)

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(shadowOpacity, setShadowOpacity)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(shadowRadius, setShadowRadius, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(shadowOffsetX, setShadowOffsetX, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(shadowOffsetY, setShadowOffsetY, checkScalarSize)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(shadowColor, setShadowColor)

		if (op == OP_PARSE) {
			LAYOUT_CONTROL_PARSE_ATTR(attr->, scrolling)
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
				params->sbDefineInit->add(String::format("%s%s->setHorizontalScrolling(%s);%n", strTab, name, (attr->scrolling.horizontal?"sl_true":"sl_false")));
				params->sbDefineInit->add(String::format("%s%s->setVerticalScrolling(%s);%n", strTab, name, (attr->scrolling.vertical?"sl_true":"sl_false")));
			}
		} else if (op == OP_SIMULATE) {
			if (!flagOnLayout) {
				if (attr->scrolling.flagDefined) {
					view->setHorizontalScrolling(attr->scrolling.horizontal?sl_true:sl_false);
					view->setVerticalScrolling(attr->scrolling.vertical?sl_true:sl_false);
				}
			}
		}

		LAYOUT_CONTROL_GENERIC_ATTR(paging, setPaging)
		LAYOUT_CONTROL_DIMENSION_ATTR(pageWidth, setPageWidth, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR(pageHeight, setPageHeight, checkScalarSize)

		if (op == OP_PARSE) {
			LAYOUT_CONTROL_PARSE_ATTR(attr->, scrollBars)
		} else if (op == OP_GENERATE_CPP) {
			if (attr->scrollBars.flagDefined) {
				params->sbDefineInit->add(String::format("%s%s->setScrollBarsVisible(%s, %s, slib::UIUpdateMode::Init);%n", strTab, name, attr->scrollBars.horizontalScrollBar ? "sl_true" : "sl_false", attr->scrollBars.verticalScrollBar ? "sl_true" : "sl_false"));
			}
		} else if (op == OP_SIMULATE) {
			if (!flagOnLayout) {
				if (attr->scrollBars.flagDefined) {
					view->setScrollBarsVisible(attr->scrollBars.horizontalScrollBar, attr->scrollBars.verticalScrollBar, UIUpdateMode::Init);
				}
			}
		}
		LAYOUT_CONTROL_GENERIC_ATTR(scrollingByMouse, setContentScrollingByMouse)
		LAYOUT_CONTROL_GENERIC_ATTR(scrollingByTouch, setContentScrollingByTouch)
		LAYOUT_CONTROL_GENERIC_ATTR(scrollingByMouseWheel, setContentScrollingByMouseWheel)
		LAYOUT_CONTROL_GENERIC_ATTR(scrollingByKeyboard, setContentScrollingByKeyboard)
		LAYOUT_CONTROL_GENERIC_ATTR(autoHideScrollBar, setAutoHideScrollBar)

		LAYOUT_CONTROL_GENERIC_ATTR(focusable, setFocusable)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(focus, setFocus)
		LAYOUT_CONTROL_GENERIC_ATTR(hitTest, setHitTestable)
		LAYOUT_CONTROL_GENERIC_ATTR(touchMultipleChildren, setTouchMultipleChildren)
		if (flagView) {
			LAYOUT_CONTROL_GENERIC_ATTR(tabStop, setTabStopEnabled)
			LAYOUT_CONTROL_GENERIC_ATTR(cursor, setCursor)
			LAYOUT_CONTROL_STRING_ATTR(toolTip, setToolTip)
		}
		if (!flagRoot) {
			if (op == OP_PARSE) {
				attr->nextTabStop = resourceItem->getXmlAttribute("nextTabStop");
				attr->previousTabStop = resourceItem->getXmlAttribute("previousTabStop");
			} else if (op == OP_GENERATE_CPP) {
				if (params->resource->itemsByName.find(attr->nextTabStop)) {
					params->sbDefineInit->add(String::format("%s%s->setNextTabStop(%s);%n", strTab, name, attr->nextTabStop));
				}
				if (params->resource->itemsByName.find(attr->previousTabStop)) {
					params->sbDefineInit->add(String::format("%s%s->setPreviousTabStop(%s);%n", strTab, name, attr->previousTabStop));
				}
			} else if (op == OP_SIMULATE) {
				if (!flagOnLayout) {
					view->setNextTabStop(params->simulator->getViewByName(attr->nextTabStop));
					view->setPreviousTabStop(params->simulator->getViewByName(attr->previousTabStop));
				}
			}
		}

		if (flagView) {
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(visibility, setVisibility)
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(visible, setVisible)
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(enabled, setEnabled)
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(clipping, setClipping)
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
			LAYOUT_CONTROL_GENERIC_ATTR(instance, setCreatingInstance)
			LAYOUT_CONTROL_GENERIC_ATTR(nativeWidget, setCreatingNativeWidget)
			LAYOUT_CONTROL_GENERIC_ATTR(nativeLayer, setCreatingNativeLayer)
			LAYOUT_CONTROL_GENERIC_ATTR(largeContent, setCreatingLargeContent)
			LAYOUT_CONTROL_GENERIC_ATTR(emptyContent, setCreatingEmptyContent)
		}
		LAYOUT_CONTROL_GENERIC_ATTR(childInstances, setCreatingChildInstances)

		if (flagView) {
			LAYOUT_CONTROL_GENERIC_ATTR(okCancelEnabled, setOkCancelEnabled)
			LAYOUT_CONTROL_BOOLEAN_ATTR(ok, setOkOnClick)
			LAYOUT_CONTROL_BOOLEAN_ATTR(cancel, setCancelOnClick)
			LAYOUT_CONTROL_GENERIC_ATTR(mnemonicKey, setMnemonicKey)
			LAYOUT_CONTROL_GENERIC_ATTR(keepKeyboard, setKeepKeyboard)
			LAYOUT_CONTROL_GENERIC_ATTR(playSoundOnClick, setPlaySoundOnClick)
			LAYOUT_CONTROL_GENERIC_ATTR(clientEdge, setClientEdge)
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Window, View)
	{
		Window* view = params->window;

		LAYOUT_CONTROL_DIMENSION_ATTR(minWidth, setMinimumWidth, checkForWindow)
		LAYOUT_CONTROL_DIMENSION_ATTR(maxWidth, setMaximumWidth, checkForWindow)
		LAYOUT_CONTROL_DIMENSION_ATTR(minHeight, setMinimumHeight, checkForWindow)
		LAYOUT_CONTROL_DIMENSION_ATTR(maxHeight, setMaximumHeight, checkForWindow)
		LAYOUT_CONTROL_GENERIC_ATTR(aspectRatio, setAspectRatio)
		LAYOUT_CONTROL_GENERIC_ATTR(minAspectRatio, setMinimumAspectRatio)
		LAYOUT_CONTROL_GENERIC_ATTR(maxAspectRatio, setMaximumAspectRatio)

		LAYOUT_CONTROL_GENERIC_ATTR(minimized, setMinimized)
		LAYOUT_CONTROL_GENERIC_ATTR(maximized, setMaximized)
		LAYOUT_CONTROL_GENERIC_ATTR(visible, setVisible)
		LAYOUT_CONTROL_GENERIC_ATTR(alwaysOnTop, setAlwaysOnTop)
		LAYOUT_CONTROL_GENERIC_ATTR(closeButton, setCloseButtonEnabled)
		LAYOUT_CONTROL_GENERIC_ATTR(minimizeButton, setMinimizeButtonEnabled)
		LAYOUT_CONTROL_GENERIC_ATTR(maximizeButton, setMaximizeButtonEnabled)
		LAYOUT_CONTROL_GENERIC_ATTR(fullScreenButton, setFullScreenButtonEnabled)
		LAYOUT_CONTROL_GENERIC_ATTR(resizable, setResizable)
		LAYOUT_CONTROL_GENERIC_ATTR(layered, setLayered)
		LAYOUT_CONTROL_GENERIC_ATTR(alpha, setAlpha)
		LAYOUT_CONTROL_GENERIC_ATTR(transparent, setTransparent)
		LAYOUT_CONTROL_COLOR_ATTR(backgroundColor, setBackgroundColor)

		LAYOUT_CONTROL_GENERIC_ATTR(modal, setModal)
		LAYOUT_CONTROL_GENERIC_ATTR(dialog, setDialog)
		LAYOUT_CONTROL_GENERIC_ATTR(borderless, setBorderless)
		LAYOUT_CONTROL_GENERIC_ATTR(titleBar, setTitleBarVisible)
		LAYOUT_CONTROL_GENERIC_ATTR(fullScreen, setFullScreen)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(centerScreen, setCenterScreen)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(marginLeft, setMarginLeft, checkForWindow)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(marginTop, setMarginTop, checkForWindow)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(marginRight, setMarginRight, checkForWindow)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(marginBottom, setMarginBottom, checkForWindow)
		if (op == OP_PARSE) {
			SAppDimensionValue margin;
			LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, margin, checkForWindow)
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

		LAYOUT_CONTROL_MENU_ATTR(menu, setMenu)
		LAYOUT_CONTROL_STRING_ATTR(title, setTitle)
		if (op == OP_PARSE) {
			LAYOUT_CONTROL_DIMENSION_ATTR(left, setLeft, checkForWindow)
			LAYOUT_CONTROL_DIMENSION_ATTR(top, setTop, checkForWindow)
			LAYOUT_CONTROL_DIMENSION_ATTR(width, setClientWidth, checkForWindowSize)
			LAYOUT_CONTROL_DIMENSION_ATTR(height, setClientHeight, checkForWindowSize)
		} else if (op == OP_GENERATE_CPP) {
			LAYOUT_CONTROL_DIMENSION_ATTR(left, setLeft, checkForWindow)
			LAYOUT_CONTROL_DIMENSION_ATTR(top, setTop, checkForWindow)
			if (attr->width.flagDefined) {
				if (attr->width.unit == SAppDimensionValue::WRAP) {
					params->sbDefineInit->add(String::format("%s%s->setWidthWrapping(sl_true, slib::UIUpdateMode::Init);%n", strTab, name));
				} else if (attr->width.unit == SAppDimensionValue::FILL) {
					params->sbDefineInit->add(String::format("%s%s->setWidthFilling(sl_true, slib::UIUpdateMode::Init);%n", strTab, name));
				} else {
					LAYOUT_CONTROL_DIMENSION_ATTR(width, setClientWidth, checkForWindowSize)
				}
			}
			if (attr->height.flagDefined) {
				if (attr->height.unit == SAppDimensionValue::WRAP) {
					params->sbDefineInit->add(String::format("%s%s->setHeightWrapping(sl_true, slib::UIUpdateMode::Init);%n", strTab, name));
				} else if (attr->height.unit == SAppDimensionValue::FILL) {
					params->sbDefineInit->add(String::format("%s%s->setHeightFilling(sl_true, slib::UIUpdateMode::Init);%n", strTab, name));
				} else {
					LAYOUT_CONTROL_DIMENSION_ATTR(height, setClientHeight, checkForWindowSize)
				}
			}
		} else if (op == OP_SIMULATE) {
			if (!flagOnLayout) {
				flagOnLayout = sl_true;
				LAYOUT_CONTROL_DIMENSION_ATTR(left, setLeft, checkForWindow)
				LAYOUT_CONTROL_DIMENSION_ATTR(top, setTop, checkForWindow)
				if (attr->width.flagDefined) {
					if (attr->width.unit == SAppDimensionValue::WRAP) {
						view->setWidthWrapping(sl_true, UIUpdateMode::Init);
					} else if (attr->width.unit == SAppDimensionValue::FILL) {
						view->setWidthFilling(sl_true, UIUpdateMode::Init);
					} else {
						LAYOUT_CONTROL_DIMENSION_ATTR(width, setClientWidth, checkForWindowSize)
					}
				}
				if (attr->height.flagDefined) {
					if (attr->height.unit == SAppDimensionValue::WRAP) {
						view->setHeightWrapping(sl_true, UIUpdateMode::Init);
					} else if (attr->height.unit == SAppDimensionValue::FILL) {
						view->setHeightFilling(sl_true, UIUpdateMode::Init);
					} else {
						LAYOUT_CONTROL_DIMENSION_ATTR(height, setClientHeight, checkForWindowSize)
					}
				}
				flagOnLayout = sl_false;
			}
		}

		params->name = "m_contentView";
		if (!(_processLayoutResourceControl_View(params))) {
			params->resourceItem->attrs->resetLayout();
			return sl_false;
		}

		if (op == OP_PARSE) {
			if (!(attr->backgroundColor.flagDefined)) {
				if (resourceItem->attrs.isNotNull()) {
					if (resourceItem->attrs->background.flagDefined && resourceItem->attrs->background.flagColor) {
						attr->backgroundColor.flagDefined = sl_true;
						attr->backgroundColor.color = resourceItem->attrs->background.color;
						attr->backgroundColor.resourceName = resourceItem->attrs->background.resourceName;
						resourceItem->attrs->background.flagDefined = sl_false;
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
			if (!(resourceItem->attrs->width.flagDefined)) {
				resourceItem->attrs->width.flagDefined = sl_true;
				resourceItem->attrs->width.amount = 1;
				resourceItem->attrs->width.unit = SAppDimensionValue::FILL;
			}
			if (!(resourceItem->attrs->height.flagDefined)) {
				resourceItem->attrs->height.flagDefined = sl_true;
				resourceItem->attrs->height.amount = 1;
				resourceItem->attrs->height.unit = SAppDimensionValue::FILL;
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
				_logError(element, g_str_error_resource_layout_attribute_invalid, "layout", attr->layout);
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
				_logError(element, g_str_error_layout_not_found, attr->layout);
				return sl_false;
			}
			if (layoutImport->layoutType != SAppLayoutType::View && layoutImport->layoutType != SAppLayoutType::Page) {
				_logError(element, g_str_error_layout_is_not_view, attr->layout);
				return sl_false;
			}
		} else if (op == OP_SIMULATE) {
			Ref<SAppLayoutResource> layoutImport;
			m_layouts.get(attr->layout, &layoutImport);
			if (layoutImport.isNull()) {
				_logError(element, g_str_error_layout_not_found, attr->layout);
				return sl_false;
			}
			if (layoutImport->layoutType != SAppLayoutType::View && layoutImport->layoutType != SAppLayoutType::Page) {
				_logError(element, g_str_error_layout_is_not_view, attr->layout);
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

		LAYOUT_CONTROL_STRING_ATTR_UPDATE(text, setText)
		LAYOUT_CONTROL_STRING_ATTR_UPDATE(hyperText, setHyperText)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColor, setTextColor)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(multiLine, setMultiLine)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(ellipsize, setEllipsize)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(lines, setLineCount)
		LAYOUT_CONTROL_GENERIC_ATTR(mnemonic, setMnemonic)

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(defaultButton, setDefaultButton)
		LAYOUT_CONTROL_GENERIC_ATTR(usingFocusedState, setUsingFocusedState)
		LAYOUT_CONTROL_DRAWABLE_ATTR(icon, setIcon)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconWidth, setIconWidth, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconHeight, setIconHeight, checkScalarSize)
		if (op == OP_PARSE) {
			SAppDimensionValue iconSize;
			LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, iconSize, checkScalarSize)
			if (iconSize.flagDefined) {
				if (!(attr->iconWidth.flagDefined)) {
					attr->iconWidth = iconSize;
				}
				if (!(attr->iconHeight.flagDefined)) {
					attr->iconHeight = iconSize;
				}
			}
		}
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(iconAlign, setIconAlignment)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(textAlign, setTextAlignment)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(textBeforeIcon, setTextBeforeIcon)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(extendTextFrame, setExtendTextFrame)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(orientation, setLayoutOrientation)

		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconMarginLeft, setIconMarginLeft, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconMarginTop, setIconMarginTop, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconMarginRight, setIconMarginRight, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconMarginBottom, setIconMarginBottom, checkPosition)
		if (op == OP_PARSE) {
			SAppDimensionValue iconMargin;
			LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, iconMargin, checkPosition)
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

		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(textMarginLeft, setTextMarginLeft, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(textMarginTop, setTextMarginTop, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(textMarginRight, setTextMarginRight, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(textMarginBottom, setTextMarginBottom, checkPosition)
		if (op == OP_PARSE) {
			SAppDimensionValue textMargin;
			LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, textMargin, checkPosition)
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

		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(colorOverlay, setColorOverlay)

		String strStates[] = {"Default", "Normal", "Hover", "Focused", "FocusedHover", "Pressed", "Disabled"};
		for (sl_uint32 i = 0; i < SLIB_SAPP_LAYOUT_BUTTON_CATEGORY_MAX; i++) {
			SAppLayoutButtonCategory& category = attr->categories[i];
			for (sl_uint32 k = 0; k < (sl_uint32)(ButtonState::Count); k++) {
				if (op == OP_PARSE) {
					String suffix;
					if (i > 0) {
						suffix = String::format("%s%d", strStates[k], i);
					} else {
						suffix = strStates[k];
					}
#define BUTTON_STATE_GET_VALUE(NAME, ...) \
					String value_##NAME##_name = #NAME + suffix; \
					String value_##NAME = resourceItem->getXmlAttribute(value_##NAME##_name); \
					if (k == 0) { \
						if (value_##NAME.isEmpty()) { \
							String suffixNone = String::format("%d", i); \
							value_##NAME##_name = #NAME + suffixNone; \
							value_##NAME = resourceItem->getXmlAttribute(value_##NAME##_name); \
						} \
					} \
					if (value_##NAME.isEmpty()) { \
						if (k) { \
							String suffixNone = String::format("%d", i); \
							value_##NAME##_name = #NAME + suffixNone; \
							value_##NAME = resourceItem->getXmlAttribute(value_##NAME##_name); \
							if (value_##NAME.isNotEmpty()) { \
								value_##NAME = "@null"; \
							} \
						} \
					} \
					if (!(category.NAME[k].parse(value_##NAME, ##__VA_ARGS__))) { \
						_logError(element, g_str_error_resource_layout_attribute_invalid, value_##NAME##_name, value_##NAME); \
						return sl_false; \
					}

					BUTTON_STATE_GET_VALUE(textColor, element)
					BUTTON_STATE_GET_VALUE(icon, this, element)
					BUTTON_STATE_GET_VALUE(background, this, element)
					BUTTON_STATE_GET_VALUE(borderStyle)
					BUTTON_STATE_GET_VALUE(borderWidth, this)
					BUTTON_STATE_GET_VALUE(borderColor, element)
					BUTTON_STATE_GET_VALUE(colorOverlay, element)

					if (!(category.borderWidth[k].flagDefined)) {
						if (category.borderColor[k].flagDefined || category.borderStyle[k].flagDefined) {
							category.borderWidth[k].flagDefined = sl_true;
							category.borderWidth[k].amount = 1;
							category.borderWidth[k].unit = SAppDimensionValue::PX;
						}
					}

				} else if (op == OP_GENERATE_CPP) {
					if (category.textColor[k].flagDefined) {
						String strTextColor;
						if (_getColorAccessString(resource->name, category.textColor[k], strTextColor)) {
							params->sbDefineInit->add(String::format("%s%s->setTextColor(%s, slib::ButtonState::%s, %d, slib::UIUpdateMode::Init);%n", strTab, name, strTextColor, strStates[k], i));
						} else {
							return sl_false;
						}
					}
					if (category.icon[k].flagDefined) {
						String strIcon;
						if (_getDrawableAccessString(resource->name, category.icon[k], strIcon)) {
							params->sbDefineInit->add(String::format("%s%s->setIcon(%s, slib::ButtonState::%s, %d, slib::UIUpdateMode::Init);%n", strTab, name, strIcon, strStates[k], i));
						} else {
							return sl_false;
						}
					}
					if (category.background[k].flagDefined) {
						String strBackground;
						if (!(_getDrawableAccessString(resource->name, category.background[k], strBackground))) {
							return sl_false;
						}
						params->sbDefineInit->add(String::format("%s%s->setBackground(%s, slib::ButtonState::%s, %d, slib::UIUpdateMode::Init);%n", strTab, name, strBackground, strStates[k], i));
					}
					if (category.borderWidth[k].flagDefined || category.borderColor[k].flagDefined || category.borderStyle[k].flagDefined) {
						if (category.borderWidth[k].flagDefined && Math::isAlmostZero(category.borderWidth[k].amount)) {
							params->sbDefineInit->add(String::format("%s%s->setBorder(sl_null, slib::ButtonState::%s, %d, slib::UIUpdateMode::Init);%n", strTab, name, strStates[k], i));
						} else {
							StringBuffer* sb;
							String strUpdateMode;
							if (category.borderWidth[k].isNeededOnLayoutFunction()) {
								sb = params->sbDefineLayout;
								strUpdateMode = "slib::UIUpdateMode::None";
							} else {
								sb = params->sbDefineInit;
								strUpdateMode = "slib::UIUpdateMode::Init";
							}
							String _borderWidth, _borderColor, _borderStyle;
							if (category.borderWidth[k].flagDefined) {
								_borderWidth = category.borderWidth[k].getAccessString();
							} else {
								_borderWidth = "1";
							}
							if (category.borderColor[k].flagDefined) {
								if (!(_getColorAccessString(resource->name, category.borderColor[k], _borderColor))) {
									return sl_false;
								}
							} else {
								_borderColor = "slib::Color::Black";
							}
							if (category.borderStyle[k].flagDefined) {
								_borderStyle = category.borderStyle[k].getAccessString();
							} else {
								_borderStyle = "slib::PenStyle::Solid";
							}
							sb->add(String::format("%s%s->setBorder(slib::Pen::create(%s, %s, %s), slib::ButtonState::%s, %d, %s);%n", strTab, name, _borderStyle, _borderWidth, _borderColor, strStates[k], i, strUpdateMode));
						}
					}
					if (category.colorOverlay[k].flagDefined) {
						String strColorOverlay;
						if (_getColorAccessString(resource->name, category.colorOverlay[k], strColorOverlay)) {
							params->sbDefineInit->add(String::format("%s%s->setColorOverlay(%s, slib::ButtonState::%s, %d, slib::UIUpdateMode::Init);%n", strTab, name, strColorOverlay, strStates[k], i));
						} else {
							return sl_false;
						}
					}
				} else if (op == OP_SIMULATE) {
					if (category.textColor[k].flagDefined) {
						if (!flagOnLayout) {
							Color _textColor;
							if (!(_getColorValue(resource->name, category.textColor[k], _textColor))) {
								return sl_false;
							}
							view->setTextColor(_textColor, (ButtonState)k, i, UIUpdateMode::Init);
						}
					}
					if (category.background[k].flagDefined) {
						if (flagOnLayout) {
							Ref<Drawable> _background;
							if (!(_getDrawableValue(resource->name, category.background[k], _background))) {
								return sl_false;
							}
							view->setBackground(_background, (ButtonState)k, i, UIUpdateMode::None);
						}
					}
					if (category.icon[k].flagDefined) {
						if (flagOnLayout) {
							Ref<Drawable> _icon;
							if (!(_getDrawableValue(resource->name, category.icon[k], _icon))) {
								return sl_false;
							}
							view->setIcon(_icon, (ButtonState)k, i, UIUpdateMode::None);
						}
					}
					if (category.borderWidth[k].flagDefined || category.borderColor[k].flagDefined || category.borderStyle[k].flagDefined) {
						if (category.borderWidth[k].flagDefined && Math::isAlmostZero(category.borderWidth[k].amount)) {
							if (!flagOnLayout) {
								view->setBorder(sl_null, (ButtonState)k, i, UIUpdateMode::Init);
							}
						} else {
							if (flagOnLayout) {
								sl_real _borderWidth;
								Color _borderColor;
								PenStyle _borderStyle;
								if (category.borderWidth[k].flagDefined) {
									_borderWidth = _getDimensionValue(category.borderWidth[k]);
								} else {
									_borderWidth = 1;
								}
								if (category.borderColor[k].flagDefined) {
									if (!(_getColorValue(resource->name, category.borderColor[k], _borderColor))) {
										return sl_false;
									}
								} else {
									_borderColor = Color::Black;
								}
								if (category.borderStyle[k].flagDefined) {
									_borderStyle = category.borderStyle[k].value;
								} else {
									_borderStyle = PenStyle::Solid;
								}
								view->setBorder(Pen::create(_borderStyle, _borderWidth, _borderColor), (ButtonState)k, i, UIUpdateMode::None);
							}
						}
					}
					if (category.colorOverlay[k].flagDefined) {
						if (!flagOnLayout) {
							Color _colorOverlay;
							if (!(_getColorValue(resource->name, category.colorOverlay[k], _colorOverlay))) {
								return sl_false;
							}
							view->setColorOverlay(_colorOverlay, (ButtonState)k, i, UIUpdateMode::Init);
						}
					}
				}
			}
		}

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(defaultColorFilter, setUsingDefaultColorFilter)

		LAYOUT_CONTROL_SET_NATIVE_WIDGET_CHECK_BACKGROUND_COLOR

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Label, LabelView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_STRING_ATTR_UPDATE(text, setText)
		LAYOUT_CONTROL_STRING_ATTR_UPDATE(hyperText, setHyperText)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColor, setTextColor)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(multiLine, setMultiLine)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(ellipsize, setEllipsize)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(lines, setLineCount)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(linksInText, setDetectingHyperlinksInPlainText);
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(linkColor, setLinkColor)
		LAYOUT_CONTROL_GENERIC_ATTR(mnemonic, setMnemonic)

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
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(orientation, setOrientation)
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

		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(lineColor, setLineColor)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(thickness, setLineThickness, checkScalarSize)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(lineStyle, setLineStyle)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity)

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Check, CheckBox)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(Button)

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(checked, setChecked)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Radio, RadioButton)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(Check)

		LAYOUT_CONTROL_STRING_ATTR(value, setValue)

		if (op == OP_PARSE) {
			attr->group = resourceItem->getXmlAttribute("group");
			if (attr->group.isNotEmpty()) {
				if (!(SAppUtil::checkName(attr->group.getData(), attr->group.getLength()))) {
					_logError(element, g_str_error_resource_layout_name_invalid, attr->group);
					return sl_false;
				}
				if (params->resource->itemsByName.find(attr->group)) {
					_logError(element, g_str_error_resource_layout_name_redefined, attr->group);
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
		LAYOUT_CONTROL_STRING_ATTR_UPDATE(text, setText) \
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity) \
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColor, setTextColor) \
		LAYOUT_CONTROL_STRING_ATTR_UPDATE(hintText, setHintText) \
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(hintGravity, setHintGravity) \
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(hintTextColor, setHintTextColor) \
		LAYOUT_CONTROL_FONT_ATTR(hintFont, setHintFont) \
		if (op == OP_PARSE) { \
			if (attr->hintFont.flagDefined) { \
				attr->hintFont.inheritFrom(attr->font); \
			} \
		} \
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(readOnly, setReadOnly) \
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(password, setPassword) \
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(number, setNumber) \
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(lowercase, setLowercase) \
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(uppercase, setUppercase) \
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(multiLine, setMultiLine) \
		LAYOUT_CONTROL_GENERIC_ATTR(returnKey, setReturnKeyType) \
		LAYOUT_CONTROL_GENERIC_ATTR(keyboard, setKeyboardType) \
		LAYOUT_CONTROL_GENERIC_ATTR(autoCap, setAutoCapitalizationType) \
		LAYOUT_CONTROL_BOOLEAN_ATTR(focusNextOnReturnKey, setFocusNextOnReturnKey)

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

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(scale, setScaleMode)
		LAYOUT_CONTROL_GENERIC_ATTR(minAspectRatio, setMinimumAutoAspectRatio)
		LAYOUT_CONTROL_GENERIC_ATTR(maxAspectRatio, setMaximumAutoAspectRatio)


		if (op == OP_PARSE) {
			if (!(resourceItem->attrs->width.flagDefined) && !(resourceItem->attrs->height.flagDefined)) {
				if (resourceItem->attrs->aspectRatio.flagDefined) {
					resourceItem->attrs->width.flagDefined = sl_true;
					resourceItem->attrs->width.amount = 1;
					resourceItem->attrs->width.unit = SAppDimensionValue::WRAP;
				} else {
					resourceItem->attrs->width.flagDefined = sl_true;
					resourceItem->attrs->width.amount = 1;
					resourceItem->attrs->width.unit = SAppDimensionValue::WRAP;
					resourceItem->attrs->height.flagDefined = sl_true;
					resourceItem->attrs->height.amount = 1;
					resourceItem->attrs->height.unit = SAppDimensionValue::WRAP;
				}
			}
		}
		if (!(resourceItem->attrs->aspectRatio.flagDefined)) {
			if (op == OP_GENERATE_CPP) {
				if (resourceItem->attrs->width.flagDefined) {
					if (!(resourceItem->attrs->height.flagDefined)) {
						params->sbDefineInit->add(String::format("%s%s->setAutoAspectRatio(sl_true);%n", strTab, name));
						params->sbDefineInit->add(String::format("%s%s->setAspectRatioMode(slib::AspectRatioMode::AdjustHeight, slib::UIUpdateMode::Init);%n", strTab, name));
					}
				} else {
					if (resourceItem->attrs->height.flagDefined) {
						params->sbDefineInit->add(String::format("%s%s->setAutoAspectRatio(sl_true);%n", strTab, name));
						params->sbDefineInit->add(String::format("%s%s->setAspectRatioMode(slib::AspectRatioMode::AdjustWidth, slib::UIUpdateMode::Init);%n", strTab, name));
					}
				}
			} else if (op == OP_SIMULATE) {
				if (!flagOnLayout) {
					if (resourceItem->attrs->width.flagDefined) {
						if (!(resourceItem->attrs->height.flagDefined)) {
							view->setAutoAspectRatio(sl_true);
							view->setAspectRatioMode(AspectRatioMode::AdjustHeight, slib::UIUpdateMode::Init);
						}
					} else {
						if (resourceItem->attrs->height.flagDefined) {
							view->setAutoAspectRatio(sl_true);
							view->setAspectRatioMode(AspectRatioMode::AdjustWidth, slib::UIUpdateMode::Init);
						}
					}
				}
			}
		}

		LAYOUT_CONTROL_DRAWABLE_ATTR(src, setSource)
		LAYOUT_CONTROL_STRING_ATTR(url, loadUrl)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

#define LAYOUT_CONTROL_PROCESS_SELECT_ITEMS \
		if (op == OP_PARSE) { \
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(itemXmls, "item") \
			for (sl_size i = 0; i < itemXmls.count; i++) { \
				LAYOUT_CONTROL_DEFINE_XML(itemXml, itemXmls[i]) \
				SAppLayoutSelectItem subItem; \
				LAYOUT_CONTROL_PARSE_XML_STRING_ATTR(itemXml, subItem., title) \
				if (!(subItem.title.flagDefined)) { \
					String text = itemXml.getXmlText(); \
					if (text.isNotEmpty()) { \
						if (!(subItem.title.parse(text, itemXml.element))) { \
							_logError(itemXml.element, g_str_error_resource_layout_value_invalid, text); \
							return sl_false; \
						} \
					} \
				} \
				LAYOUT_CONTROL_PARSE_XML_STRING_ATTR(itemXml, subItem., value) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(itemXml, subItem., selected) \
				if (!(attr->items.add_NoLock(Move(subItem)))) { \
					_logError(itemXml.element, g_str_error_out_of_memory); \
					return sl_false; \
				} \
			} \
		} else if (op == OP_GENERATE_CPP) { \
			ListElements<SAppLayoutSelectItem> selectItems(attr->items); \
			if (selectItems.count > 0) { \
				for (sl_size i = 0; i < selectItems.count; i++) { \
					SAppLayoutSelectItem& selectItem = selectItems[i]; \
					String strSelectedItemTitle; \
					if (!(_getStringAccessString(resource->name, selectItem.title, strSelectedItemTitle))) { \
						return sl_false; \
					} \
					String strSelectedItemValue; \
					if (!(_getStringAccessString(resource->name, selectItem.value, strSelectedItemValue))) { \
						return sl_false; \
					} \
					params->sbDefineInit->add(String::format("%s%s->addItem(%s, %s, slib::UIUpdateMode::Init);%n", strTab, name, strSelectedItemValue, strSelectedItemTitle)); \
					if (selectItem.selected.flagDefined && selectItem.selected.value) { \
						params->sbDefineInit->add(String::format("%s%s->selectItem(%d, slib::UIUpdateMode::Init);%n", strTab, name, i)); \
					} \
				} \
			} \
		} else if (op == OP_SIMULATE) { \
			if (!flagOnLayout) { \
				ListElements<SAppLayoutSelectItem> selectItems(attr->items); \
				if (selectItems.count > 0) { \
					sl_uint32 n = (sl_uint32)(selectItems.count); \
					for (sl_uint32 i = 0; i < n; i++) { \
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

		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColor, setTextColor)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity)

		LAYOUT_CONTROL_SET_NATIVE_WIDGET

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(SelectSwitch, SelectSwitch)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColor, setTextColor)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity)

		LAYOUT_CONTROL_DRAWABLE_ATTR(leftIcon, setLeftIcon)
		LAYOUT_CONTROL_DRAWABLE_ATTR(rightIcon, setRightIcon)

		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconWidth, setIconWidth, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconHeight, setIconHeight, checkScalarSize)
		if (op == OP_PARSE) {
			SAppDimensionValue iconSize;
			LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, iconSize, checkScalarSize)
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

		LAYOUT_CONTROL_STRING_ATTR_UPDATE(text, setText)

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
					_logError(element, g_str_error_resource_layout_scrollview_must_contain_one_child);
					return sl_false;
				}
				Ref<SAppLayoutResourceItem> contentItem = _parseLayoutResourceItemChild(params->resource, resourceItem, childXmls[0], params->source);
				if (contentItem.isNull()) {
					return sl_false;
				}
				if (!(resourceItem->attrs->scrolling.flagDefined) || resourceItem->attrs->scrolling.vertical) {
					contentItem->attrs->topMode = PositionMode::Free;
					contentItem->attrs->bottomMode = PositionMode::Free;
				}
				if (!(resourceItem->attrs->scrolling.flagDefined) || resourceItem->attrs->scrolling.horizontal) {
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
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(orientation, setOrientation)
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
					_logError(element, g_str_error_layout_not_found, attr->itemLayout);
					return sl_false;
				}
				if (layoutItem->layoutType != SAppLayoutType::View) {
					_logError(element, g_str_error_layout_is_not_view, attr->itemLayout);
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
					_logError(element, g_str_error_layout_not_found, attr->itemLayout);
					return sl_false;
				}
				if (layoutItem->layoutType != SAppLayoutType::View) {
					_logError(element, g_str_error_layout_is_not_view, attr->itemLayout);
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
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., width, checkSize)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., minWidth, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., maxWidth, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., margin, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., marginLeft, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., marginRight, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., padding, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., paddingLeft, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., paddingRight, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(columnXml, column., background)
					LAYOUT_CONTROL_PARSE_XML_ATTR(columnXml, column., align)
					if (!(attr->columns.add_NoLock(Move(column)))) {
						_logError(columnXml.element, g_str_error_out_of_memory);
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
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., height, checkSize)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., minHeight, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., maxHeight, checkScalarSize)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., margin, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., marginTop, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., marginBottom, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., padding, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., paddingTop, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., paddingBottom, checkPosition)
					LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(rowXml, row., background)
					LAYOUT_CONTROL_PARSE_XML_ATTR(rowXml, row., align)
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
						LAYOUT_CONTROL_PARSE_XML_ATTR(xmlView, cell., rowspan)
						LAYOUT_CONTROL_PARSE_XML_ATTR(xmlView, cell., colspan)
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
								_logError(xmlView.element, g_str_error_out_of_memory);
								return sl_false;
							}
						}
						for (sl_uint32 t1 = 0; t1 < cell.rowspan.value; t1++) {
							for (sl_uint32 t2 = 0; t2 < cell.colspan.value; t2++) {
								cellAllocs.put_NoLock(Pair<sl_uint32, sl_uint32>(i + t1, iCell + t2), sl_true);
							}
						}
						if (!(row.cells.setCount_NoLock(iCell+1))) {
							_logError(xmlView.element, g_str_error_out_of_memory);
							return sl_false;
						}
						row.cells.setAt_NoLock(iCell, Move(cell));
					}

					if (!(attr->rows.add_NoLock(Move(row)))) {
						_logError(rowXml.element, g_str_error_out_of_memory);
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
			params->sbDefineInit->add(String::format("%s%s->setColumnCount(%d, slib::UIUpdateMode::Init);%n", strTab, name, nCols));
			params->sbDefineInit->add(String::format("%s%s->setRowCount(%d, slib::UIUpdateMode::Init);%n", strTab, name, nRows));
			for (iCol = 0; iCol < nCols; iCol++) {
				SAppLayoutTableColumn& col = cols[iCol];
				if (col.width.flagDefined) {
					if (col.width.unit == SAppDimensionValue::WRAP) {
						params->sbDefineInit->add(String::format("%s%s->setColumnWidthWrapping(%d, slib::UIUpdateMode::Init);%n", strTab, name, iCol));
					} else if (col.width.unit == SAppDimensionValue::FILL) {
						params->sbDefineInit->add(String::format("%s%s->setColumnWidthFilling(%d, %ff, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.width.amount));
					} else if (col.width.unit == SAppDimensionValue::WEIGHT) {
						params->sbDefineInit->add(String::format("%s%s->setColumnWidthWeight(%d, %ff, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.width.amount));
					} else {
						if (col.width.isNeededOnLayoutFunction()) {
							params->sbDefineLayout->add(String::format("%s%s->setColumnWidth(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iCol, col.width.getAccessString()));
						} else {
							params->sbDefineInit->add(String::format("%s%s->setColumnWidth(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.width.getAccessString()));
						}
					}
				}
				if (col.minWidth.flagDefined) {
					if (col.minWidth.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setColumnMinimumWidth(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iCol, col.minWidth.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setColumnMinimumWidth(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.minWidth.getAccessString()));
					}
				}
				if (col.maxWidth.flagDefined) {
					if (col.maxWidth.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setColumnMaximumWidth(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iCol, col.maxWidth.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setColumnMaximumWidth(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.maxWidth.getAccessString()));
					}
				}
				if (col.margin.flagDefined) {
					if (col.margin.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setColumnMargin(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iCol, col.margin.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setColumnMargin(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.margin.getAccessString()));
					}
				}
				if (col.marginLeft.flagDefined) {
					if (col.marginLeft.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setColumnMarginLeft(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iCol, col.marginLeft.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setColumnMarginLeft(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.marginLeft.getAccessString()));
					}
				}
				if (col.marginRight.flagDefined) {
					if (col.marginRight.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setColumnMarginRight(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iCol, col.marginRight.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setColumnMarginRight(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.marginRight.getAccessString()));
					}
				}
				if (col.padding.flagDefined) {
					if (col.padding.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setColumnPadding(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iCol, col.padding.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setColumnPadding(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.padding.getAccessString()));
					}
				}
				if (col.paddingLeft.flagDefined) {
					if (col.paddingLeft.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setColumnPaddingLeft(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iCol, col.paddingLeft.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setColumnPaddingLeft(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.paddingLeft.getAccessString()));
					}
				}
				if (col.paddingRight.flagDefined) {
					if (col.paddingRight.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setColumnPaddingRight(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iCol, col.paddingRight.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setColumnPaddingRight(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.paddingRight.getAccessString()));
					}
				}
				if (col.background.flagDefined) {
					String strBackground;
					if (!(_getDrawableAccessString(resource->name, col.background, strBackground))) {
						return sl_false;
					}
					params->sbDefineInit->add(String::format("%s%s->setColumnBackground(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, strBackground));
				}
				if (col.align.flagDefined) {
					params->sbDefineInit->add(String::format("%s%s->setColumnAlignment(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iCol, col.align.getAccessString()));
				}
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				SAppLayoutTableRow& row = rows[iRow];
				if (row.height.flagDefined) {
					if (row.height.unit == SAppDimensionValue::WRAP) {
						params->sbDefineInit->add(String::format("%s%s->setRowHeightWrapping(%d, slib::UIUpdateMode::Init);%n", strTab, name, iRow));
					} else if (row.height.unit == SAppDimensionValue::FILL) {
						params->sbDefineInit->add(String::format("%s%s->setRowHeightFilling(%d, %ff, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.height.amount));
					} else if (row.height.unit == SAppDimensionValue::WEIGHT) {
						params->sbDefineInit->add(String::format("%s%s->setRowHeightWeight(%d, %ff, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.height.amount));
					} else {
						if (row.height.isNeededOnLayoutFunction()) {
							params->sbDefineLayout->add(String::format("%s%s->setRowHeight(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iRow, row.height.getAccessString()));
						} else {
							params->sbDefineInit->add(String::format("%s%s->setRowHeight(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.height.getAccessString()));
						}
					}
				}
				if (row.minHeight.flagDefined) {
					if (row.minHeight.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setRowMinimumHeight(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iRow, row.minHeight.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setRowMinimumHeight(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.minHeight.getAccessString()));
					}
				}
				if (row.maxHeight.flagDefined) {
					if (row.maxHeight.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setRowMaximumHeight(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iRow, row.maxHeight.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setRowMaximumHeight(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.maxHeight.getAccessString()));
					}
				}
				if (row.margin.flagDefined) {
					if (row.margin.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setRowMargin(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iRow, row.margin.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setRowMargin(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.margin.getAccessString()));
					}
				}
				if (row.marginTop.flagDefined) {
					if (row.marginTop.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setRowMarginTop(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iRow, row.marginTop.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setRowMarginTop(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.marginTop.getAccessString()));
					}
				}
				if (row.marginBottom.flagDefined) {
					if (row.marginBottom.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setRowMarginBottom(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iRow, row.marginBottom.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setRowMarginBottom(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.marginBottom.getAccessString()));
					}
				}
				if (row.padding.flagDefined) {
					if (row.padding.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setRowPadding(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iRow, row.padding.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setRowPadding(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.padding.getAccessString()));
					}
				}
				if (row.paddingTop.flagDefined) {
					if (row.paddingTop.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setRowPaddingTop(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iRow, row.paddingTop.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setRowPaddingTop(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.paddingTop.getAccessString()));
					}
				}
				if (row.paddingBottom.flagDefined) {
					if (row.paddingBottom.isNeededOnLayoutFunction()) {
						params->sbDefineLayout->add(String::format("%s%s->setRowPaddingBottom(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, iRow, row.paddingBottom.getAccessString()));
					} else {
						params->sbDefineInit->add(String::format("%s%s->setRowPaddingBottom(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.paddingBottom.getAccessString()));
					}
				}
				if (row.background.flagDefined) {
					String strBackground;
					if (!(_getDrawableAccessString(resource->name, row.background, strBackground))) {
						return sl_false;
					}
					params->sbDefineInit->add(String::format("%s%s->setRowBackground(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, strBackground));
				}
				if (row.align.flagDefined) {
					params->sbDefineInit->add(String::format("%s%s->setRowAlignment(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, iRow, row.align.getAccessString()));
				}
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
				if (col.width.flagDefined) {
					if (col.width.unit == SAppDimensionValue::WRAP) {
						if (!flagOnLayout) {
							view->setColumnWidthWrapping(iCol, UIUpdateMode::Init);
						}
					} else if (col.width.unit == SAppDimensionValue::FILL) {
						if (!flagOnLayout) {
							view->setColumnWidthFilling(iCol, col.width.amount, UIUpdateMode::Init);
						}
					} else if (col.width.unit == SAppDimensionValue::WEIGHT) {
						if (!flagOnLayout) {
							view->setColumnWidthWeight(iCol, col.width.amount, UIUpdateMode::Init);
						}
					} else {
						if (flagOnLayout) {
							view->setColumnWidth(iCol, _getDimensionValue(col.width), UIUpdateMode::None);
						}
					}
				}
				if (flagOnLayout) {
					if (col.minWidth.flagDefined) {
						view->setColumnMinimumWidth(iCol, _getDimensionValue(col.minWidth), UIUpdateMode::None);
					}
					if (col.maxWidth.flagDefined) {
						view->setColumnMaximumWidth(iCol, _getDimensionValue(col.maxWidth), UIUpdateMode::None);
					}
					if (col.margin.flagDefined) {
						view->setColumnMargin(iCol, _getDimensionValue(col.margin), UIUpdateMode::None);
					}
					if (col.marginLeft.flagDefined) {
						view->setColumnMarginLeft(iCol, _getDimensionValue(col.marginLeft), UIUpdateMode::None);
					}
					if (col.marginRight.flagDefined) {
						view->setColumnMarginRight(iCol, _getDimensionValue(col.marginRight), UIUpdateMode::None);
					}
					if (col.padding.flagDefined) {
						view->setColumnPadding(iCol, _getDimensionValue(col.padding), UIUpdateMode::None);
					}
					if (col.paddingLeft.flagDefined) {
						view->setColumnPaddingLeft(iCol, _getDimensionValue(col.paddingLeft), UIUpdateMode::None);
					}
					if (col.paddingRight.flagDefined) {
						view->setColumnPaddingRight(iCol, _getDimensionValue(col.paddingRight), UIUpdateMode::None);
					}
					if (col.background.flagDefined) {
						Ref<Drawable> _background;
						if (!(_getDrawableValue(resource->name, col.background, _background))) {
							return sl_false;
						}
						view->setColumnBackground(iCol, _background, UIUpdateMode::None);
					}
				}
				if (col.align.flagDefined) {
					if (!flagOnLayout) {
						view->setColumnAlignment(iCol, col.align.value, UIUpdateMode::Init);
					}
				}
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				SAppLayoutTableRow& row = rows[iRow];
				if (row.height.flagDefined) {
					if (row.height.unit == SAppDimensionValue::WRAP) {
						if (!flagOnLayout) {
							view->setRowHeightWrapping(iRow, UIUpdateMode::Init);
						}
					} else if (row.height.unit == SAppDimensionValue::FILL) {
						if (!flagOnLayout) {
							view->setRowHeightFilling(iRow, row.height.amount, UIUpdateMode::Init);
						}
					} else if (row.height.unit == SAppDimensionValue::WEIGHT) {
						if (!flagOnLayout) {
							view->setRowHeightWeight(iRow, row.height.amount, UIUpdateMode::Init);
						}
					} else {
						if (flagOnLayout) {
							view->setRowHeight(iRow, _getDimensionValue(row.height), UIUpdateMode::None);
						}
					}
				}
				if (flagOnLayout) {
					if (row.minHeight.flagDefined) {
						view->setRowMinimumHeight(iRow, _getDimensionValue(row.minHeight), UIUpdateMode::None);
					}
					if (row.maxHeight.flagDefined) {
						view->setRowMaximumHeight(iRow, _getDimensionValue(row.maxHeight), UIUpdateMode::None);
					}
					if (row.margin.flagDefined) {
						view->setRowMargin(iRow, _getDimensionValue(row.margin), UIUpdateMode::None);
					}
					if (row.marginTop.flagDefined) {
						view->setRowMarginTop(iRow, _getDimensionValue(row.marginTop), UIUpdateMode::None);
					}
					if (row.marginBottom.flagDefined) {
						view->setRowMarginBottom(iRow, _getDimensionValue(row.marginBottom), UIUpdateMode::None);
					}
					if (row.padding.flagDefined) {
						view->setRowPadding(iRow, _getDimensionValue(row.padding), UIUpdateMode::None);
					}
					if (row.paddingTop.flagDefined) {
						view->setRowPaddingTop(iRow, _getDimensionValue(row.paddingTop), UIUpdateMode::None);
					}
					if (row.paddingBottom.flagDefined) {
						view->setRowPaddingBottom(iRow, _getDimensionValue(row.paddingBottom), UIUpdateMode::None);
					}
					if (row.background.flagDefined) {
						Ref<Drawable> _background;
						if (!(_getDrawableValue(resource->name, row.background, _background))) {
							return sl_false;
						}
						view->setRowBackground(iRow, _background, UIUpdateMode::None);
					}
				}
				if (row.align.flagDefined) {
					if (!flagOnLayout) {
						view->setRowAlignment(iRow, row.align.value, UIUpdateMode::Init);
					}
				}
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
				LAYOUT_CONTROL_PARSE_XML_COLOR_ATTR(columnXml, column., title)
				LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., width, checkScalarSize)
				LAYOUT_CONTROL_PARSE_XML_ATTR(columnXml, column., align)
				LAYOUT_CONTROL_PARSE_XML_ATTR(columnXml, column., headerAlign)
				if (!(attr->columns.add_NoLock(Move(column)))) {
					_logError(columnXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == OP_GENERATE_CPP) {
			ListElements<SAppLayoutListControlColumn> columns(attr->columns);
			if (columns.count > 0) {
				params->sbDefineInit->add(String::format("%s%s->setColumnCount(%d, slib::UIUpdateMode::Init);%n", strTab, name, columns.count));
				for (sl_size i = 0; i < columns.count; i++) {
					SAppLayoutListControlColumn& column = columns[i];
					String strColumnTitle;
					if (!(_getStringAccessString(resource->name, column.title, strColumnTitle))) {
						return sl_false;
					}
					if (column.title.flagDefined) {
						params->sbDefineInit->add(String::format("%s%s->setHeaderText(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, strColumnTitle));
					}
					if (column.width.flagDefined) {
						if (column.width.isNeededOnLayoutFunction()) {
							params->sbDefineLayout->add(String::format("%s%s->setColumnWidth(%d, %s, slib::UIUpdateMode::None);%n", strTab, name, i, column.width.getAccessString()));
						} else {
							params->sbDefineInit->add(String::format("%s%s->setColumnWidth(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, column.width.getAccessString()));
						}
					}
					if (column.align.flagDefined) {
						params->sbDefineInit->add(String::format("%s%s->setColumnAlignment(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, column.align.getAccessString()));
					}
					if (column.headerAlign.flagDefined) {
						params->sbDefineInit->add(String::format("%s%s->setHeaderAlignment(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, column.headerAlign.getAccessString()));
					}
				}
			}
		} else if (op == OP_SIMULATE) {
			ListElements<SAppLayoutListControlColumn> columns(attr->columns);
			if (columns.count > 0) {
				sl_uint32 n = (sl_uint32)(columns.count);
				view->setColumnCount(n, UIUpdateMode::Init);
				for (sl_uint32 i = 0; i < n; i++) {
					SAppLayoutListControlColumn& column = columns[i];
					if (!flagOnLayout) {
						if (column.title.flagDefined) {
							String columnTitle;
							if (!(_getStringValue(resource->name, column.title, columnTitle))) {
								return sl_false;
							}
							view->setHeaderText(i, columnTitle, UIUpdateMode::Init);
						}
						if (column.align.flagDefined) {
							view->setColumnAlignment(i, column.align.value, UIUpdateMode::Init);
						}
						if (column.headerAlign.flagDefined) {
							view->setHeaderAlignment(i, column.headerAlign.value, UIUpdateMode::Init);
						}
					}
					if (column.width.flagDefined) {
						if (flagOnLayout) {
							view->setColumnWidth(i, _getDimensionValue(column.width), UIUpdateMode::None);
						}
					}
				}
			}
		}

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Render, RenderView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_GENERIC_ATTR(redraw, setRedrawMode)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Tab, TabView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(orientation, setOrientation)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(tabWidth, setTabWidth, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(tabHeight, setTabHeight, checkScalarSize)
		LAYOUT_CONTROL_DRAWABLE_ATTR(barBackground, setBarBackground)
		LAYOUT_CONTROL_DRAWABLE_ATTR(contentBackground, setContentBackground)
		LAYOUT_CONTROL_DRAWABLE_ATTR(tabBackground, setTabBackground)
		LAYOUT_CONTROL_DRAWABLE_ATTR(selectedTabBackground, setSelectedTabBackground)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverTabBackground, setHoverTabBackground)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(labelColor, setLabelColor)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(selectedLabelColor, setSelectedLabelColor)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(hoverLabelColor, setHoverLabelColor)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(tabAlign, setTabAlignment)

		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(tabPaddingLeft, setTabPaddingLeft, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(tabPaddingTop, setTabPaddingTop, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(tabPaddingRight, setTabPaddingRight, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(tabPaddingBottom, setTabPaddingBottom, checkPosition)
		if (op == OP_PARSE) {
			SAppDimensionValue tabPadding;
			LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, tabPadding, checkPosition)
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

		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(tabSpaceSize, setTabSpaceSize, checkPosition)

		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconWidth, setIconWidth, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(iconHeight, setIconHeight, checkScalarSize)
		if (op == OP_PARSE) {
			SAppDimensionValue iconSize;
			LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, iconSize, checkScalarSize)
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
				LAYOUT_CONTROL_PARSE_XML_STRING_ATTR(itemXml, subItem., label)
				LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(itemXml, subItem., icon)
				LAYOUT_CONTROL_PARSE_XML_ATTR(itemXml, subItem., selected)
				LAYOUT_CONTROL_DEFINE_XML_CHILDREN(childXmls, itemXml, sl_null)
				if (childXmls.count > 0) {
					if (childXmls.count != 1) {
						_logError(itemXml.element, g_str_error_resource_layout_item_must_contain_one_child);
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
					_logError(itemXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == OP_GENERATE_CPP) {
			ListElements<SAppLayoutTabItem> subItems(attr->items);
			if (subItems.count > 0) {
				sl_size indexSelected = 0;
				sl_bool flagSelected = sl_false;

				params->sbDefineInit->add(String::format("%s%s->setTabCount(%d, slib::UIUpdateMode::Init);%n", strTab, name, subItems.count));

				for (sl_size i = 0; i < subItems.count; i++) {
					SAppLayoutTabItem& subItem = subItems[i];
					if (subItem.label.flagDefined) {
						String strItemLabel;
						if (!(_getStringAccessString(resource->name, subItem.label, strItemLabel))) {
							return sl_false;
						}
						params->sbDefineInit->add(String::format("%s%s->setTabLabel(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, strItemLabel));
					}
					if (subItem.icon.flagDefined) {
						String strItemIcon;
						if (!(_getDrawableAccessString(resource->name, subItem.icon, strItemIcon))) {
							return sl_false;
						}
						params->sbDefineInit->add(String::format("%s%s->setTabIcon(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, strItemIcon));
					}
					if (subItem.selected.flagDefined && subItem.selected.value) {
						flagSelected = sl_true;
						indexSelected = i;
					}
				}

				if (flagSelected) {
					params->sbDefineInit->add(String::format("%s%s->selectTab(%d, slib::UIUpdateMode::Init);%n", strTab, name, indexSelected));
				}
			}
		} else if (op == OP_SIMULATE) {
			ListElements<SAppLayoutTabItem> subItems(attr->items);
			if (subItems.count > 0) {

				sl_uint32 indexSelected = 0;
				sl_bool flagSelected = sl_false;

				sl_uint32 nSubItems = (sl_uint32)(subItems.count);
				if (!flagOnLayout) {
					view->setTabCount(nSubItems, UIUpdateMode::Init);
				}

				for (sl_uint32 i = 0; i < nSubItems; i++) {

					SAppLayoutTabItem& subItem = subItems[i];
					if (!flagOnLayout) {
						if (subItem.label.flagDefined) {
							String itemLabel;
							if (!(_getStringValue(resource->name, subItem.label, itemLabel))) {
								return sl_false;
							}
							view->setTabLabel(i, itemLabel, UIUpdateMode::Init);
						}
						if (subItem.icon.flagDefined) {
							Ref<Drawable> itemIcon;
							if (!(_getDrawableValue(resource->name, subItem.icon, itemIcon))) {
								return sl_false;
							}
							view->setTabIcon(i, itemIcon, UIUpdateMode::Init);
						}
					}
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

		LAYOUT_CONTROL_DRAWABLE_ATTR(itemIcon, setItemIcon)
		LAYOUT_CONTROL_DRAWABLE_ATTR(opendIcon, setOpenedItemIcon)
		LAYOUT_CONTROL_DRAWABLE_ATTR(closedIcon, setClosedItemIcon)
		LAYOUT_CONTROL_DRAWABLE_ATTR(collapsedIcon, setCollapsedIcon)
		LAYOUT_CONTROL_DRAWABLE_ATTR(expandedIcon, setExpandedIcon)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(selectedBackgroundColor, setSelectedItemBackgroundColor)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColor, setItemTextColor)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(hoverTextColor, setHoverItemTextColor)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(selectedTextColor, setSelectedItemTextColor)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(itemHeight, setItemHeight, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(itemPadding, setItemPadding, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(itemIndent, setItemIndent, checkPosition)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(textIndent, setTextIndent, checkPosition)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Split, SplitLayout)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(orientation, setOrientation)
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

		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(dividerWidth, setDividerWidth, checkScalarSize)
		LAYOUT_CONTROL_DRAWABLE_ATTR(dividerBackground, setDividerBackground)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(dividerColor, setDividerColor)
		LAYOUT_CONTROL_DIMENSION_ATTR(cursorMargin, setCursorMargin, checkScalarSize)

		sl_bool flagRelayoutOnInit = sl_false;
		sl_bool flagRelayoutOnLayout = sl_false;

		if (op == OP_PARSE) {
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(itemXmls, "item")
			for (sl_size i = 0; i < itemXmls.count; i++) {
				LAYOUT_CONTROL_DEFINE_XML(itemXml, itemXmls[i])
				SAppLayoutSplitItem subItem;
				LAYOUT_CONTROL_PARSE_XML_ATTR(itemXml, subItem., weight)
				LAYOUT_CONTROL_PARSE_XML_ATTR(itemXml, subItem., minWeight)
				LAYOUT_CONTROL_PARSE_XML_ATTR(itemXml, subItem., maxWeight)
				LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(itemXml, subItem., minSize, checkScalarSize)
				LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(itemXml, subItem., maxSize, checkScalarSize)
				LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(itemXml, subItem., dividerWidth, checkScalarSize)
				LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(itemXml, subItem., dividerBackground)
				LAYOUT_CONTROL_PARSE_XML_COLOR_ATTR(itemXml, subItem., dividerColor)
				LAYOUT_CONTROL_DEFINE_XML_CHILDREN(childXmls, itemXml, sl_null)
				if (childXmls.count > 0) {
					if (childXmls.count != 1) {
						_logError(itemXml.element, g_str_error_resource_layout_item_must_contain_one_child);
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
					_logError(itemXml.element, g_str_error_out_of_memory);
					return sl_false;
				}
			}
		} else if (op == OP_GENERATE_CPP) {
			ListElements<SAppLayoutSplitItem> subItems(attr->items);
			if (subItems.count > 0) {
				if (subItems.count > 2) {
					params->sbDefineInit->add(String::format("%s%s->setItemCount(%d, slib::UIUpdateMode::Init);%n", strTab, name, subItems.count));
				}
				for (sl_size i = 0; i < subItems.count; i++) {
					SAppLayoutSplitItem& subItem = subItems[i];
					if (subItem.weight.flagDefined) {
						params->sbDefineInit->add(String::format("%s%s->setItemWeight(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, subItem.weight.getAccessString()));
						flagRelayoutOnInit = sl_true;
					}
					if (subItem.minWeight.flagDefined) {
						params->sbDefineInit->add(String::format("%s%s->setItemMinimumWeight(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, subItem.minWeight.getAccessString()));
						flagRelayoutOnInit = sl_true;
					}
					if (subItem.maxWeight.flagDefined) {
						params->sbDefineInit->add(String::format("%s%s->setItemMaximumWeight(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, subItem.maxWeight.getAccessString()));
						flagRelayoutOnInit = sl_true;
					}
					if (subItem.minSize.flagDefined) {
						if (subItem.minSize.isNeededOnLayoutFunction()) {
							params->sbDefineLayout->add(String::format("%s%s->setItemMinimumSize(%d, %s);%n", strTab, name, i, subItem.minSize.getAccessString()));
							flagRelayoutOnLayout = sl_true;
						} else {
							params->sbDefineInit->add(String::format("%s%s->setItemMinimumSize(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, subItem.minSize.getAccessString()));
							flagRelayoutOnInit = sl_true;
						}
					}
					if (subItem.maxSize.flagDefined) {
						if (subItem.maxSize.isNeededOnLayoutFunction()) {
							params->sbDefineLayout->add(String::format("%s%s->setItemMaximumSize(%d, %s);%n", strTab, name, i, subItem.maxSize.getAccessString()));
							flagRelayoutOnLayout = sl_true;
						} else {
							params->sbDefineInit->add(String::format("%s%s->setItemMaximumSize(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, subItem.maxSize.getAccessString()));
							flagRelayoutOnInit = sl_true;
						}
					}
					if (subItem.dividerWidth.flagDefined) {
						if (subItem.dividerWidth.isNeededOnLayoutFunction()) {
							params->sbDefineLayout->add(String::format("%s%s->setItemDividerWidth(%d, %s);%n", strTab, name, i, subItem.dividerWidth.getAccessString()));
							flagRelayoutOnLayout = sl_true;
						} else {
							params->sbDefineInit->add(String::format("%s%s->setItemDividerWidth(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, subItem.dividerWidth.getAccessString()));
							flagRelayoutOnInit = sl_true;
						}
					}
					if (subItem.dividerBackground.flagDefined) {
						String strDividerBackground;
						if (!(_getDrawableAccessString(resource->name, subItem.dividerBackground, strDividerBackground))) {
							return sl_false;
						}
						params->sbDefineInit->add(String::format("%s%s->setItemDividerBackground(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, strDividerBackground));
						flagRelayoutOnInit = sl_true;
					}
					if (subItem.dividerColor.flagDefined) {
						String strDividerColor;
						if (!(_getColorAccessString(resource->name, subItem.dividerBackground, strDividerColor))) {
							return sl_false;
						}
						params->sbDefineInit->add(String::format("%s%s->setItemDividerColor(%d, %s, slib::UIUpdateMode::Init);%n", strTab, name, i, strDividerColor));
						flagRelayoutOnInit = sl_true;
					}
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
					if (!flagOnLayout) {
						if (subItem.weight.flagDefined) {
							view->setItemWeight(i, subItem.weight.value, UIUpdateMode::Init);
						}
						if (subItem.minWeight.flagDefined) {
							view->setItemMinimumWeight(i, subItem.minWeight.value, UIUpdateMode::Init);
						}
						if (subItem.maxWeight.flagDefined) {
							view->setItemMaximumWeight(i, subItem.maxWeight.value, UIUpdateMode::Init);
						}
						if (subItem.minSize.flagDefined) {
							view->setItemMinimumSize(i, _getDimensionValue(subItem.minSize));
						}
						if (subItem.maxSize.flagDefined) {
							view->setItemMaximumSize(i, _getDimensionValue(subItem.maxSize));
						}
						if (subItem.dividerWidth.flagDefined) {
							view->setItemDividerWidth(i, _getDimensionValue(subItem.dividerWidth));
						}
						if (subItem.dividerColor.flagDefined) {
							Color dividerColor;
							if (!(_getColorValue(resource->name, subItem.dividerColor, dividerColor))) {
								return sl_false;
							}
							view->setItemDividerColor(i, dividerColor, UIUpdateMode::Init);
						}
					}
					if (subItem.dividerBackground.flagDefined) {
						if (flagOnLayout) {
							Ref<Drawable> dividerBackground;
							if (!(_getDrawableValue(resource->name, subItem.dividerBackground, dividerBackground))) {
								return sl_false;
							}
							view->setItemDividerBackground(i, dividerBackground, UIUpdateMode::None);
						}
					}
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
			if (flagRelayoutOnInit) {
				params->sbDefineInit->add(String::format("%s%s->relayout(slib::UIUpdateMode::None);%n%n", strTab, name));
			}
			if (flagRelayoutOnLayout) {
				params->sbDefineLayout->add(String::format("%s%s->relayout();%n%n", strTab, name));
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Web, WebView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		if (op == OP_PARSE) {
			LAYOUT_CONTROL_PARSE_STRING_ATTR(attr->, url)
			LAYOUT_CONTROL_PARSE_STRING_ATTR(attr->, html)
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
						params->sbDefineInit->add(String::format("%s%s->loadHTML(%s, %s);%n%n", strTab, name, strHtml, strUrl));
					} else {
						params->sbDefineInit->add(String::format("%s%s->loadHTML(%s, sl_null);%n%n", strTab, name, strHtml));
					}
				} else {
					if (attr->url.flagDefined) {
						params->sbDefineInit->add(String::format("%s%s->loadURL(%s);%n%n", strTab, name, strUrl));
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

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(orientation, setOrientation)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(min, setMinimumValue)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(max, setMaximumValue)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(value, setValue)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(value2, setSecondaryValue)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(dual, setDualValues)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(discrete, setDiscrete)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(step, setStep)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(reversed, setReversed)
		LAYOUT_CONTROL_DRAWABLE_ATTR(track, setTrackDrawable)
		LAYOUT_CONTROL_DRAWABLE_ATTR(progress, setProgressDrawable)
		LAYOUT_CONTROL_DRAWABLE_ATTR(progress2, setSecondaryProgressDrawable)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Slider, Slider)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(Progress)

		LAYOUT_CONTROL_DRAWABLE_ATTR(thumb, setThumbDrawable)
		LAYOUT_CONTROL_DRAWABLE_ATTR(pressedThumb, setPressedThumbDrawable)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverThumb, setHoverThumbDrawable);

		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(thumbWidth, setThumbWidth, checkScalarSize);
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(thumbHeight, setThumbHeight, checkScalarSize);
		if (op == OP_PARSE) {
			SAppDimensionValue thumbSize;
			LAYOUT_CONTROL_PARSE_DIMENSION_ATTR(, thumbSize, checkScalarSize)
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

		if (op == OP_PARSE) {
			String value = resourceItem->getXmlAttribute("value").trim().toLower();
			if (value == "left" || value == "off") {
				attr->value.flagDefined = sl_true;
				attr->value.value = sl_false;
			} else if (value == "right" || value == "on") {
				attr->value.flagDefined = sl_true;
				attr->value.value = sl_true;
			} else {
				LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(value, setValue)
			}
		} else {
			LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(value, setValue)
		}

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(textInButton, setTextInButton)

		LAYOUT_CONTROL_STRING_ATTR_UPDATE(text, setText)
		LAYOUT_CONTROL_STRING_ATTR_UPDATE(textOff, setTextOff)
		LAYOUT_CONTROL_STRING_ATTR_UPDATE(textOn, setTextOn)
		LAYOUT_CONTROL_STRING_ATTR_UPDATE(leftText, setLeftText)
		LAYOUT_CONTROL_STRING_ATTR_UPDATE(rightText, setRightText)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColor, setTextColor)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColorOff, setTextColorOff)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColorOn, setTextColorOn)

		LAYOUT_CONTROL_DRAWABLE_ATTR(thumb, setThumb)
		LAYOUT_CONTROL_DRAWABLE_ATTR(thumbOff, setThumbOff)
		LAYOUT_CONTROL_DRAWABLE_ATTR(thumbOn, setThumbOn)
		LAYOUT_CONTROL_DRAWABLE_ATTR(track, setTrack)
		LAYOUT_CONTROL_DRAWABLE_ATTR(trackOff, setTrackOff)
		LAYOUT_CONTROL_DRAWABLE_ATTR(trackOn, setTrackOn)
		LAYOUT_CONTROL_DRAWABLE_ATTR(pressedThumb, setPressedThumb)
		LAYOUT_CONTROL_DRAWABLE_ATTR(pressedThumbOff, setPressedThumbOff)
		LAYOUT_CONTROL_DRAWABLE_ATTR(pressedThumbOn, setPressedThumbOn)
		LAYOUT_CONTROL_DRAWABLE_ATTR(pressedTrack, setPressedTrackOn)
		LAYOUT_CONTROL_DRAWABLE_ATTR(pressedTrackOff, setPressedTrack)
		LAYOUT_CONTROL_DRAWABLE_ATTR(pressedTrackOn, setPressedTrackOff)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverThumb, setHoverThumb)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverThumbOff, setHoverThumbOff)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverThumbOn, setHoverThumbOn)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverTrack, setHoverTrack)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverTrackOff, setHoverTrackOff)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverTrackOn, setHoverTrackOn)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Picker, PickerView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColor, setTextColor)
		LAYOUT_CONTROL_GENERIC_ATTR(circular, setCircular)
		LAYOUT_CONTROL_GENERIC_ATTR(lineCount, setLineCount)

		LAYOUT_CONTROL_SET_NATIVE_WIDGET

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(DatePicker, DatePicker)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(date, setDate)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Pager, ViewPager)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_GENERIC_ATTR(loop, setLoop)

		if (op == OP_PARSE) {
			LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(itemXmls, "item")
			for (sl_size i = 0; i < itemXmls.count; i++) {
				LAYOUT_CONTROL_DEFINE_XML(itemXml, itemXmls[i])
				SAppLayoutPagerItem subItem;
				LAYOUT_CONTROL_PARSE_XML_ATTR(itemXml, subItem., selected)
				LAYOUT_CONTROL_DEFINE_XML_CHILDREN(childXmls, itemXml, sl_null)
				if (childXmls.count > 0) {
					if (childXmls.count != 1) {
						_logError(itemXml.element, g_str_error_resource_layout_item_must_contain_one_child);
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
					_logError(itemXml.element, g_str_error_out_of_memory);
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
				params->sbDefineInit->add(String::format("%s%s->selectPage(%d);%n", strTab, name, indexSelected));
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Navigation, ViewPageNavigationController)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_GENERIC_ATTR(swipe, setSwipeNavigation)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Video, VideoView)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_GENERIC_ATTR(repeat, setRepeat)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(rotation, setRotation)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(flip, setFlip)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(scale, setScaleMode)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(controls, setControlsVisible)

		if (op == OP_PARSE || op == OP_GENERATE_CPP) {
			LAYOUT_CONTROL_STRING_ATTR(src, setSource)
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

		LAYOUT_CONTROL_STRING_ATTR(device, setDeviceId)
		LAYOUT_CONTROL_GENERIC_ATTR(autoStart, setAutoStart)
		LAYOUT_CONTROL_GENERIC_ATTR(touchFocus, setTouchFocusEnabled)

		LAYOUT_CONTROL_ADD_STATEMENT
	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Drawer, Drawer)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_DIMENSION_ATTR(drawerSize, setDrawerSize, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR(dragEdgeSize, setDragEdgeSize, checkScalarSize)
		LAYOUT_CONTROL_GENERIC_ATTR(gravity, setGravity)

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
					_logError(element, g_str_error_resource_layout_refreshview_must_contain_one_child);
					return sl_false;
				}
			}
		}

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(ListBox, ListBox)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(itemCount, setItemCount)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(itemHeight, setItemHeight, checkScalarSize)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(multipleSelection, setMultipleSelection)
		LAYOUT_CONTROL_DRAWABLE_ATTR(itemBackground, setItemBackground)
		LAYOUT_CONTROL_DRAWABLE_ATTR(selectedItemBackground, setSelectedItemBackground)
		LAYOUT_CONTROL_DRAWABLE_ATTR(hoverItemBackground, setHoverItemBackground)
		LAYOUT_CONTROL_DRAWABLE_ATTR(focusedItemBackground, setHoverItemBackground)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(LabelList, LabelList)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(ListBox)

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(lineHeightWeight, setLineHeightWeight)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(textColor, setTextColor)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(gravity, setGravity)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(ellipsize, setEllipsize)

		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(selectedTextColor, setSelectedTextColor)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(hoverTextColor, setHoverTextColor)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(focusedTextColor, setFocusedTextColor)

		LAYOUT_CONTROL_PROCESS_SELECT_ITEMS

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(TileLayout, TileLayout)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(columns, setColumnCount)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(rows, setRowCount)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(columnWidth, setColumnWidth, checkScalarSize)
		LAYOUT_CONTROL_DIMENSION_ATTR_UPDATE(rowHeight, setRowHeight, checkScalarSize)
		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(cellRatio, setCellRatio)

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

		LAYOUT_CONTROL_STRING_ATTR_UPDATE(label, setLabel)
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(labelColor, setLabelColor)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

	BEGIN_PROCESS_LAYOUT_CONTROL(Grid, GridView)
	{

		LAYOUT_CONTROL_PROCESS_SUPER(View)

		if (op == OP_PARSE) {

#define LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(ATTR, XML) \
			{ \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., align) \
				LAYOUT_CONTROL_PARSE_XML_FONT_ATTR(XML, ATTR., font) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., multiLine) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., ellipsize) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., lineCount) \
				LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(XML, ATTR., background) \
				LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(XML, ATTR., backgroundHover) \
				LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(XML, ATTR., backgroundSelected) \
				LAYOUT_CONTROL_PARSE_XML_COLOR_ATTR(XML, ATTR., textColor) \
				LAYOUT_CONTROL_PARSE_XML_COLOR_ATTR(XML, ATTR., textColorHover) \
				LAYOUT_CONTROL_PARSE_XML_COLOR_ATTR(XML, ATTR., textColorSelected) \
			}
			{
				LAYOUT_CONTROL_DEFINE_ITEM_CHILDREN(columnXmls, "column")
				sl_bool flagLeft = sl_true;
				for (sl_size i = 0; i < columnXmls.count; i++) {
					LAYOUT_CONTROL_DEFINE_XML(columnXml, columnXmls[i])
					SAppLayoutGridColumn column;
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(columnXml, column., width, checkSize)
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
					LAYOUT_CONTROL_PARSE_XML_ATTR(columnXml, ,fixed)
					if (!(fixed.flagDefined && fixed.value)) {
						flagLeft = sl_false;
						attr->nRightColumns = (sl_uint32)(columnXmls.count - 1 - i);
					}
					if (flagLeft) {
						attr->nLeftColumns = (sl_uint32)(i + 1);
					}
					if (!(attr->columns.add_NoLock(Move(column)))) {
						_logError(columnXml.element, g_str_error_out_of_memory);
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
					LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(rowXml, row., height, checkSize) \
					sl_uint32 iCell = 0; \
					LAYOUT_CONTROL_DEFINE_XML_CHILDREN(cellXmls, rowXml, sl_null) \
					for (sl_size k = 0; k < cellXmls.count; k++) { \
						LAYOUT_CONTROL_DEFINE_XML(cellXml, cellXmls[k]) \
						SAppLayoutGridCell cell; \
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
						LAYOUT_CONTROL_PARSE_XML_ATTR(cellXml, cell., rowspan) \
						LAYOUT_CONTROL_PARSE_XML_ATTR(cellXml, cell., colspan) \
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
								_logError(cellXml.element, g_str_error_out_of_memory); \
								return sl_false; \
							} \
						} \
						for (sl_uint32 t1 = 0; t1 < cell.rowspan.value; t1++) { \
							for (sl_uint32 t2 = 0; t2 < cell.colspan.value; t2++) { \
								cellAllocs.put_NoLock(Pair<sl_uint32, sl_uint32>((sl_uint32)iRow + t1, (sl_uint32)iCell + t2), sl_true); \
							} \
						} \
						if (!(row.cells.setCount_NoLock(iCell+1))) { \
							_logError(cellXml.element, g_str_error_out_of_memory); \
							return sl_false; \
						} \
						row.cells.setAt_NoLock(iCell, Move(cell)); \
					} \
					row.font.inheritFrom(attr->font); \
					if (!(attr->SECTION.rows.add_NoLock(Move(row)))) { \
						_logError(rowXml.element, g_str_error_out_of_memory); \
						return sl_false; \
					} \
				} \
			}

#define LAYOUT_CONTROL_PARSE_GRID_SECTION(SECTION, XML) \
			{ \
				LAYOUT_CONTROL_PARSE_GRID_CELL_ATTRIBUTES(attr->SECTION, XML) \
				LAYOUT_CONTROL_PARSE_XML_DIMENSION_ATTR(XML, attr->SECTION., rowHeight, checkSize) \
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


		} else if (op == OP_SIMULATE) {

			ListElements<SAppLayoutGridColumn> columns(attr->columns);
			if (!flagOnLayout) {
				view->setColumnCount((sl_uint32)(columns.count), UIUpdateMode::Init);
				view->setLeftColumnCount(attr->nLeftColumns, UIUpdateMode::Init);
				view->setRightColumnCount(attr->nRightColumns, UIUpdateMode::Init);
				view->setRowCount((sl_uint32)(attr->body.rows.getCount()), UIUpdateMode::Init);
				view->setHeaderRowCount((sl_uint32)(attr->header.rows.getCount()), UIUpdateMode::Init);
				view->setFooterRowCount((sl_uint32)(attr->footer.rows.getCount()), UIUpdateMode::Init);
			}

#define LAYOUT_CONTROL_SIMULATE_GRID_CELL_ATTRIBUTES(ATTR, XML) \
			{ \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., align) \
				LAYOUT_CONTROL_PARSE_XML_FONT_ATTR(XML, ATTR., font) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., multiLine) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., ellipsize) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., lineCount) \
				LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(XML, ATTR., background) \
				LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(XML, ATTR., backgroundHover) \
				LAYOUT_CONTROL_PARSE_XML_DRAWABLE_ATTR(XML, ATTR., backgroundSelected) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., textColor) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., textColorHover) \
				LAYOUT_CONTROL_PARSE_XML_ATTR(XML, ATTR., textColorSelected) \
			}

			{
				for (sl_size iCol = 0; iCol < columns.count; iCol++) {
					SAppLayoutGridColumn& column = columns[iCol];
					if (flagOnLayout) {
						view->setColumnWidth((sl_uint32)iCol, _getDimensionValue(column.width), UIUpdateMode::None);
					}
				}
			}

#define LAYOUT_CONTROL_SIMULATE_GRID_SECTION(SECTION) \
			{ \
				\
			}

		}

		LAYOUT_CONTROL_GENERIC_ATTR_UPDATE(recordCount, setRecordCount)

		LAYOUT_CONTROL_ADD_STATEMENT

	}
	END_PROCESS_LAYOUT_CONTROL

#define PROCESS_XCONTROL_ATTRS \
		LAYOUT_CONTROL_COLOR_ATTR_UPDATE(focusedBorderColor, setFocusedBorderColor)

	BEGIN_PROCESS_LAYOUT_CONTROL(XControl, XControl)
	{
		LAYOUT_CONTROL_PROCESS_SUPER(View)

		PROCESS_XCONTROL_ATTRS

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
